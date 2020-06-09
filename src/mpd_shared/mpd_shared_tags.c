/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdlib.h>
#include <libgen.h>
#include <pthread.h>
#include <string.h>
#include <inttypes.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <mpd/client.h>

#include "../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../log.h"
#include "../list.h"
#include "config_defs.h"
#include "../utility.h"
#include "mpd_shared_typedefs.h"
#include "../mpd_shared.h"
#include "mpd_shared_tags.h"

enum mpd_tag_type get_sort_tag(enum mpd_tag_type tag) {
    if (tag == MPD_TAG_ARTIST) {
        return MPD_TAG_ARTIST_SORT;
    }
    if (tag == MPD_TAG_ALBUM_ARTIST) {
        return MPD_TAG_ALBUM_ARTIST_SORT;
    }
    if (tag == MPD_TAG_ALBUM) {
        return MPD_TAG_ALBUM_SORT;
    }

    return tag;
}

void reset_t_tags(t_tags *tags) {
    tags->len = 0;
    memset(tags->tags, 0, sizeof(tags->tags));
}

void disable_all_mpd_tags(t_mpd_state *mpd_state) {
    if (mpd_connection_cmp_server_version(mpd_state->conn, 0, 21, 0) >= 0) {
        LOG_DEBUG("Disabling all mpd tag types");
        bool rc = mpd_run_clear_tag_types(mpd_state->conn);
        check_rc_error_and_recover(mpd_state, NULL, NULL, 0, false, rc, "mpd_run_clear_tag_types");
    }
}

void enable_all_mpd_tags(t_mpd_state *mpd_state) {
    if (mpd_connection_cmp_server_version(mpd_state->conn, 0, 21, 0) >= 0) {
        LOG_DEBUG("Enabling all mpd tag types");
        bool rc = mpd_run_all_tag_types(mpd_state->conn);
        check_rc_error_and_recover(mpd_state, NULL, NULL, 0, false, rc, "mpd_run_all_tag_types");
    }
}

void enable_mpd_tags(t_mpd_state *mpd_state, t_tags enable_tags) {
    if (mpd_connection_cmp_server_version(mpd_state->conn, 0, 21, 0) >= 0) {
        LOG_DEBUG("Setting interesting mpd tag types");
        if (mpd_command_list_begin(mpd_state->conn, false)) {
            bool rc = mpd_send_clear_tag_types(mpd_state->conn);
            if (rc == false) {
                LOG_ERROR("Error adding command to command list mpd_send_clear_tag_types");
            }
            if (enable_tags.len > 0) {
                rc = mpd_send_enable_tag_types(mpd_state->conn, enable_tags.tags, enable_tags.len);
                if (rc == false) {
                    LOG_ERROR("Error adding command to command list mpd_send_enable_tag_types");
                }
            }
            if (mpd_command_list_end(mpd_state->conn)) {
                mpd_response_finish(mpd_state->conn);
            }
        }
        check_error_and_recover(mpd_state, NULL, NULL, 0);
    }
}

char *mpd_shared_get_tag(struct mpd_song const *song, const enum mpd_tag_type tag) {
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

sds put_song_tags(sds buffer, t_mpd_state *mpd_state, const t_tags *tagcols, const struct mpd_song *song) {
    if (mpd_state->feat_tags == true) {
        for (size_t tagnr = 0; tagnr < tagcols->len; ++tagnr) {
            char *tag_value = mpd_shared_get_tag(song, tagcols->tags[tagnr]);
            buffer = tojson_char(buffer, mpd_tag_name(tagcols->tags[tagnr]), tag_value == NULL ? "-" : tag_value, true);
        }
    }
    else {
        char *tag_value = mpd_shared_get_tag(song, MPD_TAG_TITLE);
        buffer = tojson_char(buffer, "Title", tag_value == NULL ? "-" : tag_value, true);
    }
    buffer = tojson_long(buffer, "Duration", mpd_song_get_duration(song), true);
    buffer = tojson_long(buffer, "LastModified", mpd_song_get_last_modified(song), true);
    buffer = tojson_char(buffer, "uri", mpd_song_get_uri(song), false);
    return buffer;
}

sds put_empty_song_tags(sds buffer, t_mpd_state *mpd_state, const t_tags *tagcols, const char *uri) {
    if (mpd_state->feat_tags == true) {
        for (size_t tagnr = 0; tagnr < tagcols->len; ++tagnr) {
            if (tagcols->tags[tagnr] == MPD_TAG_TITLE) {
                buffer = tojson_char(buffer, "Title", basename((char *)uri), true);
            }
            else {
                buffer = tojson_char(buffer, mpd_tag_name(tagcols->tags[tagnr]), "-", true);
            }
        }
    }
    else {
        buffer = tojson_char(buffer, "Title", basename((char *)uri), true);
    }
    buffer = tojson_long(buffer, "Duration", 0, true);
    buffer = tojson_char(buffer, "uri", uri, false);
    return buffer;
}

void check_tags(sds taglist, const char *taglistname, t_tags *tagtypes, t_tags allowed_tag_types) {
    sds logline = sdscatfmt(sdsempty(), "Enabled %s: ", taglistname);
    int tokens_count;
    sds *tokens = sdssplitlen(taglist, sdslen(taglist), ",", 1, &tokens_count);
    for (int i = 0; i < tokens_count; i++) {
        sdstrim(tokens[i], " ");
        enum mpd_tag_type tag = mpd_tag_name_iparse(tokens[i]);
        if (tag == MPD_TAG_UNKNOWN) {
            LOG_WARN("Unknown tag %s", tokens[i]);
        }
        else {
            if (mpd_shared_tag_exists(allowed_tag_types.tags, allowed_tag_types.len, tag) == true) {
                logline = sdscatfmt(logline, "%s ", mpd_tag_name(tag));
                tagtypes->tags[tagtypes->len++] = tag;
            }
            else {
                LOG_DEBUG("Disabling tag %s", mpd_tag_name(tag));
            }
        }
    }
    sdsfreesplitres(tokens, tokens_count);
    LOG_INFO(logline);
    sdsfree(logline);
}

bool mpd_shared_tag_exists(const enum mpd_tag_type tag_types[64], const size_t tag_types_len, const enum mpd_tag_type tag) {
    for (size_t i = 0; i < tag_types_len; i++) {
        if (tag_types[i] == tag) {
            return true;
        }
    }
    return false;
}
