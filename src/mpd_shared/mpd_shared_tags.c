/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mpd_shared_tags.h"

#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/sds_extras.h"
#include "../lib/utility.h"
#include "../mpd_shared.h"

#include <string.h>

//private definitions
static sds _mpd_shared_get_tags(struct mpd_song const *song, const enum mpd_tag_type tag, sds tags);

//public functions
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
    if (tag == MPD_TAG_COMPOSER) {
        return MPD_TAG_COMPOSER_SORT;
    }
    return tag;
}

void copy_tag_types(struct t_tags *src_tag_list, struct t_tags *dst_tag_list) {
    memcpy((void *)dst_tag_list, (void *)src_tag_list, sizeof(struct t_tags));
}

void reset_t_tags(struct t_tags *tags) {
    tags->len = 0;
    memset(tags->tags, 0, sizeof(tags->tags));
}

void disable_all_mpd_tags(struct t_mpd_state *mpd_state) {
    if (mpd_connection_cmp_server_version(mpd_state->conn, 0, 21, 0) >= 0) {
        MYMPD_LOG_DEBUG("Disabling all mpd tag types");
        bool rc = mpd_run_clear_tag_types(mpd_state->conn);
        check_rc_error_and_recover(mpd_state, NULL, NULL, 0, false, rc, "mpd_run_clear_tag_types");
    }
}

void enable_all_mpd_tags(struct t_mpd_state *mpd_state) {
    if (mpd_connection_cmp_server_version(mpd_state->conn, 0, 21, 0) >= 0) {
        MYMPD_LOG_DEBUG("Enabling all mpd tag types");
        bool rc = mpd_run_all_tag_types(mpd_state->conn);
        check_rc_error_and_recover(mpd_state, NULL, NULL, 0, false, rc, "mpd_run_all_tag_types");
    }
}

void enable_mpd_tags(struct t_mpd_state *mpd_state, struct t_tags *enable_tags) {
    if (mpd_connection_cmp_server_version(mpd_state->conn, 0, 21, 0) >= 0) {
        MYMPD_LOG_DEBUG("Setting interesting mpd tag types");
        if (mpd_command_list_begin(mpd_state->conn, false)) {
            bool rc = mpd_send_clear_tag_types(mpd_state->conn);
            if (rc == false) {
                MYMPD_LOG_ERROR("Error adding command to command list mpd_send_clear_tag_types");
            }
            if (enable_tags->len > 0) {
                rc = mpd_send_enable_tag_types(mpd_state->conn, enable_tags->tags, enable_tags->len);
                if (rc == false) {
                    MYMPD_LOG_ERROR("Error adding command to command list mpd_send_enable_tag_types");
                }
            }
            if (mpd_command_list_end(mpd_state->conn)) {
                mpd_response_finish(mpd_state->conn);
            }
        }
        check_error_and_recover(mpd_state, NULL, NULL, 0);
    }
}

sds mpd_shared_get_tags(struct mpd_song const *song, const enum mpd_tag_type tag, sds tags) {
    tags = _mpd_shared_get_tags(song, tag, tags);
    if (sdslen(tags) == 0) {
        if (tag == MPD_TAG_TITLE) {
            //title fallback to filename
            tags = sdscat(tags, basename_uri((char *)mpd_song_get_uri(song)));
        }
        else if (tag == MPD_TAG_ALBUM_ARTIST) {
            //albumartist fallback to artist tag
            tags = _mpd_shared_get_tags(song, MPD_TAG_ARTIST, tags);
        }
        if (sdslen(tags) == 0) {
            tags = sdscatlen(tags, "-", 1);
        }
    }
    return tags;
}

sds put_song_tags(sds buffer, struct t_mpd_state *mpd_state, const struct t_tags *tagcols, 
                  const struct mpd_song *song)
{
    sds tag_value = sdsempty();
    if (mpd_state->feat_tags == true) {
        for (size_t tagnr = 0; tagnr < tagcols->len; ++tagnr) {
            tag_value = mpd_shared_get_tags(song, tagcols->tags[tagnr], tag_value);
            buffer = tojson_char(buffer, mpd_tag_name(tagcols->tags[tagnr]), tag_value, true);
        }
    }
    else {
        tag_value = mpd_shared_get_tags(song, MPD_TAG_TITLE, tag_value);
        buffer = tojson_char(buffer, "Title", tag_value, true);
    }
    sdsfree(tag_value);
    buffer = tojson_long(buffer, "Duration", mpd_song_get_duration(song), true);
    buffer = tojson_long(buffer, "LastModified", mpd_song_get_last_modified(song), true);
    buffer = tojson_char(buffer, "uri", mpd_song_get_uri(song), false);
    return buffer;
}

sds put_empty_song_tags(sds buffer, struct t_mpd_state *mpd_state, const struct t_tags *tagcols, 
                        const char *uri)
{
    if (mpd_state->feat_tags == true) {
        for (size_t tagnr = 0; tagnr < tagcols->len; ++tagnr) {
            if (tagcols->tags[tagnr] == MPD_TAG_TITLE) {
                buffer = tojson_char(buffer, "Title", basename_uri((char *)uri), true);
            }
            else {
                buffer = tojson_char(buffer, mpd_tag_name(tagcols->tags[tagnr]), "-", true);
            }
        }
    }
    else {
        buffer = tojson_char(buffer, "Title", basename_uri((char *)uri), true);
    }
    buffer = tojson_long(buffer, "Duration", 0, true);
    buffer = tojson_char(buffer, "uri", uri, false);
    return buffer;
}

void check_tags(sds taglist, const char *taglistname, struct t_tags *tagtypes,
                struct t_tags allowed_tag_types)
{
    sds logline = sdscatfmt(sdsempty(), "Enabled %s: ", taglistname);
    int tokens_count;
    sds *tokens = sdssplitlen(taglist, sdslen(taglist), ",", 1, &tokens_count);
    for (int i = 0; i < tokens_count; i++) {
        sdstrim(tokens[i], " ");
        enum mpd_tag_type tag = mpd_tag_name_iparse(tokens[i]);
        if (tag == MPD_TAG_UNKNOWN) {
            MYMPD_LOG_WARN("Unknown tag %s", tokens[i]);
        }
        else {
            if (mpd_shared_tag_exists(allowed_tag_types.tags, allowed_tag_types.len, tag) == true) {
                logline = sdscatfmt(logline, "%s ", mpd_tag_name(tag));
                tagtypes->tags[tagtypes->len++] = tag;
            }
            else {
                MYMPD_LOG_DEBUG("Disabling tag %s", mpd_tag_name(tag));
            }
        }
    }
    sdsfreesplitres(tokens, tokens_count);
    MYMPD_LOG_NOTICE(logline);
    sdsfree(logline);
}

bool mpd_shared_tag_exists(const enum mpd_tag_type tag_types[64], const size_t tag_types_len, 
                           const enum mpd_tag_type tag)
{
    for (size_t i = 0; i < tag_types_len; i++) {
        if (tag_types[i] == tag) {
            return true;
        }
    }
    return false;
}

void album_cache_free(rax **album_cache) {
    if (*album_cache == NULL) {
        MYMPD_LOG_DEBUG("Album cache is NULL not freeing anything");
        return;
    }
    raxIterator iter;
    raxStart(&iter, *album_cache);
    raxSeek(&iter, "^", NULL, 0);
    while (raxNext(&iter)) {
        mpd_song_free((struct mpd_song *)iter.data);
    }
    raxStop(&iter);
    raxFree(*album_cache);
    *album_cache = NULL;
}

//private functions
static sds _mpd_shared_get_tags(struct mpd_song const *song, const enum mpd_tag_type tag, sds tags) {
    tags = sdscrop(tags);
    char *value = NULL;
    int i = 0;
    while ((value = (char *)mpd_song_get_tag(song, tag, i)) != NULL) {
        if (i++) {
            tags = sdscatlen(tags, ", ", 2);
        }
        tags = sdscat(tags, value);
    }
    return tags;
}
