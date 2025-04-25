/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Variables API functions
 */

#include "compile_time.h"
#include "src/scripts/api_tmp.h"

#include "src/lib/json/json_print.h"
#include "src/lib/json/json_rpc.h"
#include "src/lib/mem.h"
#include "src/lib/sds_extras.h"
#include "src/scripts/util.h"

#include <errno.h>
#include <time.h>

/**
 * Deletes a tmp variable in the tree, ignores not existing keys
 * @param scripts_tmp_list pointer to script tmp list
 * @param key key to remove
 */
void scripts_tmp_delete(rax *scripts_tmp_list, sds key) {
    void *value;
    if (raxRemove(scripts_tmp_list, (unsigned char *)key, sdslen(key), &value) == 1) {
        struct t_tmpvar_data *data = (struct t_tmpvar_data *)value;
        FREE_SDS(data->value);
        FREE_PTR(data);
    }
}

/**
 * Adds/modifies a tmp variable in the tree
 * @param scripts_tmp_list pointer to script tmp list
 * @param key name of the variable
 * @param value value of the variable
 * @param lifetime lifetime of the tmp variable
 * @return true on success, else false
 */
bool scripts_tmp_set(rax *scripts_tmp_list, sds key, sds value, int lifetime) {
    void *old_data;
    errno = 0;
    struct t_tmpvar_data *data = malloc_assert(sizeof(struct t_tmpvar_data));
    data->value = sdsdup(value);
    if (lifetime > 0) {
        data->expires = time(NULL) + lifetime;
    }
    else {
        data->expires = lifetime;
    }
    if (raxInsert(scripts_tmp_list, (unsigned char *)key, sdslen(key), data, &old_data) == 0) {
        struct t_tmpvar_data *old_tmpvar = (struct t_tmpvar_data *)old_data;
        FREE_SDS(old_tmpvar->value);
        FREE_PTR(old_tmpvar);
    }
    return errno == 0 
        ? true
        : false;
}

/**
 * Returns a jsonrpc response with the value of a tmp variable or an empty string if not found
 * @param scripts_tmp_list pointer to script tmp list
 * @param buffer buffer to append the response
 * @param request_id jsonrpc request id
 * @param key Key to get
 * @return pointer to buffer
 */
sds scripts_tmp_get(rax *scripts_tmp_list, sds buffer, unsigned request_id, sds key) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_SCRIPT_TMP_GET;
    void *value;
    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    if (raxFind(scripts_tmp_list, (unsigned char *)key, sdslen(key), &value) == 1) {
        struct t_tmpvar_data *data = (struct t_tmpvar_data *)value;
        buffer = tojson_sds(buffer, "value", data->value, true);
        buffer = tojson_int64(buffer, "expires", data->expires, false);
        if (data->expires == 0) {
            scripts_tmp_delete(scripts_tmp_list, key);
        }
    }
    else {
        buffer = tojson_char(buffer, "value", "", true);
        buffer = tojson_int(buffer, "expires", 0, false);
    }
    buffer = jsonrpc_end(buffer);
    return buffer;
}

/**
 * Returns a jsonrpc response with all script tmp variables
 * @param scripts_tmp_list pointer to script tmp list
 * @param buffer buffer to append the response
 * @param request_id jsonrpc request id
 * @return pointer to buffer
 */
sds scripts_tmp_list(rax *scripts_tmp_list, sds buffer, unsigned request_id) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_SCRIPT_VAR_LIST;
    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    raxIterator iter;
    raxStart(&iter, scripts_tmp_list);
    raxSeek(&iter, "^", NULL, 0);
    unsigned returned_entities = 0;
    while (raxNext(&iter)) {
        if (returned_entities++) {
            buffer = sdscatlen(buffer, ",", 1);
        }
        struct t_tmpvar_data *data = (struct t_tmpvar_data *)iter.data;
        buffer = sdscatlen(buffer, "{", 1);
        buffer = tojson_char_len(buffer, "key", (const char *)iter.key, iter.key_len, true);
        buffer = tojson_sds(buffer, "value", data->value, true);
        buffer = tojson_int64(buffer, "expires", data->expires, false);
        buffer = sdscatlen(buffer, "}", 1);
    }
    raxStop(&iter);
    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_uint(buffer, "returnedEntities", returned_entities, true);
    buffer = tojson_uint(buffer, "totalEntities", returned_entities, false);
    buffer = jsonrpc_end(buffer);
    return buffer;
}

/**
 * Checks if the tmp var list should be expired
 * @param scripts_state Pointer to scripts_state
 */
void script_tmp_list_should_expire(struct t_scripts_state *scripts_state) {
    time_t now = time(NULL);
    if (scripts_state->tmp_list_next_exp <= now) {
        scripts_tmp_list_expire(scripts_state->tmp_list, false);
        scripts_state->tmp_list_next_exp = now + 60;
    }
}

/**
 * Expires the script tmp var list
 * @param scripts_tmp_list Pointer to script tmp list
 * @param cleanup Expire all entries and free the radix tree?
 */
void scripts_tmp_list_expire(rax *scripts_tmp_list, bool cleanup) {
    raxIterator iter;
    raxStart(&iter, scripts_tmp_list);
    raxSeek(&iter, "^", NULL, 0);
    int64_t now = time(NULL);
    while (raxNext(&iter)) {
        struct t_tmpvar_data *data = (struct t_tmpvar_data *)iter.data;
        if (cleanup == true ||
            (data->expires > 0 && data->expires <= now)
        ) {
            if (raxRemove(scripts_tmp_list, iter.key, iter.key_len, NULL) == 1) {
                FREE_SDS(data->value);
                FREE_PTR(data);
                raxSeek(&iter, ">", iter.key, iter.key_len);
            }
        }
    }
    raxStop(&iter);
    if (cleanup == true) {
        raxFree(scripts_tmp_list);
    }
}
