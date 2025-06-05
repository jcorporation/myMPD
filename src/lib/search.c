/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Search implementation
 */

#include "compile_time.h"
#include "src/lib/search.h"

#include "dist/libmympdclient/src/iaf.h"
#include "dist/utf8/utf8.h"
#include "src/lib/convert.h"
#include "src/lib/datetime.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/sds_extras.h"

#include <ctype.h>
#include <inttypes.h>

/**
 * PCRE for UTF-8
 */
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include <string.h>

/**
 * Private definitions
 */

/**
 * Search operators like them from MPD
 */
enum search_operators {
    SEARCH_OP_UNKNOWN = -1,
    SEARCH_OP_EQUAL,
    SEARCH_OP_STARTS_WITH,
    SEARCH_OP_CONTAINS,
    SEARCH_OP_NOT_EQUAL,
    SEARCH_OP_REGEX,
    SEARCH_OP_NOT_REGEX,
    SEARCH_OP_GREATER,
    SEARCH_OP_GREATER_EQUAL
};

/**
 * Search filter types
 */
enum search_filters {
    SEARCH_FILTER_ANY_TAG = -2,
    SEARCH_FILTER_MODIFIED_SINCE = -3,
    SEARCH_FILTER_ADDED_SINCE = -4,
    SEARCH_FILTER_FILE = -5,
    SEARCH_FILTER_BITRATE = -6,
    SEARCH_FILTER_BASE = -7,
    SEARCH_FILTER_PRIO = -8,
    SEARCH_FILTER_AUDIO_FORMAT = -9
};

/**
 * Struct to hold a parsed search expression triple
 */
struct t_search_expression {
    int tag;                               //!< Tag to search in
    enum search_operators op;              //!< Search operator
    sds value;                             //!< Value to match
    int64_t value_i;                       //!< Integer value to match
    pcre2_code *re_compiled;               //!< Compiled regex if operator is a regex
};

static int expr_get_tag_song(const char *p, size_t *len);
static int expr_get_tag_webradio(const char *p, size_t *len);
static int expr_get_op(const char *p, int tag, size_t *len);
static sds expr_get_value(const char *p, const char *end, int tag, sds buf, bool *rc);
static bool expr_parse_value(struct t_search_expression *expr);
static void *free_search_expression(struct t_search_expression *expr);
static void free_search_expression_node(struct t_list_node *current);
static pcre2_code *compile_regex(char *regex_str);
static bool cmp_regex(pcre2_code *re_compiled, const char *value);

/**
 * Public functions
 */

/**
 * Parses a mpd search expression
 * @param expression mpd search expression
 * @param type type of struct for the search expression
 * @return list of the expression or NULL on error
 */
struct t_list *parse_search_expression_to_list(const char *expression, enum search_type type) {
    struct t_list *expr_list = list_new();
    int count = 0;
    sds *tokens = sdssplitlen(expression, (ssize_t)strlen(expression), ") AND (", 7, &count);
    for (int j = 0; j < count; j++) {
        sdstrim(tokens[j], "() ");
        MYMPD_LOG_DEBUG(NULL, "Parsing expression: %s", tokens[j]);
        struct t_search_expression *expr = malloc_assert(sizeof(struct t_search_expression));
        expr->value = sdsempty();
        expr->re_compiled = NULL;
        char *p = tokens[j];
        char *end = p + sdslen(tokens[j]) - 1;
        // Tag
        size_t skip;
        expr->tag = type == SEARCH_TYPE_SONG
            ? expr_get_tag_song(p, &skip)
            : expr_get_tag_webradio(p, &skip);
        p += skip;
        if (expr->tag == MPD_TAG_UNKNOWN ||
            p > end)
        {
            MYMPD_LOG_ERROR(NULL, "Can not parse search expression, tag: %d, skip: %lu", expr->tag, (unsigned long)skip);
            free_search_expression(expr);
            free_search_expression_list(expr_list);
            expr_list = NULL;
            break;
        }

        // Operator
        expr->op = expr_get_op(p, expr->tag, &skip);
        p += skip;
        if (expr->op == SEARCH_OP_UNKNOWN ||
            p > end)
        {
            MYMPD_LOG_ERROR(NULL, "Can not parse search expression, tag: %d, op: %d, skip: %lu", expr->tag, expr->op, (unsigned long)skip);
            free_search_expression(expr);
            free_search_expression_list(expr_list);
            expr_list = NULL;
            break;
        }

        // Value
        bool rc;
        expr->value = expr_get_value(p, end, expr->tag, expr->value, &rc);
        if (rc == false ||
            expr_parse_value(expr) == false)
        {
            MYMPD_LOG_ERROR(NULL, "Can not parse search expression, tag: %d, op: %d, value: \"%s\"", expr->tag, expr->op, expr->value);
            free_search_expression(expr);
            free_search_expression_list(expr_list);
            expr_list = NULL;
            break;
        }

        list_push(expr_list, "", 0, NULL, expr);
        MYMPD_LOG_DEBUG(NULL, "Parsed expression tag: \"%d\", op: \"%d\", value:\"%s\"", expr->tag, expr->op, expr->value);
    }
    sdsfreesplitres(tokens, count);
    return expr_list;
}

/**
 * Frees the search expression list
 * @param expr_list pointer to the list
 */
void free_search_expression_list(struct t_list *expr_list) {
    if (expr_list == NULL) {
        return;
    }
    list_free_user_data(expr_list, free_search_expression_node);
}

/**
 * Implements search expressions for mpd songs.
 * @param song pointer to mpd song struct
 * @param expr_list expression list returned by parse_search_expression
 * @param any_tag_types tags for special "any" tag in expression
 * @return expression result
 */
bool search_expression_song(const struct mpd_song *song, const struct t_list *expr_list, const struct t_mympd_mpd_tags *any_tag_types) {
    struct t_mympd_mpd_tags one_tag;
    one_tag.len = 1;
    struct t_list_node *current = expr_list->head;
    while (current != NULL) {
        struct t_search_expression *expr = (struct t_search_expression *)current->user_data;
        if (expr->tag == SEARCH_FILTER_MODIFIED_SINCE) {
            if (expr->value_i > mpd_song_get_last_modified(song)) {
                return false;
            }
        }
        else if (expr->tag == SEARCH_FILTER_ADDED_SINCE) {
            if (expr->value_i > mpd_song_get_added(song)) {
                return false;
            }
        }
        else if (expr->tag == SEARCH_FILTER_FILE) {
            if (strcmp(mpd_song_get_uri(song), expr->value) != 0) {
                return false;
            }
        }
        else if (expr->tag == SEARCH_FILTER_BASE) {
            if (strncmp(expr->value, mpd_song_get_uri(song), sdslen(expr->value)) != 0) {
                return false;
            }
        }
        else if (expr->tag == SEARCH_FILTER_PRIO) {
            if (expr->value_i > mpd_song_get_prio(song)) {
                return false;
            }
        }
        else if (expr->tag == SEARCH_FILTER_AUDIO_FORMAT) {
            // Not implemented
        }
        else {
            one_tag.tags[0] = (enum mpd_tag_type)expr->tag;
            const struct t_mympd_mpd_tags *tags = expr->tag == SEARCH_FILTER_ANY_TAG
                ? any_tag_types  //any - use provided tags
                : &one_tag;      //use only selected tag

            bool rc = false;
            for (size_t i = 0; i < tags->len; i++) {
                rc = true;
                unsigned j = 0;
                const char *value = NULL;
                while ((value = mpd_song_get_tag(song, tags->tags[i], j)) != NULL) {
                    j++;
                    if ((expr->op == SEARCH_OP_CONTAINS && utf8casestr(value, expr->value) == NULL) ||
                        (expr->op == SEARCH_OP_STARTS_WITH && utf8ncasecmp(expr->value, value, sdslen(expr->value)) != 0) ||
                        (expr->op == SEARCH_OP_EQUAL && utf8casecmp(value, expr->value) != 0) ||
                        (expr->op == SEARCH_OP_REGEX && cmp_regex(expr->re_compiled, value) == false))
                    {
                        //expression does not match
                        rc = false;
                    }
                    else if ((expr->op == SEARCH_OP_NOT_EQUAL && utf8casecmp(value, expr->value) == 0) ||
                            (expr->op == SEARCH_OP_NOT_REGEX && cmp_regex(expr->re_compiled, value) == true))
                    {
                        //negated match operator - exit instantly
                        rc = false;
                        break;
                    }
                    else {
                        //tag value matched
                        rc = true;
                        if (expr->op != SEARCH_OP_NOT_EQUAL &&
                            expr->op != SEARCH_OP_NOT_REGEX)
                        {
                            //exit only for positive match operators
                            break;
                        }
                    }
                }
                if (j == 0) {
                    //no tag value found
                    rc = expr->op == SEARCH_OP_NOT_EQUAL || expr->op == SEARCH_OP_NOT_REGEX
                        ? true
                        : false;
                }
                if (rc == true) {
                    //exit on first tag value match
                    break;
                }
            }
            if (rc == false) {
                //exit on first expression mismatch
                return false;
            }
        }
        current = current->next;
    }
    //complete expression has matched
    return true;
}

/**
 * Implements search expressions for webradios.
 * @param webradio pointer to webradio struct
 * @param expr_list expression list returned by parse_search_expression
 * @param any_tag_types tags for special "any" tag in expression
 * @return expression result
 */
bool search_expression_webradio(const struct t_webradio_data *webradio, const struct t_list *expr_list, const struct t_webradio_tags *any_tag_types) {
    struct t_webradio_tags one_tag;
    one_tag.len = 1;
    struct t_list_node *current = expr_list->head;
    while (current != NULL) {
        struct t_search_expression *expr = (struct t_search_expression *)current->user_data;
        if (expr->tag == SEARCH_FILTER_BITRATE) {
            struct t_list_node *uris = webradio->uris.head;
            bool rc = false;
            while (uris != NULL) {
                if (expr->value_i > uris->value_i) {
                    rc = true;
                    break;
                }
                uris = uris->next;
            }
            if (rc == false) {
                return false;
            }
        }
        else {
            one_tag.tags[0] = (enum webradio_tag_type)expr->tag;
            const struct t_webradio_tags *tags = expr->tag == SEARCH_FILTER_ANY_TAG
                ? any_tag_types  //any - use provided tags
                : &one_tag;      //use only selected tag

            bool rc = false;
            for (size_t i = 0; i < tags->len; i++) {
                rc = true;
                unsigned j = 0;
                const char *value = NULL;
                while ((value = webradio_get_tag(webradio, tags->tags[i], j)) != NULL) {
                    j++;
                    if ((expr->op == SEARCH_OP_CONTAINS && utf8casestr(value, expr->value) == NULL) ||
                        (expr->op == SEARCH_OP_STARTS_WITH && utf8ncasecmp(expr->value, value, sdslen(expr->value)) != 0) ||
                        (expr->op == SEARCH_OP_EQUAL && utf8casecmp(value, expr->value) != 0) ||
                        (expr->op == SEARCH_OP_REGEX && cmp_regex(expr->re_compiled, value) == false))
                    {
                        //expression does not match
                        rc = false;
                    }
                    else if ((expr->op == SEARCH_OP_NOT_EQUAL && utf8casecmp(value, expr->value) == 0) ||
                            (expr->op == SEARCH_OP_NOT_REGEX && cmp_regex(expr->re_compiled, value) == true))
                    {
                        //negated match operator - exit instantly
                        rc = false;
                        break;
                    }
                    else {
                        //tag value matched
                        rc = true;
                        if (expr->op != SEARCH_OP_NOT_EQUAL &&
                            expr->op != SEARCH_OP_NOT_REGEX)
                        {
                            //exit only for positive match operators
                            break;
                        }
                    }
                }
                if (j == 0) {
                    //no tag value found
                    rc = expr->op == SEARCH_OP_NOT_EQUAL || expr->op == SEARCH_OP_NOT_REGEX
                        ? true
                        : false;
                }
                if (rc == true) {
                    //exit on first tag value match
                    break;
                }
            }
            if (rc == false) {
                //exit on first expression mismatch
                return false;
            }
        }
        current = current->next;
    }
    //complete expression has matched
    return true;
}

/**
 * Private functions
 */

/**
 * Extracts the tag from a song search expression
 * @param p Pointer to start of expression
 * @param len Set to count of bytes parsed
 * @return MPD song tag, filter type or -1 on error
 */
static int expr_get_tag_song(const char *p, size_t *len) {
    size_t tag_len = 0;
    *len = 0;
    while (*p != ' ') {
        if (*p == '\0') {
            return MPD_TAG_UNKNOWN;
        }
        tag_len++;
        p++;
    }
    sds tag_str = sdsnewlen(p - tag_len, tag_len);
    int tag = mpd_tag_name_iparse(tag_str);
    if (tag == MPD_TAG_UNKNOWN) {
        if (strcasecmp(tag_str, "any") == 0) {
            tag = SEARCH_FILTER_ANY_TAG;
        }
        else if (strcasecmp(tag_str, "modified-since") == 0) {
            tag = SEARCH_FILTER_MODIFIED_SINCE;
        }
        else if (strcasecmp(tag_str, "added-since") == 0) {
            tag = SEARCH_FILTER_ADDED_SINCE;
        }
        else if (strcasecmp(tag_str, "file") == 0) {
            tag = SEARCH_FILTER_FILE;
        }
        else if (strcasecmp(tag_str, "base") == 0) {
            tag = SEARCH_FILTER_BASE;
        }
        else if (strcasecmp(tag_str, "prio") == 0) {
            tag = SEARCH_FILTER_PRIO;
        }
        else if (strcasecmp(tag_str, "AudioFormat") == 0) {
            tag = SEARCH_FILTER_AUDIO_FORMAT;
        }
        else {
            FREE_SDS(tag_str);
            return MPD_TAG_UNKNOWN;
        }
    }
    FREE_SDS(tag_str);
    while (*p == ' ') {
        if (*p == '\0') {
            return MPD_TAG_UNKNOWN;
        }
        tag_len++;
        p++;
    }
    *len = tag_len;
    return tag;
}

/**
 * Extracts the tag from a webradio search expression
 * @param p Pointer to start of expression
 * @param len Set to count of bytes parsed
 * @return Webradio tag, filter type or -1 on error
 */
static int expr_get_tag_webradio(const char *p, size_t *len) {
    size_t tag_len = 0;
    *len = 0;
    while (*p != ' ') {
        if (*p == '\0') {
            return WEBRADIO_TAG_UNKNOWN;
        }
        tag_len++;
        p++;
    }
    sds tag_str = sdsnewlen(p - tag_len, tag_len);
    int tag = webradio_tag_name_parse(tag_str);
    if (tag == WEBRADIO_TAG_UNKNOWN) {
        if (strcasecmp(tag_str, "any") == 0) {
            tag = SEARCH_FILTER_ANY_TAG;
        }
    }
    FREE_SDS(tag_str);
    while (*p == ' ') {
        if (*p == '\0') {
            return WEBRADIO_TAG_UNKNOWN;
        }
        tag_len++;
        p++;
    }
    *len = tag_len;
    return tag;
}

/**
 * Extracts the search operator
 * @param p Pointer to start of operator in the expression
 * @param tag Filter type
 * @param len Set to count of bytes parsed
 * @return Search operator or -1 on error
 */
static int expr_get_op(const char *p, int tag, size_t *len) {
    *len = 0;
    switch (tag) {
        case SEARCH_FILTER_MODIFIED_SINCE:
        case SEARCH_FILTER_ADDED_SINCE:
        case SEARCH_FILTER_BITRATE:
            return SEARCH_OP_GREATER;
        case SEARCH_FILTER_BASE:
            return SEARCH_OP_STARTS_WITH;
        case SEARCH_FILTER_FILE:
            return SEARCH_OP_EQUAL;
    }
    size_t op_len = 0;
    *len = 0;
    while (*p != ' ') {
        if (*p == '\0') {
            return SEARCH_OP_UNKNOWN;
        }
        op_len++;
        p++;
    }
    int op = SEARCH_OP_UNKNOWN;
    sds op_str = sdsnewlen(p - op_len, op_len);
    if (strcmp(op_str, "contains") == 0) { op = SEARCH_OP_CONTAINS; }
    else if (strcmp(op_str, "starts_with") == 0) { op = SEARCH_OP_STARTS_WITH; }
    else if (strcmp(op_str, "==") == 0) { op = SEARCH_OP_EQUAL; }
    else if (strcmp(op_str, "!=") == 0) { op = SEARCH_OP_NOT_EQUAL; }
    else if (strcmp(op_str, "=~") == 0) { op = SEARCH_OP_REGEX; }
    else if (strcmp(op_str, "!~") == 0) { op = SEARCH_OP_NOT_REGEX; }
    else if (tag == SEARCH_FILTER_PRIO && strcmp(op_str, ">=") == 0) { op = SEARCH_OP_GREATER_EQUAL; }
    FREE_SDS(op_str);
    while (*p == ' ') {
        if (*p == '\0') {
            return SEARCH_OP_UNKNOWN;
        }
        op_len++;
        p++;
    }
    *len = op_len;
    return op;
}

/**
 * Extracts the value to search for
 * @param p Pointer to start of value in the expression
 * @param end Pointer to end of the expression
 * @param tag Filter type
 * @param buf Already allocated sds string to append the value
 * @param rc Set to true on success, else false
 * @return Pointer to buf
 */
static sds expr_get_value(const char *p, const char *end, int tag, sds buf, bool *rc) {
    if (tag == SEARCH_FILTER_PRIO) {
        // Value is not in quotes and must be a number
        while (*p != '\0') {
            if (isdigit(*p) == 0) {
                *rc = false;
                return buf;
            }
            buf = sds_catchar(buf, *p);
            p++;
        }
        *rc = true;
        return buf;
    }
    // Value must be enclosed in quotes
    if (*p != '\'' && *p != '"') {
        *rc = false;
        return buf;
    }
    // Must end with same quote type
    if (p == end || *p != *end) {
        *rc = false;
        return buf;
    }
    p++;
    // Parse and unescape value
    while (p < end) {
        if (*p == '\\') {
            if (p + 1 >= end) {
                // Escape char should not be the last
                *rc = false;
                return buf;
            }
            // Skip escaping backslash
            p++;
        }
        buf = sds_catchar(buf, *p);
        p++;
    }
    *rc = true;
    return buf;
}

/**
 * Parses the search expression value
 * @param expr The parsed search expression
 * @return true on success, else false
 */
static bool expr_parse_value(struct t_search_expression *expr) {
    if (expr->tag == SEARCH_FILTER_BITRATE ||
        expr->tag == SEARCH_FILTER_PRIO)
    {
        // Convert to number
        if (str2int64(&expr->value_i, expr->value) != STR2INT_SUCCESS) {
            MYMPD_LOG_ERROR(NULL, "Can not parse search expression, invalid number");
            return false;
        }
    }
    else if (expr->tag == SEARCH_FILTER_ADDED_SINCE ||
             expr->tag == SEARCH_FILTER_MODIFIED_SINCE)
    {
        // Parse date
        expr->value_i = parse_date(expr->value);
        if (expr->value_i == 0) {
            return false;
        }
    }
    else if (expr->tag == SEARCH_FILTER_AUDIO_FORMAT) {
        // Not implemented
        return true;
    }
    else if (expr->op == SEARCH_OP_REGEX ||
             expr->op == SEARCH_OP_NOT_REGEX)
    {
        // Compile regex
        expr->re_compiled = compile_regex(expr->value);
        if (expr->re_compiled == NULL) {
            return false;
        }
    }
    return true;
}

/**
 * Frees the t_search_expression struct
 * @param expr pointer to t_search_expression struct
 * @return NULL
 */
static void *free_search_expression(struct t_search_expression *expr) {
    FREE_SDS(expr->value);
    FREE_PTR(expr->re_compiled);
    FREE_PTR(expr);
    return NULL;
}

/**
 * Callback function for freeing a list node with t_search_expression user_data
 * @param current pointer to list node
 */
static void free_search_expression_node(struct t_list_node *current) {
    struct t_search_expression *expr = (struct t_search_expression *)current->user_data;
    free_search_expression(expr);
}

/**
 * Compiles a string to regex code
 * @param regex_str regex string
 * @return regex code
 */
static pcre2_code *compile_regex(char *regex_str) {
    MYMPD_LOG_DEBUG(NULL, "Compiling regex: \"%s\"", regex_str);
    utf8lwr(regex_str);
    PCRE2_SIZE erroroffset;
    int rc;
    pcre2_code *re_compiled = pcre2_compile(
        (PCRE2_SPTR)regex_str, /* the pattern */
        PCRE2_ZERO_TERMINATED, /* indicates pattern is zero-terminated */
        0,                     /* default options */
        &rc,		           /* for error number */
        &erroroffset,          /* for error offset */
        NULL                   /* use default compile context */
    );
    if (re_compiled == NULL){
        //Compilation failed
        PCRE2_UCHAR buffer[256];
        pcre2_get_error_message(rc, buffer, sizeof(buffer));
        MYMPD_LOG_ERROR(NULL, "PCRE2 compilation failed at offset %lu: \"%s\"", (unsigned long)erroroffset, buffer);
    }
    return re_compiled;
}

/**
 * Matches the regex against a string
 * @param re_compiled the compiled regex from compile_regex
 * @param value string to match against
 * @return true if regex matches else false
 */
static bool cmp_regex(pcre2_code *re_compiled, const char *value) {
    if (re_compiled == NULL) {
        return false;
    }
    char *lower = strdup(value);
    utf8lwr(lower);
    pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(re_compiled, NULL);
    int rc = pcre2_match(
        re_compiled,          /* the compiled pattern */
        (PCRE2_SPTR)lower,    /* the subject string */
        strlen(value),        /* the length of the subject */
        0,                    /* start at offset 0 in the subject */
        0,                    /* default options */
        match_data,           /* block for storing the result */
        NULL                  /* use default match context */
    );
    pcre2_match_data_free(match_data);
    FREE_PTR(lower);
    if (rc >= 0) {
        return true;
    }
    //Matching failed: handle error cases
    switch(rc) {
        case PCRE2_ERROR_NOMATCH: 
            break;
        default: {
            PCRE2_UCHAR buffer[256];
            pcre2_get_error_message(rc, buffer, sizeof(buffer));
            MYMPD_LOG_ERROR(NULL, "PCRE2 matching error %d: \"%s\"", rc, buffer);
        }
    }
    return false;
}
