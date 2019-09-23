/* myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de> This project's
   homepage is: https://github.com/jcorporation/mympd
   
   myMPD ist fork of:
   
   ympd
   (c) 2013-2014 Andrew Karpow <andy@ndyk.de>
   This project's homepage is: http://www.ympd.org
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../utility.h"
#include "../api.h"
#include "../log.h"
#include "../list.h"
#include "../config_defs.h"
#include "../tiny_queue.h"
#include "mpd_client_utils.h"
#include "mpd_settings.h"

bool mpd_api_settings_set(t_config *config, t_mpd_state *mpd_state, struct json_token *key, struct json_token *val, bool *mpd_host_changed) {
    bool rc = true;
    char *crap;
    sds settingvalue = sdscatlen(sdsempty(), val->ptr, val->len);
    if (strncmp(key->ptr, "mpdPass", key->len) == 0) {
        if (strcmp(key, "dontsetpassword") != 0) {
            mpd_host_changed = true;
            mpd_state->mpd_pass = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
        }
        else {
            sds_free(settingvalue);
            return true;
        }
    }
    else if (strncmp(key->ptr, "mpdHost", key->len) == 0) {
        if (strncmp(val->ptr, mpd_state->mpd_host, val->len) != 0) {
            mpd_host_changed = true;
            mpd_state->mpd_host = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
        }
    }
    else if (strncmp(key->ptr, "mpdPort", key->len) == 0) {
        int mpd_port = strtoimax(settingvalue, &crap, 10);
        if (mpd_state->mpd_port != mpd_port) {
            mpd_host_changed = true;
            mpd_state->mpd_port = mpd_port;
        }
    }
    else if (strncmp(key->ptr, "musicDirectory", key->len) == 0) {
        mpd_state->mpd_host = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "jukeboxMode", key->len) == 0) {
        int jukebox_mode = strtoimax(settingvalue, &crap, 10);
        if (jukebox_mode < 0 || jukebox_mode > 2) {
            sds_free(settingvalue);
            return false;
        }
        mpd_state->jukebox_mode = jukebox_mode;
    }
    else if (strncmp(key->ptr, "jukeboxPlaylist", key->len) == 0) {
        mpd_state->jukebox_playlist = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "jukeboxQueueLength", key->len) == 0) {
        int jukebox_queue_length = strtoimax(settingvalue, &crap, 10);
        if (jukebox_queue_length <= 0 || jukebox_queue_length > 999) {
            sds_free(settingvalue);
            return false;
        }
        mpd_state->jukebox_queue_length = jukebox_queue_length;
    }
    else if (strncmp(key->ptr, "autoPlay", key->len) == 0) {
        mpd_state->auto_play = val->type == JSON_TYPE_TRUE ? true : false;
    }
    else if (strncmp(key->ptr, "coverimage", key->len) == 0) {
        mympd_state->coverimage = val->type == JSON_TYPE_TRUE ? true : false;
        settingname = sdscat(sdsempty(), "coverimage");
    }
    else if (strncmp(key->ptr, "coverimageName", key->len) == 0) {
        if (validate_string(settingvalue) && sdslen(settingvalue) > 0) {
            mpd_state->coverimage_name = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
        }
        else {
            sds_free(settingvalue);
            return false;
        }
    }
    else if (strncmp(key->ptr, "love", key->len) == 0) {
        mpd_state->love = val->type == JSON_TYPE_TRUE ? true : false;
    }
    else if (strcmp(key->ptr, "loveChannel", key->len) == 0) {
        mpd_state->love_channel = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "loveMessage", key->len) == 0) {
        mpd_state->love_message = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "taglist", key->len) == 0) {
        mpd_state->taglist = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "searchtaglist", key->len) == 0) {
        mpd_state->searchtaglist = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "browsetaglist", key->len) == 0) {
        mpd_state->browsetaglist = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "stickers", key->len) == 0) {
        mpd_state->stickers = val->type == JSON_TYPE_TRUE ? true : false;
    }
    else if (strncmp(key->ptr, "smartpls", key->len) == 0) {
        mpd_state->smartpls = val->type == JSON_TYPE_TRUE ? true : false;
    }
    else if (strncmp(key->ptr, "maxElementsPerPage", key->len) == 0) {
        int max_elements_per_page = strtoimax(settingvalue, &crap, 10);
        if (max_elements_per_page <= 0 || max_elements_per_page > 999) {
            sds_free(settingvalue);
            return false;
        }
        mpd_state->max_elements_per_page = max_elements_per_page;
    }
    else if (strncmp(key->ptr, "lastPlayedCount", key->len) == 0) {
        int last_played_count = strtoimax(settingvalue, &crap, 10);
        if (last_played_count <= 0) {
            sds_free(settingvalue);
            return false;
        }
        mpd_state->last_played_count = last_played_count;
    }
    else if (strncmp(key->ptr, "random", key->len) == 0) {
        unsigned uint_buf = strtoumax(settingvalue, &crap, 10);
        rc = mpd_run_random(mpd_state->conn, uint_buf));
    }
    else if (strncmp(key->ptr, "repeat", key->len) == 0) {
        unsigned uint_buf = strtoumax(settingvalue, &crap, 10);
        rc = mpd_run_repeat(mpd_state->conn, uint_buf));
    }
    else if (strncmp(key->ptr, "consume", key->len) == 0) {
        unsigned uint_buf = strtoumax(settingvalue, &crap, 10);
        rc = mpd_run_consume(mpd_state->conn, uint_buf));
    }
    else if (strncmp(key->ptr, "single", key->len) == 0) {
        unsigned uint_buf = strtoumax(settingvalue, &crap, 10);
        rc = mpd_run_single(mpd_state->conn, uint_buf));
    }
    else if (strncmp(key->ptr, "crossfade", key->len) == 0) {
        unsigned uint_buf = strtoumax(settingvalue, &crap, 10);
        rc = mpd_run_crossfade(mpd_state->conn, uint_buf));
    }
    else if (strncmp(key->ptr, "mixrampdb", key->len) == 0) {
        if (config->mixramp == true) {
            float float_buf = strtof(settingvalue, &crap, 10);
            rc = mpd_run_mixrampdb(mpd_state->conn, float_buf));
        }
    }
    else if (strncmp(key->ptr, "mixrampdelay", key->len) == 0) {
        if (config->mixramp == true) {
            float float_buf = strtof(settingvalue, &crap, 10);
            rc = mpd_run_mixrampdelay(mpd_state->conn, float_buf));
        }
    }
    else if (strncmp(key->ptr, "replaygain", key->len) == 0) {
        rc = mpd_send_command(mpd_state->conn, "replay_gain_mode", settingvalue, NULL));
    }    
    else {
        LOG_ERROR("Setting with name \"%s\" not supported", settingname);
        sds_free(settingvalue);
        return false;
    }

    sds_free(settingvalue);
    return rc;
}

int mpd_client_put_settings(t_mpd_state *mpd_state, char *buffer) {
    char *replaygain = NULL;
    size_t len, nr;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    
    struct mpd_status *status = mpd_run_status(mpd_state->conn);
    if (!status) {
        RETURN_ERROR_AND_RECOVER("mpd_run_status");
    }

    if (!mpd_send_command(mpd_state->conn, "replay_gain_status", NULL)) {
        RETURN_ERROR_AND_RECOVER("replay_gain_status");
    }
    struct mpd_pair *pair = mpd_recv_pair(mpd_state->conn);
    if (!pair) {
        RETURN_ERROR_AND_RECOVER("replay_gain_status");
    }
    replaygain = strdup(pair->value);
    mpd_return_pair(mpd_state->conn, pair);
    mpd_response_finish(mpd_state->conn);
    
    len = json_printf(&out, "{type: settings, data: {"
        "repeat: %d, single: %d, crossfade: %d, consume: %d, random: %d, "
        "mixrampdb: %f, mixrampdelay: %f, replaygain: %Q, featPlaylists: %B,"
        "featTags: %B, featLibrary: %B, featAdvsearch: %B, featStickers: %B,"
        "featSmartpls: %B, featLove: %B, featCoverimage: %B, featFingerprint: %B, "
        "musicDirectoryValue: %Q, mpdConnected: %B, tags: [", 
        mpd_status_get_repeat(status),
        mpd_status_get_single(status),
        mpd_status_get_crossfade(status),
        mpd_status_get_consume(status),
        mpd_status_get_random(status),
        mpd_status_get_mixrampdb(status),
        mpd_status_get_mixrampdelay(status),
        replaygain == NULL ? "" : replaygain,
        mpd_state->feat_playlists,
        mpd_state->feat_tags,
        mpd_state->feat_library,
        mpd_state->feat_advsearch,
        mpd_state->feat_sticker,
        mpd_state->feat_smartpls,
        mpd_state->feat_love,
        mpd_state->feat_coverimage,
        mpd_state->feat_fingerprint,
        mpd_state->music_directory_value,
        true
    );
    mpd_status_free(status);
    FREE_PTR(replaygain);
    
    for (nr = 0; nr < mpd_state->mympd_tag_types->len; nr++) {
        if (nr > 0) 
            len += json_printf(&out, ",");
        len += json_printf(&out, "%Q", mpd_tag_name(mpd_state->mympd_tag_types->tags[nr]));
    }
    
    len += json_printf(&out, "], searchtags: [");
        for (nr = 0; nr < mpd_state->search_tag_types->len; nr++) {
        if (nr > 0) 
            len += json_printf(&out, ",");
        len += json_printf(&out, "%Q", mpd_tag_name(mpd_state->search_tag_types->tags[nr]));
    }
    
    len += json_printf(&out, "], browsetags: [");
    for (nr = 0; nr < mpd_state->browse_tag_types->len; nr++) {
        if (nr > 0) 
            len += json_printf(&out, ",");
        len += json_printf(&out, "%Q", mpd_tag_name(mpd_state->browse_tag_types->tags[nr]));
    }

    len += json_printf(&out, "], allmpdtags: [");
    for (nr = 0; nr < mpd_state->mpd_tag_types->len; nr++) {
        if (nr > 0) 
            len += json_printf(&out, ",");
        len += json_printf(&out, "%Q", mpd_tag_name(mpd_state->mpd_tag_types->tags[nr]));
    }

    len += json_printf(&out, "]}}");
    
    CHECK_RETURN_LEN();
}
