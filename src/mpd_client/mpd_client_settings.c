/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../../dist/src/frozen/frozen.h"
#include "../list.h"
#include "config_defs.h"
#include "../utility.h"
#include "../api.h"
#include "../log.h"
#include "mpd_client_utility.h"
#include "mpd_client_settings.h"

//private defintions
static sds print_tags_array(sds buffer, const char *tagsname, t_tags tags);

//public functions
bool mpd_api_settings_set(t_config *config, t_mpd_state *mpd_state, struct json_token *key, 
                          struct json_token *val, bool *mpd_host_changed, bool *jukebox_changed)
{
    bool rc = true;
    char *crap;
    sds settingvalue = sdscatlen(sdsempty(), val->ptr, val->len);
    LOG_DEBUG("Parse setting \"%.*s\" with value \"%.*s\"", key->len, key->ptr, val->len, val->ptr);
    if (strncmp(key->ptr, "mpdPass", key->len) == 0) {
        if (strncmp(val->ptr, "dontsetpassword", val->len) != 0) {
            *mpd_host_changed = true;
            mpd_state->mpd_pass = sdsreplacelen(mpd_state->mpd_pass, settingvalue, sdslen(settingvalue));
        }
        else {
            sdsfree(settingvalue);
            return true;
        }
    }
    else if (strncmp(key->ptr, "mpdHost", key->len) == 0) {
        if (strncmp(val->ptr, mpd_state->mpd_host, val->len) != 0) {
            *mpd_host_changed = true;
            mpd_state->mpd_host = sdsreplacelen(mpd_state->mpd_host, settingvalue, sdslen(settingvalue));
        }
    }
    else if (strncmp(key->ptr, "mpdPort", key->len) == 0) {
        int mpd_port = strtoimax(settingvalue, &crap, 10);
        if (mpd_state->mpd_port != mpd_port) {
            *mpd_host_changed = true;
            mpd_state->mpd_port = mpd_port;
        }
    }
    else if (strncmp(key->ptr, "musicDirectory", key->len) == 0) {
        mpd_state->music_directory = sdsreplacelen(mpd_state->music_directory, settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "jukeboxMode", key->len) == 0) {
        unsigned jukebox_mode = strtoumax(settingvalue, &crap, 10);
        if (jukebox_mode > 2) {
            sdsfree(settingvalue);
            return false;
        }
        if (mpd_state->jukebox_mode != jukebox_mode) {
            mpd_state->jukebox_mode = jukebox_mode;
            *jukebox_changed = true;
        }
    }
    else if (strncmp(key->ptr, "jukeboxPlaylist", key->len) == 0) {
        if (strcmp(mpd_state->jukebox_playlist, settingvalue) != 0) {
            mpd_state->jukebox_playlist = sdsreplacelen(mpd_state->jukebox_playlist, settingvalue, sdslen(settingvalue));
            *jukebox_changed = true;
        }
    }
    else if (strncmp(key->ptr, "jukeboxQueueLength", key->len) == 0) {
        int jukebox_queue_length = strtoimax(settingvalue, &crap, 10);
        if (jukebox_queue_length <= 0 || jukebox_queue_length > 999) {
            sdsfree(settingvalue);
            return false;
        }
        mpd_state->jukebox_queue_length = jukebox_queue_length;
    }
    else if (strncmp(key->ptr, "jukeboxLastPlayed", key->len) == 0) {
        int jukebox_last_played = strtoimax(settingvalue, &crap, 10);
        if (jukebox_last_played != mpd_state->jukebox_last_played) {
            mpd_state->jukebox_last_played = jukebox_last_played;
            *jukebox_changed = true;
        }
    }
    else if (strncmp(key->ptr, "jukeboxUniqueTag", key->len) == 0) {
        enum mpd_tag_type unique_tag = mpd_tag_name_parse(settingvalue);
        if (unique_tag == MPD_TAG_UNKNOWN) {
            unique_tag = MPD_TAG_TITLE;
        }
        if (mpd_state->jukebox_unique_tag.tags[0] != unique_tag) {
            mpd_state->jukebox_unique_tag.tags[0] = unique_tag;
            *jukebox_changed = true;
        }
    }
    else if (strncmp(key->ptr, "autoPlay", key->len) == 0) {
        mpd_state->auto_play = val->type == JSON_TYPE_TRUE ? true : false;
    }
    else if (strncmp(key->ptr, "coverimage", key->len) == 0) {
        mpd_state->coverimage = val->type == JSON_TYPE_TRUE ? true : false;
    }
    else if (strncmp(key->ptr, "coverimageName", key->len) == 0) {
        if (validate_string(settingvalue) && sdslen(settingvalue) > 0) {
            mpd_state->coverimage_name = sdsreplacelen(mpd_state->coverimage_name, settingvalue, sdslen(settingvalue));
        }
        else {
            sdsfree(settingvalue);
            return false;
        }
    }
    else if (strncmp(key->ptr, "bookletName", key->len) == 0) {
        mpd_state->booklet_name = sdsreplacelen(mpd_state->booklet_name, settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "love", key->len) == 0) {
        mpd_state->love = val->type == JSON_TYPE_TRUE ? true : false;
    }
    else if (strncmp(key->ptr, "loveChannel", key->len) == 0) {
        mpd_state->love_channel = sdsreplacelen(mpd_state->love_channel, settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "loveMessage", key->len) == 0) {
        mpd_state->love_message = sdsreplacelen(mpd_state->love_message, settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "taglist", key->len) == 0) {
        mpd_state->taglist = sdsreplacelen(mpd_state->taglist, settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "searchtaglist", key->len) == 0) {
        mpd_state->searchtaglist = sdsreplacelen(mpd_state->searchtaglist, settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "browsetaglist", key->len) == 0) {
        mpd_state->browsetaglist = sdsreplacelen(mpd_state->browsetaglist, settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "stickers", key->len) == 0) {
        mpd_state->stickers = val->type == JSON_TYPE_TRUE ? true : false;
    }
    else if (strncmp(key->ptr, "smartpls", key->len) == 0) {
        mpd_state->smartpls = val->type == JSON_TYPE_TRUE ? true : false;
    }
    else if (strncmp(key->ptr, "smartplsSort", key->len) == 0) {
        mpd_state->smartpls_sort = sdsreplacelen(mpd_state->smartpls_sort, settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "smartplsPrefix", key->len) == 0) {
        mpd_state->smartpls_prefix = sdsreplacelen(mpd_state->smartpls_prefix, settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "smartplsInterval", key->len) == 0) {
        mpd_state->smartpls_interval = strtoumax(settingvalue, &crap, 10);
    }
    else if (strncmp(key->ptr, "generatePlsTags", key->len) == 0) {
        mpd_state->generate_pls_tags = sdsreplacelen(mpd_state->generate_pls_tags, settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "maxElementsPerPage", key->len) == 0) {
        int max_elements_per_page = strtoimax(settingvalue, &crap, 10);
        if (max_elements_per_page <= 0 || max_elements_per_page > 999) {
            sdsfree(settingvalue);
            return false;
        }
        mpd_state->max_elements_per_page = max_elements_per_page;
    }
    else if (strncmp(key->ptr, "lastPlayedCount", key->len) == 0) {
        int last_played_count = strtoimax(settingvalue, &crap, 10);
        if (last_played_count <= 0) {
            sdsfree(settingvalue);
            return false;
        }
        mpd_state->last_played_count = last_played_count;
    }
    else if (strncmp(key->ptr, "random", key->len) == 0) {
        unsigned uint_buf = strtoumax(settingvalue, &crap, 10);
        rc = mpd_run_random(mpd_state->conn, uint_buf);
    }
    else if (strncmp(key->ptr, "repeat", key->len) == 0) {
        unsigned uint_buf = strtoumax(settingvalue, &crap, 10);
        rc = mpd_run_repeat(mpd_state->conn, uint_buf);
    }
    else if (strncmp(key->ptr, "consume", key->len) == 0) {
        unsigned uint_buf = strtoumax(settingvalue, &crap, 10);
        rc = mpd_run_consume(mpd_state->conn, uint_buf);
    }
    else if (strncmp(key->ptr, "single", key->len) == 0) {
        unsigned uint_buf = strtoumax(settingvalue, &crap, 10);
        if (mpd_state->feat_single_oneshot == true) {
            enum mpd_single_state state;
            if (uint_buf == 0) { state = MPD_SINGLE_OFF; }
            else if (uint_buf == 1) { state = MPD_SINGLE_ON; }
            else if (uint_buf == 2) { state = MPD_SINGLE_ONESHOT; }
            else { state = MPD_SINGLE_UNKNOWN; }
            rc = mpd_run_single_state(mpd_state->conn, state);
        }
        else {
            rc = mpd_run_single(mpd_state->conn, uint_buf);
        }
    }
    else if (strncmp(key->ptr, "crossfade", key->len) == 0) {
        unsigned uint_buf = strtoumax(settingvalue, &crap, 10);
        rc = mpd_run_crossfade(mpd_state->conn, uint_buf);
    }
    else if (strncmp(key->ptr, "mixrampdb", key->len) == 0) {
        if (config->mixramp == true) {
            float float_buf = strtof(settingvalue, &crap);
            rc = mpd_run_mixrampdb(mpd_state->conn, float_buf);
        }
    }
    else if (strncmp(key->ptr, "mixrampdelay", key->len) == 0) {
        if (config->mixramp == true) {
            float float_buf = strtof(settingvalue, &crap);
            rc = mpd_run_mixrampdelay(mpd_state->conn, float_buf);
        }
    }
    else if (strncmp(key->ptr, "replaygain", key->len) == 0) {
        enum mpd_replay_gain_mode mode = mpd_parse_replay_gain_name(settingvalue);
        if (mode == MPD_REPLAY_UNKNOWN) {
            LOG_ERROR("Unknown replay gain mode: %s", settingvalue);
        }
        else {
            rc = mpd_run_replay_gain_mode(mpd_state->conn, mode);
        }
    }    

    sdsfree(settingvalue);
    return rc;
}

sds mpd_client_put_settings(t_mpd_state *mpd_state, sds buffer, sds method, int request_id) {
    struct mpd_status *status = mpd_run_status(mpd_state->conn);
    if (status == NULL) {
        buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
        return buffer;
    }

    enum mpd_replay_gain_mode replay_gain_mode = mpd_run_replay_gain_status(mpd_state->conn);
    if (replay_gain_mode == MPD_REPLAY_UNKNOWN) {
        if (check_error_and_recover2(mpd_state, &buffer, method, request_id, false) == false) {
            return buffer;
        }
    }
    const char *replaygain = mpd_parse_replay_gain_mode(replay_gain_mode);
    
    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer, ",");
    buffer = tojson_long(buffer, "repeat", mpd_status_get_repeat(status), true);
    if (mpd_state->feat_single_oneshot == true) {
        buffer = tojson_long(buffer, "single", mpd_status_get_single_state(status), true);
    }
    else {
        buffer = tojson_long(buffer, "single", mpd_status_get_single(status), true);
    }
    buffer = tojson_long(buffer, "crossfade", mpd_status_get_crossfade(status), true);
    buffer = tojson_long(buffer, "random", mpd_status_get_random(status), true);
    buffer = tojson_long(buffer, "consume", mpd_status_get_consume(status), true);
    buffer = tojson_float(buffer, "mixrampdb", mpd_status_get_mixrampdb(status), true);
    buffer = tojson_float(buffer, "mixrampdelay", mpd_status_get_mixrampdelay(status), true);
    buffer = tojson_char(buffer, "replaygain", replaygain == NULL ? "" : replaygain, true);
    buffer = tojson_bool(buffer, "featPlaylists", mpd_state->feat_playlists, true);
    buffer = tojson_bool(buffer, "featTags", mpd_state->feat_tags, true);
    buffer = tojson_bool(buffer, "featLibrary", mpd_state->feat_library, true);
    buffer = tojson_bool(buffer, "featAdvsearch", mpd_state->feat_advsearch, true);
    buffer = tojson_bool(buffer, "featStickers", mpd_state->feat_sticker, true);
    buffer = tojson_bool(buffer, "featSmartpls", mpd_state->feat_smartpls, true);
    buffer = tojson_bool(buffer, "featLove", mpd_state->feat_love, true);
    buffer = tojson_bool(buffer, "featCoverimage", mpd_state->feat_coverimage, true);
    buffer = tojson_bool(buffer, "featFingerprint", mpd_state->feat_fingerprint, true);
    buffer = tojson_bool(buffer, "featSingleOneshot", mpd_state->feat_single_oneshot, true);
    buffer = tojson_bool(buffer, "featSearchwindow", mpd_state->feat_mpd_searchwindow, true);
    buffer = tojson_char(buffer, "musicDirectoryValue", mpd_state->music_directory_value, true);
    buffer = tojson_bool(buffer, "mpdConnected", true, true);
    mpd_status_free(status);

    buffer = print_tags_array(buffer, "tags", mpd_state->mympd_tag_types);
    buffer = sdscat(buffer, ",");
    buffer = print_tags_array(buffer, "searchtags", mpd_state->search_tag_types);
    buffer = sdscat(buffer, ",");
    buffer = print_tags_array(buffer, "browsetags", mpd_state->browse_tag_types);
    buffer = sdscat(buffer, ",");
    buffer = print_tags_array(buffer, "allmpdtags", mpd_state->mpd_tag_types);
    buffer = sdscat(buffer, ",");
    buffer = print_tags_array(buffer, "generatePlsTags", mpd_state->generate_pls_tag_types);

    buffer = jsonrpc_end_result(buffer);
    
    return buffer;
}

//private functions
static sds print_tags_array(sds buffer, const char *tagsname, t_tags tags) {
    buffer = sdscatfmt(buffer, "\"%s\": [", tagsname);
    for (size_t i = 0; i < tags.len; i++) {
        if (i > 0) {
            buffer = sdscat(buffer, ",");
        }
        const char *tagname = mpd_tag_name(tags.tags[i]);
        buffer = sdscatjson(buffer, tagname, strlen(tagname));
    }
    buffer = sdscat(buffer, "]");
    return buffer;
}
