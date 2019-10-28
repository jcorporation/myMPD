/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <string.h>
#include <assert.h>
#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../../dist/src/frozen/frozen.h"
#include "../sds_extras.h"
#include "../log.h"
#include "../list.h"
#include "config_defs.h"
#include "../api.h"
#include "../tiny_queue.h"
#include "../global.h"
#include "../utility.h"
#include "mympd_api_utility.h"

void mympd_api_push_to_mpd_client(t_mympd_state *mympd_state) {
    t_work_request *mpd_client_request = (t_work_request *)malloc(sizeof(t_work_request));
    assert(mpd_client_request);
    mpd_client_request->conn_id = -1;
    mpd_client_request->id = 0;
    mpd_client_request->cmd_id = MYMPD_API_SETTINGS_SET;
    mpd_client_request->method = sdsnew("MYMPD_API_SETTINGS_SET");
    sds data = sdsempty();

    data = sdscat(data, "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"MYMPD_API_SETTINGS_SET\",\"params\":{");
    data = tojson_long(data, "jukeboxMode", mympd_state->jukebox_mode, true);
    data = tojson_char(data, "jukeboxPlaylist", mympd_state->jukebox_playlist, true);
    data = tojson_long(data, "jukeboxQueueLength", mympd_state->jukebox_queue_length, true);
    data = tojson_bool(data, "autoPlay", mympd_state->auto_play, true);
    data = tojson_bool(data, "coverimage", mympd_state->coverimage, true);
    data = tojson_char(data, "coverimageName", mympd_state->coverimage_name, true);
    data = tojson_bool(data, "love", mympd_state->love, true);
    data = tojson_char(data, "loveChannel", mympd_state->love_channel, true);
    data = tojson_char(data, "loveMessage", mympd_state->love_message, true);
    data = tojson_char(data, "taglist", mympd_state->taglist, true);
    data = tojson_char(data, "searchtaglist", mympd_state->searchtaglist, true);
    data = tojson_char(data, "browsetaglist", mympd_state->browsetaglist, true);
    data = tojson_bool(data, "stickers", mympd_state->stickers, true);
    data = tojson_bool(data, "smartpls", mympd_state->smartpls, true);
    data = tojson_char(data, "mpdHost", mympd_state->mpd_host, true);
    data = tojson_char(data, "mpdPass", mympd_state->mpd_pass, true);
    data = tojson_long(data, "mpdPort", mympd_state->mpd_port, true);
    data = tojson_long(data, "lastPlayedCount", mympd_state->last_played_count, true);
    data = tojson_long(data, "maxElementsPerPage", mympd_state->max_elements_per_page, true);
    data = tojson_char(data, "musicDirectory", mympd_state->music_directory, false);
    data = sdscat(data, "}}");

    mpd_client_request->data = data;
    tiny_queue_push(mpd_client_queue, mpd_client_request);
}

void free_mympd_state(t_mympd_state *mympd_state) {
    sdsfree(mympd_state->mpd_host);
    sdsfree(mympd_state->mpd_pass);
    sdsfree(mympd_state->taglist);
    sdsfree(mympd_state->searchtaglist);
    sdsfree(mympd_state->browsetaglist);
    sdsfree(mympd_state->love_channel);
    sdsfree(mympd_state->love_message);
    sdsfree(mympd_state->jukebox_playlist);
    sdsfree(mympd_state->cols_queue_current);
    sdsfree(mympd_state->cols_search);
    sdsfree(mympd_state->cols_browse_database);
    sdsfree(mympd_state->cols_browse_playlists_detail);
    sdsfree(mympd_state->cols_browse_filesystem);
    sdsfree(mympd_state->cols_playback);
    sdsfree(mympd_state->cols_queue_last_played);
    sdsfree(mympd_state->stream_url);
    sdsfree(mympd_state->bg_color);
    sdsfree(mympd_state->bg_css_filter);
    sdsfree(mympd_state->coverimage_name);
    sdsfree(mympd_state->locale);
    sdsfree(mympd_state->music_directory);
    FREE_PTR(mympd_state);
}

static const char *mympd_cols[]={"Pos", "Duration", "Type", "LastPlayed", 0};

static bool is_mympd_col(sds token) {
    const char** ptr = mympd_cols;
    while (*ptr != 0) {
        if (strncmp(token, *ptr, sdslen(token)) == 0) {
            return true;
        }
        ++ptr;
    }
    return false;
}

sds json_to_cols(sds cols, char *str, size_t len) {
    struct json_token t;
    int j = 0;
    for (int i = 0; json_scanf_array_elem(str, len, ".params.cols", i, &t) > 0; i++) {
        if (j > 0) {
            cols = sdscatlen(cols, ",", 1);
        }
        sds token = sdscatlen(sdsempty(), t.ptr, t.len);
        if (mpd_tag_name_iparse(token) != MPD_TAG_UNKNOWN || is_mympd_col(token) == true) {
            cols = sdscatjson(cols, t.ptr, t.len);
            j++;
        }
        else {
            LOG_WARN("Unknown column: %s", token);
        }
        sdsfree(token);
    }
    return cols;
}
