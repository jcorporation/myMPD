/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Jsonrpc implementation
 */

#include "compile_time.h"
#include "src/lib/json/json_query.h"

#include "dist/mongoose/mongoose.h"
#include "src/lib/convert.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/sds_extras.h"
#include "src/lib/sticker.h"
#include "src/mympd_client/tags.h"

#include <string.h>

/**
 * This unit provides functions for json parsing
 * Json parsing is done by mjson
 */

/**
 * private definitions
 */
static enum json_vtype get_vtype(char p);
static bool icb_json_get_field(const char *path, sds key, sds value, enum json_vtype vtype, validate_callback vcb, void *userdata, struct t_json_parse_error *error);
static bool icb_json_get_tag_value(const char *path, sds key, sds value, enum json_vtype vtype, validate_callback vcb, void *userdata, struct t_json_parse_error *error);
static bool json_get_string_unescape(sds s, const char *path, size_t min, size_t max, sds *result, validate_callback vcb, struct t_json_parse_error *error);
static void set_parse_error(struct t_json_parse_error *error, const char *path, const char *key, const char *fmt, ...);

/**
 * public functions
 */

/**
 * Initializes the sds strings t_json_parse_error struct to NULL
 * @param parse_error t_json_parse_error struct to initialize
 */
void json_parse_error_init(struct t_json_parse_error *parse_error) {
    parse_error->message = NULL;
    parse_error->path = NULL;
}

/**
 * Clears the sds strings
 * @param parse_error t_json_parse_error struct to clear
 */
void json_parse_error_clear(struct t_json_parse_error *parse_error) {
    FREE_SDS(parse_error->message);
    FREE_SDS(parse_error->path);
}

/**
 * Helper function to get myMPD fields out of a jsonrpc request
 * and return a validated json array
 * @param s sds string to parse
 * @param fields sds string to append the fields
 * @param error pointer to t_json_parse_error
 * @return true on success, else false
 */
bool json_get_fields_as_string(sds s, sds *fields, struct t_json_parse_error *error) {
    struct t_list col_list;
    list_init(&col_list);
    bool rc = json_get_array_string(s, "$.params.fields", &col_list, vcb_isfield, 20, error);
    if (rc == true) {
        *fields = list_to_json_array(*fields, &col_list);
    }
    list_clear(&col_list);
    return rc;
}

/**
 * Gets a bool value by json path
 * @param s json object to parse
 * @param path mjson path expression
 * @param result pointer to bool with the result
 * @param error pointer to t_json_parse_error
 * @return true on success, else false
 */
bool json_get_bool(sds s, const char *path, bool *result, struct t_json_parse_error *error) {
    bool v = false;
    if (mg_json_get_bool(mg_str_n(s, sdslen(s)), path, &v) == true) {
        *result = v;
        return true;
    }
    set_parse_error(error, path, "", "JSON path \"%s\" not found", path);
    return false;
}

/**
 * Gets a int value by json path
 * @param s json object to parse
 * @param path mjson path expression
 * @param result pointer to int with the result
 * @param error pointer to t_json_parse_error
 * @return true on success, else false
 */
bool json_get_int_max(sds s, const char *path, int *result, struct t_json_parse_error *error) {
    return json_get_int(s, path, JSONRPC_INT_MIN, JSONRPC_INT_MAX, result, error);
}

/**
 * Gets a int value by json path
 * @param s json object to parse
 * @param path mjson path expression
 * @param min minimum value (including)
 * @param max maximum value (including)
 * @param result pointer to int with the result
 * @param error pointer to t_json_parse_error
 * @return true on success, else false
 */
bool json_get_int(sds s, const char *path, int min, int max, int *result, struct t_json_parse_error *error) {
    double value;
    if (mg_json_get_num(mg_str_n(s, sdslen(s)), path, &value) == true) {
        if (value >= JSONRPC_INT_MIN &&
            value <= JSONRPC_INT_MAX)
        {
            int value_int = (int)value;
            if (value_int >= min &&
                value_int <= max)
            {
                *result = value_int;
                return true;
            }
        }
        set_parse_error(error, path, "", "Number is out of valid range");
    }
    else {
        set_parse_error(error, path, "", "JSON path \"%s\" not found", path);
    }
    return false;
}

/**
 * Gets a time_t value by json path
 * @param s json object to parse
 * @param path mjson path expression
 * @param result pointer to time_t with the result
 * @param error pointer to t_json_parse_error
 * @return true on success, else false
 */
bool json_get_time_max(sds s, const char *path, time_t *result, struct t_json_parse_error *error) {
    double value;
    if (mg_json_get_num(mg_str_n(s, sdslen(s)), path, &value) == true) {
        if (value >= JSONRPC_TIME_MIN &&
            value <= JSONRPC_TIME_MAX)
        {
            time_t value_time = (time_t)value;
            *result = value_time;
            return true;
        }
        set_parse_error(error, path, "", "Number is out of valid range");
    }
    else {
        set_parse_error(error, path, "", "JSON path \"%s\" not found", path);
    }
    return false;
}

/**
 * Gets an int64_t value by json path
 * @param s json object to parse
 * @param path mjson path expression
 * @param result pointer to int64_t with the result
 * @param error pointer to t_json_parse_error
 * @return true on success, else false
 */
bool json_get_int64_max(sds s, const char *path, int64_t *result, struct t_json_parse_error *error) {
    return json_get_int64(s, path, JSONRPC_INT64_MIN, JSONRPC_INT64_MAX, result, error);
}

/**
 * Gets an int64_t value by json path
 * @param s json object to parse
 * @param path mjson path expression
 * @param min minimum value (including)
 * @param max maximum value (including)
 * @param result pointer to int64_t with the result
 * @param error pointer to t_json_parse_error
 * @return true on success, else false
 */
bool json_get_int64(sds s, const char *path, int64_t min, int64_t max, int64_t *result, struct t_json_parse_error *error) {
    double value;
    if (mg_json_get_num(mg_str_n(s, sdslen(s)), path, &value) == true) {
        if (value >= (double)JSONRPC_INT64_MIN &&
            value <= (double)JSONRPC_INT64_MAX)
        {
            int64_t value_int64 = (int64_t)value;
            if (value_int64 >= min && value_int64 <= max) {
                *result = value_int64;
                return true;
            }
        }
        set_parse_error(error, path, "", "Number is out of valid range");
    }
    else {
        set_parse_error(error, path, "", "JSON path \"%s\" not found", path);
    }
    return false;
}

/**
 * Gets a unsigned int value by json path
 * @param s json object to parse
 * @param path mjson path expression
 * @param result pointer to unsigned int with the result
 * @param error pointer to t_json_parse_error
 * @return true on success, else false
 */
bool json_get_uint_max(sds s, const char *path, unsigned *result, struct t_json_parse_error *error) {
    return json_get_uint(s, path, 0, JSONRPC_INT_MAX, result, error);
}

/**
 * Gets a unsigned int value by json path
 * @param s json object to parse
 * @param path mjson path expression
 * @param min minimum value (including)
 * @param max maximum value (including)
 * @param result pointer to unsigned int with the result
 * @param error pointer to t_json_parse_error
 * @return true on success, else false
 */
bool json_get_uint(sds s, const char *path, unsigned min, unsigned max, unsigned *result, struct t_json_parse_error *error) {
    double value;
    if (mg_json_get_num(mg_str_n(s, sdslen(s)), path, &value) == true) {
        if (value >= JSONRPC_UINT_MIN &&
            value <= JSONRPC_UINT_MAX)
        {
            if (value >= min &&
                value <= max)
            {
                *result = (unsigned)value;
                return true;
            }
        }
        set_parse_error(error, path, "", "Number is out of valid range");
    }
    else {
        set_parse_error(error, path, "", "JSON path \"%s\" not found", path);
    }
    return false;
}

/**
 * Gets a string value by json path
 * @param s json object to parse
 * @param path mjson path expression
 * @param result pointer to sds with the result
 * @param vcb validation callback
 * @param error pointer to t_json_parse_error
 * @return true on success, else false
 */
bool json_get_string_max(sds s, const char *path, sds *result, validate_callback vcb, struct t_json_parse_error *error) {
    if (vcb == NULL) {
        set_parse_error(error, path, "", "Validation callback is NULL");
        return false;
    }
    return json_get_string_unescape(s, path, 0, JSONRPC_STR_MAX, result, vcb, error);
}

/**
 * Gets a string value by json path
 * @param s json object to parse
 * @param path mjson path expression
 * @param result pointer to sds with the result
 * @param min minimum length (inclusive)
 * @param max maximum length (inclusive)
 * @param cmp compare result against this string
 * @param error pointer to t_json_parse_error
 * @return true on success, else false
 */
bool json_get_string_cmp(sds s, const char *path, size_t min, size_t max, const char *cmp, sds *result, struct t_json_parse_error *error) {
    if (json_get_string_unescape(s, path, min, max, result, NULL, error) == false) {
        return false;
    }
    if (strcmp(*result, cmp) != 0) {
        sdsclear(*result);
        set_parse_error(error, path, "", "Value of JSON path \"%s\" is not equal \"%s\"", path, cmp);
        return false;
    }
    return true;
}

/**
 * Gets a string value by json path
 * @param s json object to parse
 * @param path mjson path expression
 * @param result pointer to int with the result
 * @param min minimum length (inclusive)
 * @param max maximum length (inclusive)
 * @param vcb validation callback
 * @param error pointer to t_json_parse_error
 * @return true on success, else false
 */
bool json_get_string(sds s, const char *path, size_t min, size_t max, sds *result, validate_callback vcb, struct t_json_parse_error *error) {
    if (vcb == NULL) {
        set_parse_error(error, path, "", "Validation callback is NULL");
        return false;
    }
    return json_get_string_unescape(s, path, min, max, result, vcb, error);
}

/**
 * Iterates through object/array found by path
 * @param s json object to parse
 * @param path mjson path expression
 * @param icb iteration callback
 * @param icb_userdata custom data for iteration callback
 * @param vcb_key validation callback for the key
 * @param vcb_value validation callback for the value
 * @param max_elements maximum of elements
 * @param error pointer to t_json_parse_error
 * @return true on success, else false
 */
bool json_iterate_object(sds s, const char *path, iterate_callback icb, void *icb_userdata,
        validate_callback vcb_key, validate_callback vcb_value, int max_elements, struct t_json_parse_error *error)
{
    if (icb == NULL) {
        set_parse_error(error, path, "", "Iteration callback is NULL");
        return false;
    }
    if (vcb_key == NULL) {
        vcb_key = vcb_isalnum;
    }
    struct mg_str obj = mg_json_get_tok(mg_str_n(s, sdslen(s)), path);
    if (obj.buf == NULL) {
        set_parse_error(error, path, "", "JSON path \"%s\" not found", path);
        return false;
    }
    if (obj.buf[0] != '{' && obj.buf[0] != '[') {
        set_parse_error(error, path, "", "Invalid json object type for JSON path \"%s\"", path);
        return false;
    }
    //iterable object/array
    sds value = sdsempty();
    sds key = sdsempty();
    int i = 0;
    struct mg_str j_key;
    struct mg_str j_value;
    enum json_vtype vtype = JSON_TOK_INVALID;
    size_t off = 0;
    while ((off = mg_json_next(obj, off, &j_key, &j_value)) != 0) {
        if (j_key.buf != NULL &&
            j_key.len > 2)
        {
            key = sdscatlen(key, j_key.buf, j_key.len);
            if (j_key.len > JSONRPC_KEY_MAX) {
                set_parse_error(error, path, key, "Key in JSON path \"%s\" is too long", path);
                FREE_SDS(value);
                FREE_SDS(key);
                return false;
            }
            sdsclear(key);
            if (sds_json_unescape(j_key.buf +  1, j_key.len - 2, &key) == false ||
                vcb_key(key) == false)
            {
                set_parse_error(error, path, key, "Validation of key in path \"%s\" has failed.", path);
                FREE_SDS(value);
                FREE_SDS(key);
                return false;
            }
        }
        if (j_value.buf == NULL ||
            j_value.len == 0)
        {
            set_parse_error(error, path, key, "Value for key \"%s\" in JSON path \"%s\" is not set", key, path);
            FREE_SDS(value);
            FREE_SDS(key);
            return false;
        }
        if (j_value.len > JSONRPC_STR_MAX) {
            set_parse_error(error, path, key, "Value for key \"%s\" in JSON path \"%s\" is too long", key, path);
            FREE_SDS(value);
            FREE_SDS(key);
            return false;
        }
        vtype = get_vtype(j_value.buf[0]);
        switch(vtype) {
            case JSON_TOK_STRING:
                if (j_value.len > 2) {
                    if (sds_json_unescape(j_value.buf + 1, j_value.len - 2, &value) == false) {
                        set_parse_error(error, path, key, "JSON unescape error for value for key \"%s\" in JSON path \"%s\" has failed", key, path);
                        FREE_SDS(value);
                        FREE_SDS(key);
                        return false;
                    }
                }
                break;
            case JSON_TOK_INVALID:
            case JSON_TOK_NULL:
                set_parse_error(error, path, key, "Invalid json value type: %s", get_mjson_toktype_name(vtype));
                FREE_SDS(value);
                FREE_SDS(key);
                return false;

            default:
                value = sdscatlen(value, j_value.buf, j_value.len);
                break;
        }
        if (sdslen(key) == 0) {
            //array - fallback to parent key
            const char *key_ptr = strrchr(path, '.');
            if (key_ptr != NULL) {
                key = sdscat(key, key_ptr + 1);
            }
        }
        if (icb(path, key, value, vtype, vcb_value, icb_userdata, error) == false) {
            MYMPD_LOG_WARN(NULL, "Iteration callback for path \"%s\" has failed", path);
            FREE_SDS(value);
            FREE_SDS(key);
            return false;
        }

        sdsclear(value);
        sdsclear(key);
        i++;

        if (i == max_elements) {
            break;
        }
    }
    FREE_SDS(value);
    FREE_SDS(key);
    return true;
}

/**
 * Iteration callback to populate mpd_song tag values
 * @param path json path
 * @param key json key
 * @param value json value
 * @param vtype mjson value type
 * @param vcb validation callback
 * @param userdata pointer to a t_list struct to populate
 * @param error pointer to t_json_parse_error
 * @return true on success, else false
 */
static bool icb_json_get_tag_value(const char *path, sds key, sds value, enum json_vtype vtype, validate_callback vcb, void *userdata, struct t_json_parse_error *error) {
    enum mpd_tag_type tag = mpd_tag_name_parse(key);
    if (tag == MPD_TAG_UNKNOWN) {
        set_parse_error(error, path, "", "Unknown mpd tag type \"%s\"", key);
        return false;
    }
    if (vtype != JSON_TOK_STRING) {
        set_parse_error(error, path, "", "Invalid type for tag \"%s\": %s", key, get_mjson_toktype_name(vtype));
        return false;
    }
    if (vcb != NULL && vcb(value) == false) {
        set_parse_error(error, path, "", "Validation of value \"%s\" has failed", value);
        return false;
    }
    mympd_mpd_song_add_tag_dedup((struct mpd_song *)userdata, tag, value);
    return true;
}

/**
 * Converts a json array to a mpd song tag value(s)
 * Shortcut for json_iterate_object with icb_json_get_tag_value
 * @param s json object to parse
 * @param path mjson path expression
 * @param song mpd_song struct
 * @param vcb validation callback
 * @param max_elements maximum of elements
 * @param error pointer to t_json_parse_error
 * @return true on success, else false
 */
bool json_get_tag_values(sds s, const char *path, struct mpd_song *song, validate_callback vcb, int max_elements, struct t_json_parse_error *error) {
    return json_iterate_object(s, path, icb_json_get_tag_value, song, vcb, vcb, max_elements, error);
}

/**
 * Iteration callback to populate a list with json array of strings
 * @param path json path
 * @param key json key
 * @param value json value
 * @param vtype mjson value type
 * @param vcb validation callback
 * @param userdata pointer to a t_list struct to populate
 * @param error pointer to t_json_parse_error
 * @return true on success, else false
 */
static bool icb_json_get_array_string(const char *path, sds key, sds value, enum json_vtype vtype, validate_callback vcb, void *userdata, struct t_json_parse_error *error) {
    (void)key;
    if (vtype != JSON_TOK_STRING ||
        vcb(value) == false)
    {
        set_parse_error(error, path, "", "Validation of value \"%s\" has failed", value);
        return false;
    }
    struct t_list *l = (struct t_list *)userdata;
    list_push(l, value, 0, NULL, NULL);
    return true;
}

/**
 * Converts a json array of strings to a t_list struct
 * Shortcut for json_iterate_object with icb_json_get_array_string
 * @param s json object to parse
 * @param path mjson path expression
 * @param l t_list struct to populate
 * @param vcb validation callback
 * @param max_elements maximum of elements
 * @param error pointer to t_json_parse_error
 * @return true on success, else false
 */
bool json_get_array_string(sds s, const char *path, struct t_list *l, validate_callback vcb, int max_elements, struct t_json_parse_error *error) {
    return json_iterate_object(s, path, icb_json_get_array_string, l, vcb, vcb, max_elements, error);
}

/**
 * Iteration callback to populate a list with json array of int64_t
 * @param path json path
 * @param key json key
 * @param value json value
 * @param vtype mjson value type
 * @param vcb validation callback - not used
 * @param userdata pointer to a t_list struct to populate
 * @param error pointer to t_json_parse_error
 * @return true on success, else false
 */
static bool icb_json_get_array_int64(const char *path, sds key, sds value, enum json_vtype vtype, validate_callback vcb, void *userdata, struct t_json_parse_error *error) {
    (void)key;
    (void)vcb;
    if (vtype != JSON_TOK_NUMBER) {
        set_parse_error(error, path, "", "Validation of value \"%s\" has failed", value);
        return false;
    }
    int64_t value_int64; 
    enum str2int_errno rc = str2int64(&value_int64, value);
    if (rc != STR2INT_SUCCESS) {
        return false;
    }
    struct t_list *l = (struct t_list *)userdata;
    list_push(l, "", value_int64, NULL, NULL);
    return true;
}

/**
 * Converts a json array of int64_t to a t_list struct
 * Shortcut for json_iterate_object with icb_json_get_array_int64
 * @param s json object to parse
 * @param path mjson path expression
 * @param l t_list struct to populate
 * @param max_elements maximum of elements
 * @param error pointer to t_json_parse_error
 * @return true on success, else false
 */
bool json_get_array_int64(sds s, const char *path, struct t_list *l, int max_elements, struct t_json_parse_error *error) {
    return json_iterate_object(s, path, icb_json_get_array_int64, l, NULL, NULL, max_elements, error);
}

/**
 * Iteration callback to populate a list with json object key/values
 * @param path json path
 * @param key json key
 * @param value json value
 * @param vtype mjson value type
 * @param vcb validation callback
 * @param userdata pointer to a t_list struct to populate
 * @param error pointer to t_json_parse_error
 * @return true on success, else false
 */
static bool icb_json_get_object_string(const char *path, sds key, sds value, enum json_vtype vtype, validate_callback vcb, void *userdata, struct t_json_parse_error *error) {
    if (sdslen(key) == 0 ||
        vtype != JSON_TOK_STRING ||
        vcb(value) == false)
    {
        set_parse_error(error, path, key, "Validation of key \"%s\" with value \"%s\" has failed", key, value);
        return false;
    }
    struct t_list *l = (struct t_list *)userdata;
    list_push(l, key, 0, value, NULL);
    return true;
}

/**
 * Converts a json object key/values to a t_list struct
 * Shortcut for json_iterate_object with icb_json_get_object_string
 * @param s json object to parse
 * @param path mjson path expression
 * @param l t_list struct to populate
 * @param vcb_key validation callback for key
 * @param vcb_value validation callback for value
 * @param max_elements maximum of elements
 * @param error pointer to t_json_parse_error
 * @return true on success, else false
 */
bool json_get_object_string(sds s, const char *path, struct t_list *l, validate_callback vcb_key,
    validate_callback vcb_value, int max_elements, struct t_json_parse_error *error)
{
    return json_iterate_object(s, path, icb_json_get_object_string, l, vcb_key, vcb_value, max_elements, error);
}

/**
 * Converts a json array to a struct t_tags
 * Shortcut for json_iterate_object with icb_json_get_tag
 * @param s json object to parse
 * @param path mjson path expression
 * @param tags t_tags struct to populate
 * @param max_elements maximum of elements
 * @param error pointer to t_json_parse_error
 * @return true on success, else false
 */
bool json_get_fields(sds s, const char *path, struct t_fields *tags, int max_elements, struct t_json_parse_error *error) {
    return json_iterate_object(s, path, icb_json_get_field, tags, NULL, NULL, max_elements, error);
}

/**
 * Searches for a key in json object
 * @param s json object to search
 * @param path mjson path expression
 * @return true on success, else false
 */
bool json_find_key(sds s, const char *path) {
    int off = mg_json_get(mg_str_n(s, sdslen(s)), path, NULL);
    return off < 0
        ? false
        : true;
}

/**
 * Searches for a key in json object and returns its value as sds string
 * @param s json object to search
 * @param path mjson path expression
 * @return Key value as sds or NULL on error
 */
sds json_get_key_as_sds(sds s, const char *path) {
    struct mg_str val = mg_json_get_tok(mg_str_n(s, sdslen(s)), path);
    if (val.buf == NULL) {
        return NULL;
    }
    return sdsnewlen(val.buf, val.len);
}

/**
 * Returns the name of a mjson token type
 * @param vtype token type
 * @return token type as string
 */
const char *get_mjson_toktype_name(enum json_vtype vtype) {
    switch(vtype) {
        case JSON_TOK_INVALID: return "JSON_TOK_INVALID";
        case JSON_TOK_STRING:  return "JSON_TOK_STRING";
        case JSON_TOK_NUMBER:  return "JSON_TOK_NUMBER";
        case JSON_TOK_TRUE:    return "JSON_TOK_TRUE";
        case JSON_TOK_FALSE:   return "JSON_TOK_FALSE";
        case JSON_TOK_NULL:    return "JSON_TOK_NULL";
        case JSON_TOK_ARRAY:   return "JSON_TOK_ARRAY";
        case JSON_TOK_OBJECT:  return "JSON_TOK_OBJECT";
        default:               return "JSON_TOK_UNKNOWN";
    }
}

//private functions

/**
 * Iteration callback to populate a t_fields struct
 * @param path json path
 * @param key not used
 * @param value value to parse as mpd tag
 * @param vtype mjson value type
 * @param vcb not used - we validate directly
 * @param userdata void pointer to t_fields struct
 * @param error pointer to t_json_parse_error
 * @return true on success, else false
 */
static bool icb_json_get_field(const char *path, sds key, sds value, enum json_vtype vtype, validate_callback vcb, void *userdata, struct t_json_parse_error *error) {
    (void) key;
    (void) vcb;
    if (vtype != JSON_TOK_STRING) {
        set_parse_error(error, path, key, "Value is not a string \"%s\"", value);
        return false;
    }

    struct t_fields *fields = (struct t_fields *) userdata;
    enum mpd_tag_type mpd_tag = mpd_tag_name_iparse(value);
    if (mpd_tag != MPD_TAG_UNKNOWN) {
        fields->mpd_tags.tags[fields->mpd_tags.len++] = mpd_tag;
        return true;
    }
    enum mympd_sticker_names sticker = sticker_name_parse(value);
    if (sticker != STICKER_UNKNOWN) {
        fields->stickers.stickers[fields->stickers.len++] = sticker;
        return true;
    }
    if (strcmp(value, "userDefinedSticker") == 0) {
        fields->stickers.user_defined = true;
    }
    return true;
}

/**
 * Helper function to set parsing errors
 * @param error already initialized t_json_parse_error struct to propagate the error
 *              or NULL for log only
 * @param path json path where the error occurred
 * @param key optional json key inside the path where the error occurred
 *            set this to a blank string for none object paths
 * @param fmt printf format string
 * @param ... arguments for the format string
 */
static void set_parse_error(struct t_json_parse_error *error, const char *path, const char *key, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (error != NULL) {
        error->message = sdscatvprintf(sdsempty(), fmt, args); // NOLINT(clang-diagnostic-format-nonliteral)
        error->path = key[0] != '\0'
            ? sdscatfmt(sdsempty(), "%s.%s", path, key)
            : sdsnew(path);
        MYMPD_LOG_WARN(NULL, "%s: %s", error->path, error->message);
    }
    else {
        sds e = sdscatvprintf(sdsempty(), fmt, args); // NOLINT(clang-diagnostic-format-nonliteral)
        MYMPD_LOG_WARN(NULL, "%s: %s", path, e);
        FREE_SDS(e);
    }
    va_end(args);
}

/**
 * Helper function to get a string from a json object
 * Enclosing quotes are removed and string is unescaped
 * @param s json object to parse
 * @param path path to the string to extract
 * @param min minimum length
 * @param max maximum length
 * @param result newly allocated sds string with the result
 * @param vcb validation callback
 * @param error pointer to t_json_parse_error
 * @return true on success, else false
 */
static bool json_get_string_unescape(sds s, const char *path, size_t min, size_t max, sds *result, validate_callback vcb, struct t_json_parse_error *error) {
    if (*result != NULL) {
        MYMPD_LOG_ERROR(NULL, "Result parameter must be NULL, path: \"%s\"", path);
        return false;
    }

    char *str = mg_json_get_str(mg_str_n(s, sdslen(s)), path);
    if (str == NULL) {
        *result = NULL;
        set_parse_error(error, path, "", "JSON path \"%s\" not found or value is not string", path);
        return false;
    }

    *result = sdsnew(str);
    size_t n = sdslen(*result);
    FREE_PTR(str);
    if (n == 0) {
        //empty string
        if (min == 0) {
            return true;
        }
        set_parse_error(error, path, "", "Value length for JSON path \"%s\" is too short", path);
        FREE_SDS(*result);
        return false;
    }
    if (n > max) {
        set_parse_error(error, path, "", "Value length for JSON path \"%s\" is too long", path);
        FREE_SDS(*result);
        return false;
    }

    if (vcb != NULL) {
        if (vcb(*result) == false) {
            set_parse_error(error, path, "", "Validation of value for JSON path \"%s\" has failed", path);
            FREE_SDS(*result);
            return false;
        }
    }

    return true;
}

/**
 * Detects the json value type by first char of value
 * @param p First char of value
 * @return bool 
 */
static enum json_vtype get_vtype(char p) {
    switch(p) {
        case '{': return JSON_TOK_OBJECT;
        case '[': return JSON_TOK_ARRAY;
        case '"': return JSON_TOK_STRING;
        case 'f':
        case 'F':
            return JSON_TOK_FALSE;
        case 't':
        case 'T':
            return JSON_TOK_TRUE;
        case 'n':
        case 'N':
            return JSON_TOK_NULL;
        default:
            if (p == '-' || ((p >= '0' && p <= '9'))) {
                return JSON_TOK_NUMBER;
            }
    }
    return JSON_TOK_INVALID;
}
