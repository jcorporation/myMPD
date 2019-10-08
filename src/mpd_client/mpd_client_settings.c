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
#include <inttypes.h>
#include <stdbool.h>
#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../../dist/src/frozen/frozen.h"
#include "../utility.h"
#include "../log.h"
#include "../list.h"
#include "config_defs.h"
#include "mpd_client_utility.h"
#include "mpd_client_settings.h"

//private defintions
static sds print_tags_array(sds buffer, const char *tagsname, t_tags tags);

//public functions
bool mpd_api_settings_set(t_config *config, t_mpd_state *mpd_state, struct json_token *key, 
                          struct json_token *val, bool *mpd_host_changed)
{
    bool rc = true;
    char *crap;
    sds settingvalue = sdscatlen(sdsempty(), val->ptr, val->len);
    LOG_DEBUG("Parse setting \"%.*s\" with value \"%.*s\"", key->len, key->ptr, val->len, val->ptr);
    if (strncmp(key->ptr, "mpdPass", key->len) == 0) {
        if (strncmp(val->ptr, "dontsetpassword", val->len) != 0) {
            *mpd_host_changed = true;
            mpd_state->mpd_pass = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
        }
        else {
            sdsfree(settingvalue);
            return true;
        }
    }
    else if (strncmp(key->ptr, "mpdHost", key->len) == 0) {
        if (strncmp(val->ptr, mpd_state->mpd_host, val->len) != 0) {
            *mpd_host_changed = true;
            mpd_state->mpd_host = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
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
        mpd_state->music_directory = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "jukeboxMode", key->len) == 0) {
        int jukebox_mode = strtoimax(settingvalue, &crap, 10);
        if (jukebox_mode < 0 || jukebox_mode > 2) {
            sdsfree(settingvalue);
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
            sdsfree(settingvalue);
            return false;
        }
        mpd_state->jukebox_queue_length = jukebox_queue_length;
    }
    else if (strncmp(key->ptr, "autoPlay", key->len) == 0) {
        mpd_state->auto_play = val->type == JSON_TYPE_TRUE ? true : false;
    }
    else if (strncmp(key->ptr, "coverimage", key->len) == 0) {
        mpd_state->coverimage = val->type == JSON_TYPE_TRUE ? true : false;
    }
    else if (strncmp(key->ptr, "coverimageName", key->len) == 0) {
        if (validate_string(settingvalue) && sdslen(settingvalue) > 0) {
            mpd_state->coverimage_name = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
        }
        else {
            sdsfree(settingvalue);
            return false;
        }
    }
    else if (strncmp(key->ptr, "love", key->len) == 0) {
        mpd_state->love = val->type == JSON_TYPE_TRUE ? true : false;
    }
    else if (strncmp(key->ptr, "loveChannel", key->len) == 0) {
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
        rc = mpd_run_single(mpd_state->conn, uint_buf);
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
        rc = mpd_send_command(mpd_state->conn, "replay_gain_mode", settingvalue, NULL);
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

    if (!mpd_send_command(mpd_state->conn, "replay_gain_status", NULL)) {
        buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
        return buffer;
    }
    struct mpd_pair *pair = mpd_recv_pair(mpd_state->conn);
    if (pair == NULL) {
        buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
        return buffer;
    }
    char *replaygain = strdup(pair->value);
    mpd_return_pair(mpd_state->conn, pair);
    mpd_response_finish(mpd_state->conn);
    
    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer, "{");
    buffer = tojson_long(buffer, "repeat", mpd_status_get_repeat(status), true);
    buffer = tojson_long(buffer, "single", mpd_status_get_single(status), true);
    buffer = tojson_long(buffer, "crossfade", mpd_status_get_crossfade(status), true);
    buffer = tojson_long(buffer, "random", mpd_status_get_random(status), true);
    buffer = tojson_float(buffer, "mixrampdb", mpd_status_get_mixrampdb(status), true);
    buffer = tojson_float(buffer, "mixrampdelay", mpd_status_get_mixrampdelay(status), true);
    buffer = tojson_char(buffer, "replaygain", replaygain == NULL ? "" : replaygain, true);
    buffer = tojson_bool(buffer, "featPlaylists", mpd_state->feat_playlists, true);
    buffer = tojson_bool(buffer, "featTags", mpd_state->feat_tags, true);
    buffer = tojson_bool(buffer, "featLibray", mpd_state->feat_library, true);
    buffer = tojson_bool(buffer, "featAdvsearch", mpd_state->feat_advsearch, true);
    buffer = tojson_bool(buffer, "featStickers", mpd_state->feat_sticker, true);
    buffer = tojson_bool(buffer, "featSmartpls", mpd_state->feat_smartpls, true);
    buffer = tojson_bool(buffer, "featLove", mpd_state->feat_love, true);
    buffer = tojson_bool(buffer, "featCoverimage", mpd_state->feat_coverimage, true);
    buffer = tojson_bool(buffer, "featFingerprint", mpd_state->feat_fingerprint, true);
    buffer = tojson_char(buffer, "musicDirectoryValue", mpd_state->music_directory_value, true);
    buffer = tojson_bool(buffer, "mpdConnected", true, true);
    mpd_status_free(status);
    FREE_PTR(replaygain);

    buffer = print_tags_array(buffer, "tags", mpd_state->mympd_tag_types);
    buffer = sdscat(buffer, ",");
    buffer = print_tags_array(buffer, "searchtags", mpd_state->search_tag_types);
    buffer = sdscat(buffer, ",");
    buffer = print_tags_array(buffer, "browsetags", mpd_state->browse_tag_types);
    buffer = sdscat(buffer, ",");
    buffer = print_tags_array(buffer, "allmpdtags", mpd_state->mpd_tag_types);

    buffer = sdscat(buffer, "}");
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
