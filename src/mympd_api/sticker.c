/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD sticker API
 */

#include "compile_time.h"
#include "src/mympd_api/sticker.h"

#include "dist/utf8/utf8.h"
#include "src/lib/cache/cache_rax_album.h"
#include "src/lib/json/json_print.h"
#include "src/lib/json/json_rpc.h"
#include "src/lib/sds_extras.h"
#include "src/mympd_api/trigger.h"
#include "src/mympd_client/search.h"
#include "src/mympd_client/stickerdb.h"

/**
 * Gets a sticker value
 * @param stickerdb pointer to stickerdb
 * @param buffer already allocated sds string to append the result
 * @param request_id jsonrpc request id
 * @param uri Sticker uri
 * @param type MPD sticker type
 * @param name Sticker name
 * @return Pointer to buffer
 */
sds mympd_api_sticker_get(struct t_stickerdb_state *stickerdb, sds buffer, unsigned request_id,
    sds uri, enum mympd_sticker_type type, sds name)
{
    if (stickerdb->mpd_state->feat.stickers == false) {
        buffer = jsonrpc_respond_message(buffer, MYMPD_API_STICKER_GET, request_id,
            JSONRPC_FACILITY_STICKER, JSONRPC_SEVERITY_ERROR, "MPD stickers are disabled");
        return buffer;
    }
    sds value = stickerdb_get(stickerdb, type, uri, name);
    if (value == NULL) {
        return jsonrpc_respond_message(buffer, MYMPD_API_STICKER_GET, request_id,
            JSONRPC_FACILITY_STICKER, JSONRPC_SEVERITY_ERROR, "Sticker not found");
    }
    buffer = jsonrpc_respond_start(buffer, MYMPD_API_STICKER_GET, request_id);
    buffer = tojson_sds(buffer, "value", value, false);
    buffer = jsonrpc_end(buffer);
    FREE_SDS(value);
    return buffer;
}

/**
 * Gets a sorted list of stickers by name and value
 * @param stickerdb pointer to the stickerdb state
 * @param buffer already allocated sds string to append the list
 * @param request_id jsonrpc request id
 * @param uri baseuri for search
 * @param type MPD sticker type
 * @param name sticker name
 * @param op mpd sticker compare operator
 * @param value sticker value or NULL to get all stickers with this name
 * @param sort sticker sort type
 * @param sort_desc sort descending?
 * @param offset window start (including)
 * @param limit window end (excluding), use UINT_MAX for open end
 * @return pointer to buffer
 */
sds mympd_api_sticker_find(struct t_stickerdb_state *stickerdb, sds buffer, unsigned request_id,
        sds uri, enum mympd_sticker_type type, sds name, enum mpd_sticker_operator op, sds value,
        enum mpd_sticker_sort sort, bool sort_desc, unsigned offset, unsigned limit)
{
    if (stickerdb->mpd_state->feat.stickers == false) {
        buffer = jsonrpc_respond_message(buffer, MYMPD_API_STICKER_FIND, request_id,
            JSONRPC_FACILITY_STICKER, JSONRPC_SEVERITY_ERROR, "MPD stickers are disabled");
        return buffer;
    }
    struct t_list *result = stickerdb_find_stickers_sorted(stickerdb, type, uri, name, op, value, sort, sort_desc, offset, limit);
    if (result == NULL) {
        buffer = jsonrpc_respond_message(buffer, MYMPD_API_STICKER_FIND, request_id,
            JSONRPC_FACILITY_STICKER, JSONRPC_SEVERITY_ERROR, "Failure searching for stickers");
        return buffer;
    }
    buffer = jsonrpc_respond_start(buffer, MYMPD_API_STICKER_FIND, request_id);
    buffer = sdscat(buffer,"\"data\":[");
    unsigned entities_returned = 0;
    struct t_list_node *current;
    while ((current = list_shift_first(result)) != NULL) {
        if (entities_returned++) {
            buffer= sdscatlen(buffer, ",", 1);
        }
        buffer = sdscatlen(buffer, "{", 1);
        buffer = tojson_char(buffer, "uri", current->key, true);
        buffer = tojson_char(buffer, "name", name, true);
        buffer = tojson_char(buffer, "value", current->value_p, false);
        buffer = sdscatlen(buffer, "}", 1);
        list_node_free(current);
    }
    list_free(result);
    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_uint(buffer, "offset", offset, true);
    buffer = tojson_uint(buffer, "limit", limit, true);
    buffer = tojson_uint(buffer, "returnedEntities", entities_returned, true);
    buffer = tojson_int(buffer, "totalEntities", -1, false);
    buffer = jsonrpc_end(buffer);
    return buffer;
}

/**
 * Gets all MPD stickers for an uri.
 * @param stickerdb pointer to stickerdb
 * @param buffer already allocated sds string to append the list
 * @param request_id jsonrpc request id
 * @param uri Sticker uri
 * @param type MPD sticker type
 * @return Pointer to buffer
 */
sds mympd_api_sticker_list(struct t_stickerdb_state *stickerdb, sds buffer, unsigned request_id,
    sds uri, enum mympd_sticker_type type)
{
    if (stickerdb->mpd_state->feat.stickers == false) {
        buffer = jsonrpc_respond_message(buffer, MYMPD_API_STICKER_LIST, request_id,
            JSONRPC_FACILITY_STICKER, JSONRPC_SEVERITY_ERROR, "MPD stickers are disabled");
        return buffer;
    }
    struct t_stickers stickers;
    stickers_enable_all(&stickers, type);
    buffer = jsonrpc_respond_start(buffer, MYMPD_API_STICKER_LIST, request_id);
    buffer = tojson_char(buffer, "type", mympd_sticker_type_name_lookup(type), true);
    buffer = tojson_sds(buffer, "uri", uri, true);
    buffer = mympd_api_sticker_get_print(buffer, stickerdb, type, uri, &stickers);
    buffer = jsonrpc_end(buffer);
    return buffer;
}

/**
 * Sets the like sticker and triggers the feedback event
 * @param stickerdb pointer to stickerdb
 * @param trigger_list pointer to trigger list
 * @param partition_name the partition name
 * @param sticker_type MPD sticker type
 * @param uri uri to set the feedback
 * @param feedback_type feedback type
 * @param value feedback value to set
 * @param error already allocated sds string to append the error message
 * @return true on success, else false
 */
bool mympd_api_sticker_set_feedback(struct t_stickerdb_state *stickerdb, struct t_list *trigger_list, const char *partition_name,
    enum mympd_sticker_type sticker_type, sds uri, enum mympd_feedback_type feedback_type, int value, sds *error)
{
    //mympd_feedback trigger
    mympd_api_trigger_execute_feedback(trigger_list, uri, feedback_type, value, partition_name);

    if (stickerdb->mpd_state->feat.stickers == false) {
        *error = sdscat(*error, "MPD stickers are disabled");
        return false;
    }
    bool rc = feedback_type == FEEDBACK_LIKE
        ? value == 1
            ? stickerdb_remove(stickerdb, sticker_type, uri, "like")
            : stickerdb_set_like(stickerdb, sticker_type, uri, (enum sticker_like)value)
        : value == 0
            ? stickerdb_remove(stickerdb, sticker_type, uri, "rating")
            : stickerdb_set_rating(stickerdb, sticker_type, uri, value);
    if (rc == false) {
        *error = sdscat(*error, "Failed to set feedback");
        return false;
    }
    return true;
}

/**
 * Gets the stickers from stickerdb and returns a json list
 * Shortcut for stickerdb_get_all and print_sticker
 * @param buffer already allocated sds string to append the list
 * @param stickerdb pointer to stickerdb
 * @param type MPD sticker type
 * @param uri song uri
 * @param stickers list of stickers to print
 * @return pointer to the modified buffer
 */
sds mympd_api_sticker_get_print(sds buffer, struct t_stickerdb_state *stickerdb,
        enum mympd_sticker_type type, const char *uri, const struct t_stickers *stickers)
{
    if (stickers->len == 0 &&
        stickers->user_defined == false)
    {
        return buffer;
    }
    struct t_sticker sticker;
    if (stickerdb_get_all(stickerdb, type, uri, &sticker, stickers->user_defined) != NULL) {
        buffer = mympd_api_sticker_print(buffer, &sticker, stickers);
        sticker_struct_clear(&sticker);
    }
    return buffer;
}

/**
 * Gets the stickers from stickerdb and returns a json list.
 * Shortcut for stickerdb_get_all_batch and print_sticker.
 * You must exit the stickerdb idle mode before.
 * @param buffer already allocated sds string to append the list
 * @param stickerdb pointer to stickerdb
 * @param type MPD sticker type
 * @param uri song uri
 * @param stickers stickers to print
 * @return pointer to the modified buffer
 */
sds mympd_api_sticker_get_print_batch(sds buffer, struct t_stickerdb_state *stickerdb,
        enum mympd_sticker_type type, const char *uri, const struct t_stickers *stickers)
{
    if (stickers->len == 0 &&
        stickers->user_defined == false)
    {
        return buffer;
    }
    struct t_sticker sticker;
    if ((stickerdb_get_all_batch(stickerdb, type, uri, &sticker, stickers->user_defined)) != NULL) {
        buffer = mympd_api_sticker_print(buffer, &sticker, stickers);
        sticker_struct_clear(&sticker);
    }
    return buffer;
}

/**
 * Print the sticker struct as json list
 * @param buffer already allocated sds string to append the list
 * @param sticker pointer to sticker struct to print
 * @param stickers array of stickers to print
 * @return pointer to the modified buffer
 */
sds mympd_api_sticker_print(sds buffer, struct t_sticker *sticker, const struct t_stickers *stickers) {
    if (sticker == NULL) {
        return buffer;
    }
    buffer = json_comma(buffer);
    for (size_t i = 0; i < stickers->len; i++) {
        if (i > 0) {
            buffer = sdscatlen(buffer, ",", 1);
        }
        buffer = tojson_int64(buffer, sticker_name_lookup(stickers->stickers[i]), sticker->mympd[stickers->stickers[i]], false);
    }
    if (stickers->user_defined == true) {
        buffer = json_comma(buffer);
        buffer = sdscat(buffer, "\"sticker\":{");
        struct t_list_node *current = sticker->user.head;
        while (current != NULL) {
            buffer = tojson_char(buffer, current->key, current->value_p, false);
            current = current->next;
            if (current != NULL) {
                buffer = sdscatlen(buffer, ",", 1);
            }
        }
        buffer = sdscatlen(buffer, "}", 1);
    }
    return buffer;
}

/**
 * Print the sticker types as json array
 * @param stickerdb Pointer to stickerdb
 * @param buffer already allocated sds string to append the list
 * @return Pointer to buffer
 */
sds mympd_api_sticker_print_types(struct t_stickerdb_state *stickerdb, sds buffer) {
    struct t_list_node *current = stickerdb->mpd_state->sticker_types.head;
    while (current != NULL) {
        buffer = sds_catjson(buffer, current->key, sdslen(current->key));
        current = current->next;
        if (current != NULL) {
            buffer = sdscatlen(buffer, ",", 1);
        }
    }
    return buffer;
}

/**
 * Lists user defined sticker names by type
 * @param stickerdb Pointer to stickerdb
 * @param buffer Already allocated sds string to append the list
 * @param request_id jsonrpc request id
 * @param searchstr Search string
 * @param type MPD sticker type
 * @return Pointer to buffer
 */
sds mympd_api_sticker_names(struct t_stickerdb_state *stickerdb, sds buffer, unsigned request_id,
        sds searchstr, enum mympd_sticker_type type)
{
    struct t_list sticker_names;
    list_init(&sticker_names);
    if (stickerdb_get_names(stickerdb, type, &sticker_names) == false) {
        return jsonrpc_respond_message(buffer, MYMPD_API_STICKER_NAMES, request_id,
                JSONRPC_FACILITY_STICKER, JSONRPC_SEVERITY_ERROR, "Failure listing stickernames");
    }
    buffer = jsonrpc_respond_start(buffer, MYMPD_API_STICKER_NAMES, request_id);
    buffer = sdscat(buffer,"\"data\":[");
    struct t_list_node *current = sticker_names.head;
    unsigned entities_returned = 0;
    while (current != NULL) {
        if (sticker_name_parse(current->key) == STICKER_UNKNOWN &&
            (sdslen(searchstr) == 0 ||
             utf8casestr(current->key, searchstr) != NULL))
        {
            if (entities_returned++) {
                buffer= sdscatlen(buffer, ",", 1);
            }
            buffer = sds_catjson(buffer, current->key, sdslen(current->key));
        }
        current = current->next;
    }
    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_uint(buffer, "returnedEntities", entities_returned, true);
    buffer = tojson_uint(buffer, "totalEntities", entities_returned, false);
    buffer = jsonrpc_end(buffer);
    list_clear(&sticker_names);
    return buffer;
}

/**
 * Translates the uri and type for myMPD specific sticker type
 * @param mympd_state Pointer to mympd_state
 * @param uri Sticker URI
 * @param type Pointer to mympd_sticker_type
 * @return Pointer to uri
 */
sds mympd_api_get_sticker_uri(struct t_mympd_state *mympd_state, sds uri, enum mympd_sticker_type *type) {
    if (*type != STICKER_TYPE_MYMPD_ALBUM) {
        return uri;
    }

    struct t_album *album = album_cache_get_album(&mympd_state->album_cache, uri);
    if (album != NULL) {
        *type = STICKER_TYPE_FILTER;
        return get_search_expression_album(uri, mympd_state->mpd_state->tag_albumartist, album, &mympd_state->config->albums);
    }
    return uri;
}

/**
 * Translates myMPD specific sticker type to MPD sticker type
 * @param type myMPD sticker type
 * @return MPD sticker type
 */
enum mympd_sticker_type mympd_api_get_mpd_sticker_type(enum mympd_sticker_type type) {
    switch(type) {
        case STICKER_TYPE_MYMPD_ALBUM:
            return STICKER_TYPE_FILTER;
        default:
            return type;
    }
}
