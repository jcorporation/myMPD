/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <string.h>
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
#include "mympd_api_timer.h"

void mympd_api_push_to_mpd_client(t_mympd_state *mympd_state) {
    t_work_request *request = create_request(-1, 0, MYMPD_API_SETTINGS_SET, "MYMPD_API_SETTINGS_SET", "");

    request->data = sdscat(request->data, "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"MYMPD_API_SETTINGS_SET\",\"params\":{");
    request->data = tojson_long(request->data, "jukeboxMode", mympd_state->jukebox_mode, true);
    request->data = tojson_char(request->data, "jukeboxPlaylist", mympd_state->jukebox_playlist, true);
    request->data = tojson_long(request->data, "jukeboxQueueLength", mympd_state->jukebox_queue_length, true);
    request->data = tojson_long(request->data, "jukeboxLastPlayed", mympd_state->jukebox_last_played, true);
    request->data = tojson_char(request->data, "jukeboxUniqueTag", mympd_state->jukebox_unique_tag, true);
    request->data = tojson_bool(request->data, "autoPlay", mympd_state->auto_play, true);
    request->data = tojson_bool(request->data, "coverimage", mympd_state->coverimage, true);
    request->data = tojson_char(request->data, "coverimageName", mympd_state->coverimage_name, true);
    request->data = tojson_bool(request->data, "love", mympd_state->love, true);
    request->data = tojson_char(request->data, "loveChannel", mympd_state->love_channel, true);
    request->data = tojson_char(request->data, "loveMessage", mympd_state->love_message, true);
    request->data = tojson_char(request->data, "taglist", mympd_state->taglist, true);
    request->data = tojson_char(request->data, "searchtaglist", mympd_state->searchtaglist, true);
    request->data = tojson_char(request->data, "browsetaglist", mympd_state->browsetaglist, true);
    request->data = tojson_bool(request->data, "stickers", mympd_state->stickers, true);
    request->data = tojson_bool(request->data, "smartpls", mympd_state->smartpls, true);
    request->data = tojson_char(request->data, "smartplsSort", mympd_state->smartpls_sort, true);
    request->data = tojson_char(request->data, "smartplsPrefix", mympd_state->smartpls_prefix, true);
    request->data = tojson_long(request->data, "smartplsInterval", mympd_state->smartpls_interval, true);
    request->data = tojson_char(request->data, "generatePlsTags", mympd_state->generate_pls_tags, true);
    request->data = tojson_char(request->data, "mpdHost", mympd_state->mpd_host, true);
    request->data = tojson_char(request->data, "mpdPass", mympd_state->mpd_pass, true);
    request->data = tojson_long(request->data, "mpdPort", mympd_state->mpd_port, true);
    request->data = tojson_long(request->data, "lastPlayedCount", mympd_state->last_played_count, true);
    request->data = tojson_long(request->data, "maxElementsPerPage", mympd_state->max_elements_per_page, true);
    request->data = tojson_char(request->data, "musicDirectory", mympd_state->music_directory, false);
    request->data = sdscat(request->data, "}}");

    tiny_queue_push(mpd_client_queue, request);
}

void free_mympd_state(t_mympd_state *mympd_state) {
    sdsfree(mympd_state->mpd_host);
    sdsfree(mympd_state->mpd_pass);
    sdsfree(mympd_state->taglist);
    sdsfree(mympd_state->searchtaglist);
    sdsfree(mympd_state->browsetaglist);
    sdsfree(mympd_state->generate_pls_tags);
    sdsfree(mympd_state->love_channel);
    sdsfree(mympd_state->love_message);
    sdsfree(mympd_state->jukebox_playlist);
    sdsfree(mympd_state->jukebox_unique_tag);
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
    sdsfree(mympd_state->theme);
    sdsfree(mympd_state->highlight_color);
    sdsfree(mympd_state->smartpls_sort);
    sdsfree(mympd_state->smartpls_prefix);
    truncate_timerlist(&mympd_state->timer_list);
    FREE_PTR(mympd_state);
}

static const char *mympd_cols[]={"Pos", "Duration", "Type", "LastPlayed", "Filename", "Filetype", "Fileformat", "LastModified", 0};

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
