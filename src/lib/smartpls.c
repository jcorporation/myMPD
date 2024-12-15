/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Smart playlist implementation
 */

#include "compile_time.h"
#include "src/lib/smartpls.h"

#include "src/lib/filehandler.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/msg_queue.h"
#include "src/lib/sds_extras.h"
#include "src/lib/state_files.h"

#include <errno.h>
#include <string.h>
#include <sys/stat.h>

//privat definitions
static bool smartpls_save(sds workdir, const char *smartpltype, const char *playlist,
        const char *fields, const char *sort, bool sortdesc, int max_entries);

//public functions

/**
 * Saves a sticker based smart playlist
 * @param workdir myMPD working directory
 * @param playlist playlist name
 * @param sticker sticker name
 * @param value minimum integer value of sticker
 * @param op compare operator
 * @param sort tag to sort the playlist: empty = no sorting, shuffle or mpd tag
 * @param sortdesc sort descending?
 * @param max_entries maximum number of playlist entries
 * @return true on success else false
 */
bool smartpls_save_sticker(sds workdir, const char *playlist, const char *sticker,
        const char *value, const char *op, const char *sort, bool sortdesc, int max_entries)
{
    sds fields = tojson_char(sdsempty(), "sticker", sticker, true);
    fields = tojson_char(fields, "value", value, true);
    fields = tojson_char(fields, "op", op, true);
    bool rc = smartpls_save(workdir, "sticker", playlist, fields, sort, sortdesc, max_entries);
    FREE_SDS(fields);
    return rc;
}

/**
 * Saves a newest smart playlist
 * @param workdir myMPD working directory
 * @param playlist playlist name
 * @param timerange number of second since now
 * @param sort tag to sort the playlist: empty = no sorting, shuffle or mpd tag
 * @param sortdesc sort descending?
 * @param max_entries maximum number of playlist entries
 * @return true on success else false
 */
bool smartpls_save_newest(sds workdir, const char *playlist, unsigned timerange,
        const char *sort, bool sortdesc, int max_entries)
{
    sds fields = tojson_uint(sdsempty(), "timerange", timerange, true);
    bool rc = smartpls_save(workdir, "newest", playlist, fields, sort, sortdesc, max_entries);
    FREE_SDS(fields);
    return rc;
}

/**
 * Saves a search based smart playlist
 * @param workdir myMPD working directory
 * @param playlist playlist name
 * @param expression mpd search expression
 * @param sort tag to sort the playlist: empty = no sorting, shuffle or mpd tag
 * @param sortdesc sort descending?
 * @param max_entries maximum number of playlist entries
 * @return true on success else false
 */
bool smartpls_save_search(sds workdir, const char *playlist, const char *expression,
        const char *sort, bool sortdesc, int max_entries)
{
    sds fields = tojson_char(sdsempty(), "expression", expression, true);
    bool rc = smartpls_save(workdir, "search", playlist, fields, sort, sortdesc, max_entries);
    FREE_SDS(fields);
    return rc;
}

/**
 * Checks if playlist is a smart playlist
 * @param workdir myMPD working directory
 * @param playlist name of the playlist to check
 * @return true if it is a smart playlist else false
 */
bool is_smartpls(sds workdir, const char *playlist) {
    bool smartpls = false;
    if (strchr(playlist, '/') == NULL) {
        //filename only
        sds smartpls_file = sdscatfmt(sdsempty(), "%S/%s/%s", workdir, DIR_WORK_SMARTPLS, playlist);
        smartpls = testfile_read(smartpls_file);
        FREE_SDS(smartpls_file);
    }
    return smartpls;
}

/**
 * Returns the smart playlist last modification time
 * @param workdir myMPD working directory
 * @param playlist name of the playlist to check
 * @return last modification time
 */
time_t smartpls_get_mtime(sds workdir, const char *playlist) {
    sds plpath = sdscatfmt(sdsempty(), "%S/%s/%s", workdir, DIR_WORK_SMARTPLS, playlist);
    time_t mtime = get_mtime(plpath);
    FREE_SDS(plpath);
    return mtime;
}

/**
 * Sends a request to the mympd_api_queue to update a smart playlist
 * @param playlist smart playlist to update
 * @param conn_id mongoose connection id
 * @param request_id jsonrpc request id
 * @return true on success else false
 */
bool smartpls_update(const char *playlist, unsigned long conn_id, unsigned int request_id) {
    struct t_work_request *request = create_request(REQUEST_TYPE_DEFAULT, conn_id, request_id, MYMPD_API_SMARTPLS_UPDATE, NULL, MPD_PARTITION_DEFAULT);
    request->data = tojson_char(request->data, "plist", playlist, false);
    request->data = jsonrpc_end(request->data);
    return mympd_queue_push(mympd_api_queue, request, 0);
}

/**
 * Sends a request to the mympd_api_queue to update all smart playlists
 * @return true on success else false
 */
bool smartpls_update_all(void) {
    struct t_work_request *request = create_request(REQUEST_TYPE_DEFAULT, 0, 0, MYMPD_API_SMARTPLS_UPDATE_ALL, NULL, MPD_PARTITION_DEFAULT);
    request->data = jsonrpc_end(request->data);
    return mympd_queue_push(mympd_api_queue, request, 0);
}

//privat functions

/**
 * Saves the smart playlist to disk.
 * @param workdir myMPD working directory
 * @param smartpltype type of the smart playlist: sticker, newest or search
 * @param playlist name of the smart playlist
 * @param fields custom fields
 * @param sort mpd tag to sort or the string "shuffle"
 * @param sortdesc sort descending?
 * @param max_entries maximum songs
 * @return true on success else false
 */
static bool smartpls_save(sds workdir, const char *smartpltype, const char *playlist,
        const char *fields, const char *sort, bool sortdesc, int max_entries)
{
    sds line = sdscatlen(sdsempty(), "{", 1);
    line = tojson_char(line, "type", smartpltype, true);
    line = sdscat(line, fields);
    line = tojson_int(line, "maxentries", max_entries, true);
    line = tojson_char(line, "sort", sort, true);
    line = tojson_bool(line, "sortdesc", sortdesc, false);
    line = sdscatlen(line, "}", 1);

    sds pl_file = sdscatfmt(sdsempty(), "%S/smartpls/%s", workdir, playlist);
    bool rc = write_data_to_file(pl_file, line, sdslen(line));
    FREE_SDS(line);
    FREE_SDS(pl_file);
    return rc;
}
