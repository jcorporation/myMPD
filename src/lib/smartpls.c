/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "smartpls.h"

#include "filehandler.h"
#include "jsonrpc.h"
#include "log.h"
#include "sds_extras.h"
#include "state_files.h"

#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

//privat definitions
static bool _smartpls_save(sds workdir, const char *smartpltype,
        const char *playlist, const char *expression, const int max_entries,
        const int timerange, const char *sort);
static bool _smartpls_init(sds workdir, const char *name, const char *value);

//public functions

bool smartpls_save_sticker(sds workdir, sds playlist, sds sticker, int max_entries, int min_value, sds sort) {
    return _smartpls_save(workdir, "sticker", playlist, sticker, max_entries, min_value, sort);
}

bool smartpls_save_newest(sds workdir, sds playlist, int timerange, sds sort) {
    return _smartpls_save(workdir, "newest", playlist, NULL, 0, timerange, sort);
}

bool smartpls_save_search(sds workdir, sds playlist, sds expression, sds sort) {
    return _smartpls_save(workdir, "search", playlist, expression, 0, 0, sort);
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
        sds smartpls_file = sdscatfmt(sdsempty(), "%S/smartpls/%s", workdir, playlist);
        if (access(smartpls_file, F_OK ) != -1) { /* Flawfinder: ignore */
            smartpls = true;
        }
        FREE_SDS(smartpls_file);
    }
    return smartpls;
}

/**
 * Returns the samrt playlist last modification time
 * @param workdir myMPD working directory
 * @param playlist name of the playlist to check
 * @return last modification time
 */
time_t smartpls_get_mtime(sds workdir, const char *playlist) {
    sds plpath = sdscatfmt(sdsempty(), "%S/smartpls/%s", workdir, playlist);
    struct stat attr;
    errno = 0;
    if (stat(plpath, &attr) != 0) {
        MYMPD_LOG_ERROR("Error getting mtime for \"%s\"", plpath);
        MYMPD_LOG_ERRNO(errno);
        attr.st_mtime = 0;
    }
    FREE_SDS(plpath);
    return attr.st_mtime;
}

bool smartpls_default(sds workdir) {
    bool rc = true;

    //try to get prefix from state file, fallback to default value
    sds prefix = state_file_rw_string(workdir, "state", "smartpls_prefix", MYMPD_SMARTPLS_PREFIX, vcb_isname, false);

    sds smartpls_file = sdscatfmt(sdsempty(), "%S-bestRated", prefix);
    rc = _smartpls_init(workdir, smartpls_file,
        "{\"type\": \"sticker\", \"sticker\": \"like\", \"maxentries\": 200, \"minvalue\": 2, \"sort\": \"\"}");
    if (rc == false) {
        FREE_SDS(smartpls_file);
        FREE_SDS(prefix);
        return rc;
    }

    sdsclear(smartpls_file);
    smartpls_file = sdscatfmt(smartpls_file, "%S-mostPlayed", prefix);
    rc = _smartpls_init(workdir, smartpls_file,
        "{\"type\": \"sticker\", \"sticker\": \"playCount\", \"maxentries\": 200, \"minvalue\": 0, \"sort\": \"\"}");
    if (rc == false) {
        FREE_SDS(smartpls_file);
        FREE_SDS(prefix);
        return rc;
    }

    sdsclear(smartpls_file);
    smartpls_file = sdscatfmt(smartpls_file, "%S-newestSongs", prefix);
    rc = _smartpls_init(workdir, smartpls_file,
        "{\"type\": \"newest\", \"timerange\": 604800, \"sort\": \"\"}");
    FREE_SDS(smartpls_file);
    FREE_SDS(prefix);

    return rc;
}

void smartpls_update(const char *playlist) {
    struct t_work_request *request = create_request(-1, 0, MYMPD_API_SMARTPLS_UPDATE, NULL);
    request->data = tojson_char(request->data, "plist", playlist, false);
    request->data = sdscatlen(request->data, "}}", 2);
    mympd_queue_push(mympd_api_queue, request, 0);
}

void smartpls_update_all(void) {
    struct t_work_request *request = create_request(-1, 0, MYMPD_API_SMARTPLS_UPDATE_ALL, NULL);
    request->data = sdscatlen(request->data, "}}", 2);
    mympd_queue_push(mympd_api_queue, request, 0);
}

//privat functions

/**
 * Saves the smart playlist to disk.
 * @param workdir myMPD working directory
 * @param smartpltype type of the smart playlist: sticker, newest or search
 * @param playlist name of the smart playlist
 * @param expression mpd search expression
 * @param max_entries max entries for the playlist
 * @param timerange timerange for newest smart playlist type
 * @param sort mpd tag to sort or shuffle
 * @return true on success else false
 */
static bool _smartpls_save(sds workdir, const char *smartpltype, const char *playlist,
                              const char *expression, const int max_entries,
                              const int timerange, const char *sort)
{
    sds line = sdscatlen(sdsempty(), "{", 1);
    line = tojson_char(line, "type", smartpltype, true);
    if (strcmp(smartpltype, "sticker") == 0) {
        line = tojson_char(line, "sticker", expression, true);
        line = tojson_long(line, "maxentries", max_entries, true);
        line = tojson_long(line, "minvalue", timerange, true);
    }
    else if (strcmp(smartpltype, "newest") == 0) {
        line = tojson_long(line, "timerange", timerange, true);
    }
    else if (strcmp(smartpltype, "search") == 0) {
        line = tojson_char(line, "expression", expression, true);
    }
    line = tojson_char(line, "sort", sort, false);
    line = sdscatlen(line, "}", 1);

    sds pl_file = sdscatfmt(sdsempty(), "%S/smartpls/%s", workdir, playlist);
    bool rc = write_data_to_file(pl_file, line, sdslen(line));
    FREE_SDS(line);
    FREE_SDS(pl_file);
    return rc;
}

static bool _smartpls_init(sds workdir, const char *name, const char *value) {
    sds filepath = sdscatfmt(sdsempty(), "%S/smartpls/%s", workdir, name);
    bool rc = write_data_to_file(filepath, value, strlen(value));
    FREE_SDS(filepath);
    return rc;
}
