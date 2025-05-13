/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD browse database API
 */

#include "compile_time.h"
#include "src/mympd_api/tags.h"

#include "dist/utf8/utf8.h"
#include "src/lib/filehandler.h"
#include "src/lib/json/json_print.h"
#include "src/lib/json/json_rpc.h"
#include "src/lib/log.h"
#include "src/lib/rax_extras.h"
#include "src/lib/sds_extras.h"
#include "src/mympd_client/errorhandler.h"
#include "src/mympd_client/search.h"

// private definitions

static sds tag_list_legacy(struct t_partition_state *partition_state, sds buffer,
    unsigned request_id, sds searchstr, sds tag, unsigned offset, unsigned limit, bool sortdesc);
static sds tag_list_mpd025(struct t_partition_state *partition_state, sds buffer, unsigned request_id,
    sds searchstr, sds tag, unsigned offset, unsigned limit, bool sortdesc);

// public functions

/**
 * Lists tags from the mpd database.
 * Wrapper that chooses the right method by MPD version.
 * @param partition_state pointer to partition specific states
 * @param buffer sds string to append response
 * @param request_id jsonrpc request id
 * @param searchstr string to search
 * @param tag tag type to list
 * @param offset offset of results to print
 * @param limit max number of results to print
 * @param sortdesc true to sort descending, false to sort ascending
 * @return pointer to buffer
 */
sds mympd_api_tag_list(struct t_partition_state *partition_state, sds buffer, unsigned request_id,
        sds searchstr, sds tag, unsigned offset, unsigned limit, bool sortdesc)
{
    return partition_state->mpd_state->feat.mpd_0_25_0 == true && sortdesc == false
        ? tag_list_mpd025(partition_state, buffer, request_id, searchstr, tag, offset, limit, sortdesc)
        : tag_list_legacy(partition_state, buffer, request_id, searchstr, tag, offset, limit, sortdesc);
}

// private functions

/**
 * Lists tags from the mpd database.
 * Searches and sorts the result on client side.
 * @param partition_state pointer to partition specific states
 * @param buffer sds string to append response
 * @param request_id jsonrpc request id
 * @param searchstr string to search
 * @param tag tag type to list
 * @param offset offset of results to print
 * @param limit max number of results to print
 * @param sortdesc true to sort descending, false to sort ascending
 * @return pointer to buffer
 */
static sds tag_list_legacy(struct t_partition_state *partition_state, sds buffer, unsigned request_id,
        sds searchstr, sds tag, unsigned offset, unsigned limit, bool sortdesc)
{
    size_t searchstr_len = sdslen(searchstr);
    enum mympd_cmd_ids cmd_id = MYMPD_API_DATABASE_TAG_LIST;
    enum mpd_tag_type mpdtag = mpd_tag_name_parse(tag);

    if (mpd_search_db_tags(partition_state->conn, mpdtag) == false) {
        mpd_search_cancel(partition_state->conn);
        return jsonrpc_respond_message(buffer, cmd_id, request_id, JSONRPC_FACILITY_DATABASE,
            JSONRPC_SEVERITY_ERROR, "Error creating MPD search command");
    }

    unsigned real_limit = offset + limit;
    rax *taglist = raxNew();
    sds key = sdsempty();

    if (mpd_search_commit(partition_state->conn)) {
        struct mpd_pair *pair;
        //filter and sort
        while ((pair = mpd_recv_pair_tag(partition_state->conn, mpdtag)) != NULL) {
            if (pair->value[0] == '\0') {
                MYMPD_LOG_DEBUG(partition_state->name, "Value is empty, skipping");
            }
            else if (searchstr_len == 0 ||
                (searchstr_len <= 2 && utf8ncasecmp(searchstr, pair->value, searchstr_len) == 0) ||
                (searchstr_len > 2 && utf8casestr(pair->value, searchstr) != NULL))
            {
                key = sdscat(key, pair->value);
                //handle tags case insensitive
                sds_utf8_tolower(key);
                sds data = sdsnew(pair->value);
                rax_insert_no_dup(taglist, key, data);
                sdsclear(key);
            }
            mpd_return_pair(partition_state->conn, pair);
        }
    }
    if (mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, "mpd_search_db_tags") == false) {
        rax_free_sds_data(taglist);
        return buffer;
    }
    FREE_SDS(key);

    //print list
    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    raxIterator iter;
    raxStart(&iter, taglist);
    int (*iterator)(struct raxIterator *iter);
    if (sortdesc == false) {
        raxSeek(&iter, "^", NULL, 0);
        iterator = &raxNext;
    }
    else {
        raxSeek(&iter, "$", NULL, 0);
        iterator = &raxPrev;
    }
    while (iterator(&iter)) {
        if (entity_count >= offset &&
            entity_count < real_limit)
        {
            if (entities_returned++) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            buffer = sdscatlen(buffer, "{", 1);
            buffer = tojson_sds(buffer, "Value", (sds)iter.data, false);
            buffer = sdscatlen(buffer, "}", 1);
        }
        entity_count++;
        FREE_SDS(iter.data);
    }
    raxStop(&iter);

    //checks if this tag has a directory with pictures in /var/lib/mympd/pics
    sds pic_path = sdscatfmt(sdsempty(), "%S/%s/%s", partition_state->config->workdir, DIR_WORK_PICS, tag);
    bool pic =  testdir("Tag pics folder", pic_path, false, true) == DIR_EXISTS
        ? true
        : false;
    FREE_SDS(pic_path);

    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_uint64(buffer, "totalEntities", taglist->numele, true);
    buffer = tojson_uint(buffer, "returnedEntities", entities_returned, true);
    buffer = tojson_uint(buffer, "offset", offset, true);
    buffer = tojson_uint(buffer, "limit", limit, true);
    buffer = tojson_sds(buffer, "searchstr", searchstr, true);
    buffer = tojson_sds(buffer, "tag", tag, true);
    buffer = tojson_bool(buffer, "sortdesc", sortdesc, true);
    buffer = tojson_bool(buffer, "pics", pic, false);
    buffer = jsonrpc_end(buffer);
    raxFree(taglist);
    return buffer;
}

/**
 * Lists tags from the mpd database.
 * Uses window parameter and does filtering and sorting on MPD side.
 * Supported since MPD 0.25.
 * @param partition_state pointer to partition specific states
 * @param buffer sds string to append response
 * @param request_id jsonrpc request id
 * @param searchstr string to search
 * @param tag tag type to list
 * @param offset offset of results to print
 * @param limit max number of results to print
 * @param sortdesc true to sort descending, false to sort ascending
 * @return pointer to buffer
 */
static sds tag_list_mpd025(struct t_partition_state *partition_state, sds buffer, unsigned request_id,
    sds searchstr, sds tag, unsigned offset, unsigned limit, bool sortdesc)
{
    enum mympd_cmd_ids cmd_id = MYMPD_API_DATABASE_TAG_LIST;
    unsigned real_limit = limit + offset;
    sds expr = escape_mpd_search_expression(sdsempty(), tag, "contains_ci", searchstr);
    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    unsigned entities_returned = 0;
    enum mpd_tag_type mpdtag = mpd_tag_name_parse(tag);
    (void) sortdesc; // not implemented in MPD

    if (mpd_search_db_tags(partition_state->conn, mpdtag) == false ||
        mpd_search_add_expression(partition_state->conn, expr) == false ||
        mpd_search_add_window(partition_state->conn, offset, real_limit) == false)
    {
        mpd_search_cancel(partition_state->conn);
        FREE_SDS(expr);
        return jsonrpc_respond_message(buffer, cmd_id, request_id, JSONRPC_FACILITY_DATABASE,
            JSONRPC_SEVERITY_ERROR, "Error creating MPD search command");
    }

    if (mpd_search_commit(partition_state->conn)) {
        struct mpd_pair *pair;
        while ((pair = mpd_recv_pair_tag(partition_state->conn, mpdtag)) != NULL) {
            if (pair->value[0] == '\0') {
                MYMPD_LOG_DEBUG(partition_state->name, "Value is empty, skipping");
            }
            else {
                if (entities_returned++) {
                    buffer = sdscatlen(buffer, ",", 1);
                }
                buffer = sdscatlen(buffer, "{", 1);
                buffer = tojson_char(buffer, "Value", pair->value, false);
                buffer = sdscatlen(buffer, "}", 1);
            }
            mpd_return_pair(partition_state->conn, pair);
        }
    }
    if (mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, "mpd_search_db_tags") == false) {
        FREE_SDS(expr);
        return buffer;
    }

    //checks if this tag has a directory with pictures in /src/lib/mympd/pics
    sds pic_path = sdscatfmt(sdsempty(), "%S/%s/%s", partition_state->config->workdir, DIR_WORK_PICS, tag);
    bool pic =  testdir("Tag pics folder", pic_path, false, true) == DIR_EXISTS
        ? true
        : false;
    FREE_SDS(pic_path);
    FREE_SDS(expr);

    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_int(buffer, "totalEntities", -1, true);
    buffer = tojson_uint(buffer, "returnedEntities", entities_returned, true);
    buffer = tojson_uint(buffer, "offset", offset, true);
    buffer = tojson_uint(buffer, "limit", limit, true);
    buffer = tojson_sds(buffer, "searchstr", searchstr, true);
    buffer = tojson_sds(buffer, "tag", tag, true);
    buffer = tojson_bool(buffer, "sortdesc", sortdesc, true);
    buffer = tojson_bool(buffer, "pics", pic, false);
    buffer = jsonrpc_end(buffer);
    return buffer;
}
