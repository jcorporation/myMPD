/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Json query implementation
 */

#ifndef MYMPD_JSON_QUERY_H
#define MYMPD_JSON_QUERY_H

#include "dist/sds/sds.h"
#include "src/lib/fields.h"
#include "src/lib/list.h"
#include "src/lib/validate.h"

#include <stdbool.h>

/**
 * JSON value types
 */
enum json_vtype {
    JSON_TOK_INVALID,
    JSON_TOK_STRING,
    JSON_TOK_NUMBER,
    JSON_TOK_TRUE,
    JSON_TOK_FALSE,
    JSON_TOK_NULL,
    JSON_TOK_ARRAY,
    JSON_TOK_OBJECT
};

/**
 * Struct that holds the validation errors for the json_get_* functions
 */
struct t_json_parse_error {
    sds message;  //!< the error message
    sds path;     //!< the json path of the invalid value
};

/**
 * Iteration callback definition
 */
typedef bool (*iterate_callback) (const char *, sds, sds, enum json_vtype, validate_callback, void *, struct t_json_parse_error *);

void json_parse_error_init(struct t_json_parse_error *parse_error);
void json_parse_error_clear(struct t_json_parse_error *parse_error);

bool json_get_bool(sds s, const char *path, bool *result, struct t_json_parse_error *error);
bool json_get_int_max(sds s, const char *path, int *result, struct t_json_parse_error *error);
bool json_get_int(sds s, const char *path, int min, int max, int *result, struct t_json_parse_error *error);
bool json_get_time_max(sds s, const char *path, time_t *result, struct t_json_parse_error *error);
bool json_get_int64_max(sds s, const char *path, int64_t *result, struct t_json_parse_error *error);
bool json_get_int64(sds s, const char *path, int64_t min, int64_t max, int64_t *result, struct t_json_parse_error *error);
bool json_get_uint_max(sds s, const char *path, unsigned *result, struct t_json_parse_error *error);
bool json_get_uint(sds s, const char *path, unsigned min, unsigned max, unsigned *result, struct t_json_parse_error *error);
bool json_get_string_max(sds s, const char *path, sds *result, validate_callback vcb, struct t_json_parse_error *error);
bool json_get_string(sds s, const char *path, size_t min, size_t max, sds *result, validate_callback vcb, struct t_json_parse_error *error);
bool json_get_string_cmp(sds s, const char *path, size_t min, size_t max, const char *cmp, sds *result, struct t_json_parse_error *error);
bool json_get_array_string(sds s, const char *path, struct t_list *l, validate_callback vcb, int max_elements, struct t_json_parse_error *error);
bool json_get_array_int64(sds s, const char *path, struct t_list *l, int max_elements, struct t_json_parse_error *error);
bool json_get_object_string(sds s, const char *path, struct t_list *l, validate_callback vcb_key,
    validate_callback vcb_value, int max_elements, struct t_json_parse_error *error);
bool json_iterate_object(sds s, const char *path, iterate_callback icb, void *icb_userdata,
    validate_callback vcb_key, validate_callback vcb_value, int max_elements, struct t_json_parse_error *error);
bool json_get_fields(sds s, const char *path, struct t_fields *tags, int max_elements, struct t_json_parse_error *error);
bool json_get_tag_values(sds s, const char *path, struct mpd_song *song, validate_callback vcb, int max_elements, struct t_json_parse_error *error);

bool json_find_key(sds s, const char *path);
sds json_get_key_as_sds(sds s, const char *path);

const char *get_mjson_toktype_name(enum json_vtype vtype);
sds list_to_json_array(sds s, struct t_list *l);
bool json_get_fields_as_string(sds s, sds *fields, struct t_json_parse_error *error);

#endif
