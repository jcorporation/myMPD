/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mpd_client/search_local.h"

#include "dist/utf8/utf8.h"
#include "src/lib/datetime.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"

#include <inttypes.h>

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
    SEARCH_OP_EQUAL,
    SEARCH_OP_STARTS_WITH,
    SEARCH_OP_CONTAINS,
    SEARCH_OP_NOT_EQUAL,
    SEARCH_OP_REGEX,
    SEARCH_OP_NOT_REGEX,
    SEARCH_OP_NEWER
};

/**
 * Struct to hold a parsed search expression triple
 */
struct t_search_expression {
    int tag;                   //!< tag to search in
    enum search_operators op;  //!< search operator
    sds value;                 //!< value to match
    time_t value_time;         //!< time value to match
    pcre2_code *re_compiled;   //!< compiled regex if operator is a regex
};

static void *free_search_expression(struct t_search_expression *expr);
static void free_search_expression_node(struct t_list_node *current);
static pcre2_code *compile_regex(char *regex_str);
static bool cmp_regex(pcre2_code *re_compiled, const char *value);

/**
 * Public functions
 */

/**
 * Searches for a string in mpd tag values
 * @param song pointer to mpd song struct
 * @param searchstr string to search for
 * @param tagcols tags to search
 * @return true if searchstr was found else false
 */
bool search_mpd_song(const struct mpd_song *song, sds searchstr, const struct t_tags *tagcols) {
    if (sdslen(searchstr) == 0) {
        return true;
    }
    bool rc = false;
    if (tagcols->tags_len == 0) {
        //fallback to filename if no tags are enabled
        sds filename = sdsnew(mpd_song_get_uri(song));
        basename_uri(filename);
        if (utf8casestr(filename, searchstr) != NULL) {
            rc = true;
        }
        FREE_SDS(filename);
        return rc;
    }
    for (unsigned i = 0; i < tagcols->tags_len; i++) {
        const char *value;
        unsigned idx = 0;
        while ((value = mpd_song_get_tag(song, tagcols->tags[i], idx)) != NULL) {
            if (utf8casestr(value, searchstr) != NULL) {
                rc = true;
                break;
            }
            idx++;
        }
    }
    return rc;
}

/**
 * Parses a mpd search expression
 * @param expression mpd search expression
 * @return list of the expression
 */
struct t_list *parse_search_expression_to_list(const char *expression) {
    struct t_list *expr_list = list_new();
    int count = 0;
    sds *tokens = sdssplitlen(expression, (ssize_t)strlen(expression), ") AND (", 7, &count);
    sds tag = sdsempty();
    sds op = sdsempty();
    for (int j = 0; j < count; j++) {
        sdstrim(tokens[j], "() ");
        sdsclear(tag);
        sdsclear(op);
        struct t_search_expression *expr = malloc_assert(sizeof(struct t_search_expression));
        expr->value = sdsempty();
        expr->re_compiled = NULL;
        char *p = tokens[j];
        char *end = p + sdslen(tokens[j]) - 1; //ignore concluding apostrophe
        //tag
        while (p < end) {
            if (*p == ' ') {
                break;
            }
            tag = sds_catchar(tag, *p);
            p++;
        }
        if (p + 1 >= end) {
            MYMPD_LOG_ERROR(NULL, "Can not parse search expression");
            free_search_expression(expr);
            break;
        }
        expr->tag = mpd_tag_name_parse(tag);
        if (expr->tag == MPD_TAG_UNKNOWN) {
            if (strcmp(tag, "any") == 0) {
                expr->tag = SEARCH_FILTER_ANY_TAG;
            }
            else if (strcmp(tag, "modified-since") == 0) {
                expr->tag = SEARCH_FILTER_MODIFIED_SINCE;
            }
            else if (strcmp(tag, "added-since") == 0) {
                expr->tag = SEARCH_FILTER_ADDED_SINCE;
            }
            else {
                MYMPD_LOG_ERROR(NULL, "Can not parse search expression, invalid tag");
                free_search_expression(expr);
                break;
            }
        }
        //skip space
        p++;
        //operator
        if (expr->tag == SEARCH_FILTER_MODIFIED_SINCE ||
            expr->tag == SEARCH_FILTER_ADDED_SINCE)
        {
            expr->op = SEARCH_OP_NEWER;
        }
        else {
            while (p < end) {
                if (*p == ' ') {
                    break;
                }
                op = sds_catchar(op, *p);
                p++;
            }
            if (p + 2 >= end) {
                MYMPD_LOG_ERROR(NULL, "Can not parse search expression");
                free_search_expression(expr);
                break;
            }
            if (strcmp(op, "contains") == 0) { expr->op = SEARCH_OP_CONTAINS; }
            else if (strcmp(op, "starts_with") == 0) { expr->op = SEARCH_OP_STARTS_WITH; }
            else if (strcmp(op, "==") == 0) { expr->op = SEARCH_OP_EQUAL; }
            else if (strcmp(op, "!=") == 0) { expr->op = SEARCH_OP_NOT_EQUAL; }
            else if (strcmp(op, "=~") == 0) { expr->op = SEARCH_OP_REGEX; }
            else if (strcmp(op, "!~") == 0) { expr->op = SEARCH_OP_NOT_REGEX; }
            else {
                MYMPD_LOG_ERROR(NULL, "Unknown search operator: \"%s\"", op);
                free_search_expression(expr);
                break;
            }
            p++;
        }
        //skip apostrophe
        p++;
        //value
        while (p < end) {
            if (*p == '\\') {
                if (p + 1 >= end) {
                    //escape char should not be the last
                    break;
                }
                //skip escaping backslash
                p++;
            }
            expr->value = sds_catchar(expr->value, *p);
            p++;
        }
        //push to list
        if (*end == '\'') {
            if (expr->op == SEARCH_OP_REGEX ||
                expr->op == SEARCH_OP_NOT_REGEX)
            {
                //is regex, compile
                expr->re_compiled = compile_regex(expr->value);
            }
            else if (expr->op == SEARCH_OP_NEWER) {
                expr->value_time = parse_date(expr->value);
                if (expr->value_time == 0) {
                    MYMPD_LOG_ERROR(NULL, "Can not parse search expression, invalid date");
                    free_search_expression(expr);
                    break;
                }
            }
            list_push(expr_list, "", 0, NULL, expr);
            MYMPD_LOG_DEBUG(NULL, "Parsed expression tag: \"%s\", op: \"%s\", value:\"%s\"", tag, op, expr->value);
        }
        else {
            free_search_expression(expr);
            MYMPD_LOG_ERROR(NULL, "Can not parse search expression");
            break;
        }
    }
    FREE_SDS(tag);
    FREE_SDS(op);
    sdsfreesplitres(tokens, count);
    return expr_list;
}

/**
 * Frees the search expression list
 * @param expr_list pointer to the list
 */
void *free_search_expression_list(struct t_list *expr_list) {
    if (expr_list == NULL) {
        return NULL;
    }
    return list_free_user_data(expr_list, free_search_expression_node);
}

/**
 * Searches for a string in mpd tag values
 * @param song pointer to mpd song struct
 * @param expr_list expression list returned by parse_search_expression
 * @param tag_types tags for special "any" tag in expression
 * @return expression result
 */
bool search_song_expression(const struct mpd_song *song, const struct t_list *expr_list, const struct t_tags *tag_types) {
    struct t_tags one_tag;
    one_tag.tags_len = 1;
    struct t_list_node *current = expr_list->head;
    while (current != NULL) {
        struct t_search_expression *expr = (struct t_search_expression *)current->user_data;
        if (expr->tag == SEARCH_FILTER_MODIFIED_SINCE) {
            if (expr->value_time > mpd_song_get_last_modified(song)) {
                return false;
            }
        }
        else if (expr->tag == SEARCH_FILTER_ADDED_SINCE) {
            if (expr->value_time > mpd_song_get_added(song)) {
                return false;
            }
        }
        else {
            one_tag.tags[0] = (enum mpd_tag_type)expr->tag;
            const struct t_tags *tags = expr->tag == SEARCH_FILTER_ANY_TAG
                ? tag_types  //any - use all browse tags
                : &one_tag;  //use only selected tag

            bool rc = false;
            for (size_t i = 0; i < tags->tags_len; i++) {
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
 * Private functions
 */

/**
 * Frees the t_search_expression struct
 * @param expr pointer to t_search_expression struct
 */
void *free_search_expression(struct t_search_expression *expr) {
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
        MYMPD_LOG_ERROR(NULL, "PCRE2 compilation failed at offset %d: \"%s\"", (int)erroroffset, buffer);
        return NULL;
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
