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

#include <stdlib.h>
#include <libgen.h>
#include <pthread.h>
#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../utility.h"
#include "mpd_client_utils.h"

sds put_song_tags(sds buffer, t_mpd_state *mpd_state, const t_tags *tagcols, const struct mpd_song *song) {
    if (mpd_state->feat_tags == true) {
        for (int tagnr = 0; tagnr < tagcols->len; ++tagnr) {
            char *tag_value = mpd_client_get_tag(song, tagcols->tags[tagnr]);
            buffer = tojson_char(buffer, mpd_tag_name(tagcols->tags[tagnr]), tag_value == NULL ? "-" : tag_value, true);
        }
    }
    else {
        char *tag_value = mpd_client_get_tag(song, MPD_TAG_TITLE);
        buffer = tojson_char(buffer, "Title", tag_value == NULL ? "-" : tag_value, true);
    }
    buffer = tojson_long(buffer, "Duration", mpd_song_get_duration(song), true);
    buffer = tojson_long(buffer, "uri", mpd_song_get_uri(song), false);
}

sds check_error_and_recover(sds buffer, sds method, int request_id) {
    if (mpd_connection_get_error != MPD_ERROR_SUCCESS) {
        LOG_ERROR("MPD error: %s", mpd_connection_get_error_message(mpd_state->conn));
        if (buffer != NULL) {
            buffer = jsonrpc_respond_error(buffer, method, request_id, mpd_connection_get_error_message(mpd_state->conn));
        }
        if (!mpd_connection_clear_error(mpd_state->conn)) {
            mpd_state->conn_state = MPD_FAILURE;
        }
    }
    return buffer;
}

sds respond_with_mpd_error_or_ok(sds buffer, sds method, int request_id) {
    buffer = sdscat(sdsempty(), sdsempty());
    buffer = check_error_and_recover(buffer, method, request_id);
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
        char token[t.len + 1];
        snprintf(token, t.len + 1, "%.*s", t.len, t.ptr);
        enum mpd_tag_type tag = mpd_tag_name_iparse(token);
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

bool mpd_client_tag_exists(const enum mpd_tag_type tag_types[64], const size_t tag_types_len, const enum mpd_tag_type tag) {
    for (size_t i = 0; i < tag_types_len; i++) {
        if (tag_types[i] == tag) {
            return true;
        }
    }
    return false;
}

void reset_t_tags(tags) {
    tags->len = 0;
    memset(tags->tags, 0, sizeof(tags->tags));
}
