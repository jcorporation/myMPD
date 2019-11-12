/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdlib.h>
#include <libgen.h>
#include <pthread.h>
#include <string.h>
#include <inttypes.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h>
#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../../dist/src/frozen/frozen.h"
#include "../list.h"
#include "config_defs.h"
#include "../tiny_queue.h"
#include "../api.h"
#include "../global.h"
#include "../utility.h"
#include "../log.h"
#include "mpd_client_utility.h"

void mpd_client_notify(sds message) {
    LOG_DEBUG("Push websocket notify to queue: %s", message);
    t_work_result *response = (t_work_result *)malloc(sizeof(t_work_result));
    assert(response);
    response->conn_id = 0;
    response->data = sdsdup(message);
    tiny_queue_push(web_server_queue, response);
}

sds put_song_tags(sds buffer, t_mpd_state *mpd_state, const t_tags *tagcols, const struct mpd_song *song) {
    if (mpd_state->feat_tags == true) {
        for (size_t tagnr = 0; tagnr < tagcols->len; ++tagnr) {
            char *tag_value = mpd_client_get_tag(song, tagcols->tags[tagnr]);
            buffer = tojson_char(buffer, mpd_tag_name(tagcols->tags[tagnr]), tag_value == NULL ? "-" : tag_value, true);
        }
    }
    else {
        char *tag_value = mpd_client_get_tag(song, MPD_TAG_TITLE);
        buffer = tojson_char(buffer, "Title", tag_value == NULL ? "-" : tag_value, true);
    }
    buffer = tojson_long(buffer, "Duration", mpd_song_get_duration(song), true);
    buffer = tojson_char(buffer, "uri", mpd_song_get_uri(song), false);
    return buffer;
}

sds check_error_and_recover(t_mpd_state *mpd_state, sds buffer, sds method, int request_id) {
    if (mpd_connection_get_error(mpd_state->conn) != MPD_ERROR_SUCCESS) {
        LOG_ERROR("MPD error: %s", mpd_connection_get_error_message(mpd_state->conn));
        if (buffer != NULL) {
            buffer = jsonrpc_respond_message(buffer, method, request_id, mpd_connection_get_error_message(mpd_state->conn), true);
        }
        if (!mpd_connection_clear_error(mpd_state->conn)) {
            mpd_state->conn_state = MPD_FAILURE;
        }
    }
    return buffer;
}

sds check_error_and_recover_notify(t_mpd_state *mpd_state, sds buffer) {
    if (mpd_connection_get_error(mpd_state->conn) != MPD_ERROR_SUCCESS) {
        LOG_ERROR("MPD error: %s", mpd_connection_get_error_message(mpd_state->conn));
        if (buffer != NULL) {
            buffer = jsonrpc_start_notify(buffer, "error");
            buffer = tojson_char(buffer, "message", mpd_connection_get_error_message(mpd_state->conn), false);
            buffer = jsonrpc_end_notify(buffer);
        }
        if (!mpd_connection_clear_error(mpd_state->conn)) {
            mpd_state->conn_state = MPD_FAILURE;
        }
    }
    return buffer;
}

sds respond_with_mpd_error_or_ok(t_mpd_state *mpd_state, sds buffer, sds method, int request_id) {
    buffer = sdscrop(buffer);
    buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
    if (sdslen(buffer) == 0) {
        buffer = jsonrpc_respond_ok(buffer, method, request_id);
    }
    return buffer;
}

void json_to_tags(const char *str, int len, void *user_data) {
    struct json_token t;
    int i;
    t_tags *tags = (t_tags *) user_data;
    tags->len = 0;
    for (i = 0; json_scanf_array_elem(str, len, "", i, &t) > 0; i++) {
        sds token = sdscatlen(sdsempty(), t.ptr, t.len);
        enum mpd_tag_type tag = mpd_tag_name_iparse(token);
        sdsfree(token);
        if (tag != MPD_TAG_UNKNOWN) {
            tags->tags[tags->len++] = tag;
        }
    }
}

char *mpd_client_get_tag(struct mpd_song const *song, const enum mpd_tag_type tag) {
    char *str = (char *)mpd_song_get_tag(song, tag, 0);
    if (str == NULL) {
        if (tag == MPD_TAG_TITLE) {
            str = basename((char *)mpd_song_get_uri(song));
        }
        else if (tag == MPD_TAG_ALBUM_ARTIST) {
            str = (char *)mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
        }
    }
    return str;
}

bool is_smartpls(t_config *config, t_mpd_state *mpd_state, const char *plpath) {
    bool smartpls = false;
    if (mpd_state->feat_smartpls == true) {
        sds smartpls_file = sdscatfmt(sdsempty(), "%s/smartpls/%s", config->varlibdir, plpath);
        if (validate_string(plpath) == true) {
            if (access(smartpls_file, F_OK ) != -1) { /* Flawfinder: ignore */
                smartpls = true;
            }
        }
        sdsfree(smartpls_file);
    }
    return smartpls;
}

bool mpd_client_get_sticker(t_mpd_state *mpd_state, const char *uri, t_sticker *sticker) {
    struct mpd_pair *pair;
    char *crap = NULL;
    sticker->playCount = 0;
    sticker->skipCount = 0;
    sticker->lastPlayed = 0;
    sticker->lastSkipped = 0;
    sticker->like = 1;

    if (uri == NULL || strstr(uri, "://") != NULL) {
        return false;
    }

    if (mpd_send_sticker_list(mpd_state->conn, "song", uri)) {
        while ((pair = mpd_recv_sticker(mpd_state->conn)) != NULL) {
            if (strcmp(pair->name, "playCount") == 0) {
                sticker->playCount = strtoimax(pair->value, &crap, 10);
            }
            else if (strcmp(pair->name, "skipCount") == 0) {
                sticker->skipCount = strtoimax(pair->value, &crap, 10);
            }
            else if (strcmp(pair->name, "lastPlayed") == 0) {
                sticker->lastPlayed = strtoimax(pair->value, &crap, 10);
            }
            else if (strcmp(pair->name, "lastSkipped") == 0) {
                sticker->lastSkipped = strtoimax(pair->value, &crap, 10);
            }
            else if (strcmp(pair->name, "like") == 0) {
                sticker->like = strtoimax(pair->value, &crap, 10);
            }
            mpd_return_sticker(mpd_state->conn, pair);
        }
    }
    else {
        check_error_and_recover(mpd_state, NULL, NULL, 0);
        return false;
    }
    return true;
}

bool mpd_client_tag_exists(const enum mpd_tag_type tag_types[64], const size_t tag_types_len, const enum mpd_tag_type tag) {
    for (size_t i = 0; i < tag_types_len; i++) {
        if (tag_types[i] == tag) {
            return true;
        }
    }
    return false;
}

void reset_t_tags(t_tags *tags) {
    tags->len = 0;
    memset(tags->tags, 0, sizeof(tags->tags));
}

void default_mpd_state(t_mpd_state *mpd_state) {
    mpd_state->conn_state = MPD_DISCONNECTED;
    mpd_state->reconnect_time = 0;
    mpd_state->reconnect_intervall = 0;
    mpd_state->timeout = 10000;
    mpd_state->state = MPD_STATE_UNKNOWN;
    mpd_state->song_id = -1;
    mpd_state->song_uri = sdsempty();
    mpd_state->next_song_id = -1;
    mpd_state->last_song_id = -1;
    mpd_state->last_song_uri = sdsempty();
    mpd_state->queue_version = 0;
    mpd_state->queue_length = 0;
    mpd_state->last_last_played_id = -1;
    mpd_state->song_end_time = 0;
    mpd_state->song_start_time = 0;
    mpd_state->last_song_end_time = 0;
    mpd_state->last_song_start_time = 0;
    mpd_state->last_skipped_id = 0;
    mpd_state->crossfade = 0;
    mpd_state->set_song_played_time = 0;
    mpd_state->music_directory = sdsempty();
    mpd_state->music_directory_value = sdsempty();
    mpd_state->jukebox_playlist = sdsempty();
    mpd_state->coverimage_name = sdsempty();
    mpd_state->love_channel = sdsempty();
    mpd_state->love_message = sdsempty();
    mpd_state->taglist = sdsempty();
    mpd_state->searchtaglist = sdsempty();
    mpd_state->browsetaglist = sdsempty();
    mpd_state->mpd_host = sdsempty();
    mpd_state->mpd_port = 0;
    mpd_state->mpd_pass = sdsempty();
    reset_t_tags(&mpd_state->mpd_tag_types);
    reset_t_tags(&mpd_state->mympd_tag_types);
    reset_t_tags(&mpd_state->search_tag_types);
    reset_t_tags(&mpd_state->browse_tag_types);
}

void free_mpd_state(t_mpd_state *mpd_state) {
    sdsfree(mpd_state->music_directory);
    sdsfree(mpd_state->music_directory_value);
    sdsfree(mpd_state->jukebox_playlist);
    sdsfree(mpd_state->song_uri);
    sdsfree(mpd_state->last_song_uri);
    sdsfree(mpd_state->coverimage_name);
    sdsfree(mpd_state->love_channel);
    sdsfree(mpd_state->love_message);
    sdsfree(mpd_state->taglist);
    sdsfree(mpd_state->searchtaglist);
    sdsfree(mpd_state->browsetaglist);
    sdsfree(mpd_state->mpd_host);
    sdsfree(mpd_state->mpd_pass);
    free(mpd_state);
}