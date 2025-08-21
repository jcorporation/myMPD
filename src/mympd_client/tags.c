/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief MPD tags helper functions
 */

#include "compile_time.h"
#include "src/mympd_client/tags.h"

#include "src/lib/cache/cache_rax_album.h"
#include "src/lib/convert.h"
#include "src/lib/json/json_print.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"
#include "src/mympd_client/errorhandler.h"
#include "src/mympd_client/shortcuts.h"

#include <inttypes.h>
#include <string.h>

/**
 * Private definitions
 */

static sds get_tag_value_string(const struct mpd_song *song, enum mpd_tag_type tag,
        sds tag_values, unsigned *value_count);
static sds get_tag_values(const struct mpd_song *song, enum mpd_tag_type tag,
        sds tag_values, bool multi, unsigned *value_count);

/**
 * Public functions
 */

/**
 * Returns the mpd database last modification time
 * @param partition_state pointer to partition specific states
 * @return last modification time
 */
time_t mympd_client_get_db_mtime(struct t_partition_state *partition_state) {
    time_t mtime = 0;
    struct mpd_stats *stats = mpd_run_stats(partition_state->conn);
    if (stats != NULL) {
        mtime = (time_t)mpd_stats_get_db_update_time(stats);
        mpd_stats_free(stats);
    }
    if (mympd_check_error_and_recover(partition_state, NULL, "mpd_run_stats") == false) {
        mtime = 0;
    }
    return mtime;
}

/**
 * Checks if tag is a multivalue tag
 * @param tag mpd tag type
 * @return true if it is a multivalue tag, else false
 */
bool is_multivalue_tag(enum mpd_tag_type tag) {
    switch(tag) {
        case MPD_TAG_ARTIST:
        case MPD_TAG_ARTIST_SORT:
        case MPD_TAG_ALBUM_ARTIST:
        case MPD_TAG_ALBUM_ARTIST_SORT:
        case MPD_TAG_GENRE:
        case MPD_TAG_COMPOSER:
        case MPD_TAG_COMPOSER_SORT:
        case MPD_TAG_PERFORMER:
        case MPD_TAG_CONDUCTOR:
        case MPD_TAG_ENSEMBLE:
        case MPD_TAG_MUSICBRAINZ_ARTISTID:
        case MPD_TAG_MUSICBRAINZ_ALBUMARTISTID:
            return true;
        default:
            return false;
    }
}

/**
 * Checks if tag is a numeric tag
 * @param tag mpd tag type
 * @return true if it is a numeric tag, else false
 */
bool is_numeric_tag(enum mpd_tag_type tag) {
    switch(tag) {
        case MPD_TAG_DISC:
        case MPD_TAG_TRACK:
            return true;
        default:
            return false;
    }
}

/**
 * Maps a tag to its sort tag pendant and checks if the sort tag is enabled.
 * @param tag mpd tag type
 * @param available_tags pointer to enabled tags
 * @return sort tag if exists, else the original tag
 */
enum mpd_tag_type get_sort_tag(enum mpd_tag_type tag, const struct t_mympd_mpd_tags *available_tags) {
    enum mpd_tag_type sort_tag;
    switch(tag) {
        case MPD_TAG_ARTIST:
            sort_tag = MPD_TAG_ARTIST_SORT;
            break;
        case MPD_TAG_ALBUM_ARTIST:
            sort_tag = MPD_TAG_ALBUM_ARTIST_SORT;
            break;
        case MPD_TAG_ALBUM:
            sort_tag = MPD_TAG_ALBUM_SORT;
            break;
        case MPD_TAG_COMPOSER:
            sort_tag = MPD_TAG_COMPOSER_SORT;
            break;
        case MPD_TAG_TITLE:
            sort_tag = MPD_TAG_TITLE_SORT;
            break;
        default:
            return tag;
    }
    return mympd_client_tag_exists(available_tags, sort_tag) == true
        ? sort_tag
        : tag;
}

/**
 * Gets an alphanumeric string for sorting
 * @param key already allocated sds string to append
 * @param sort_by enum sort_by
 * @param sort_tag mpd tag to sort by
 * @param song pointer to mpd song
 * @return pointer to key
 */
sds get_sort_key(sds key, enum sort_by_type sort_by, enum mpd_tag_type sort_tag, const struct mpd_song *song) {
    if (sort_by == SORT_BY_LAST_MODIFIED) {
        key = sds_pad_int((int64_t)mpd_song_get_last_modified(song), key);
    }
    else if (sort_by == SORT_BY_ADDED) {
        key = sds_pad_int((int64_t)mpd_song_get_added(song), key);
    }
    else if (is_numeric_tag(sort_tag) == true) {
        key = mympd_client_get_tag_value_padded(song, sort_tag, '0', PADDING_LENGTH, key);
    }
    else if (sort_tag > MPD_TAG_UNKNOWN) {
        key = mympd_client_get_tag_value_string(song, sort_tag, key);
        if (sdslen(key) == 0) {
            key = sdscatlen(key, "zzzzzzzzzz", 10);
        }
    }
    key = sdscatfmt(key, "::%s", mpd_song_get_uri(song));
    sds_utf8_tolower(key);
    return key;
}

/**
 * Disables all mpd tags
 * @param partition_state pointer to partition specific states
 * @return true on success, else false
 */
bool disable_all_mpd_tags(struct t_partition_state *partition_state) {
    MYMPD_LOG_DEBUG(partition_state->name, "Disabling all mpd tag types");
    mpd_run_clear_tag_types(partition_state->conn);
    return mympd_check_error_and_recover(partition_state, NULL, "mpd_run_clear_tag_types");
}

/**
 * Enables all mpd tags
 * @param partition_state pointer to partition specific states
 * @return true on success, else false
 */
bool enable_all_mpd_tags(struct t_partition_state *partition_state) {
    MYMPD_LOG_DEBUG(partition_state->name, "Enabling all mpd tag types");
    mpd_run_all_tag_types(partition_state->conn);
    return mympd_check_error_and_recover(partition_state, NULL, "mpd_run_all_tag_types");
}

/**
 * Helper function to print athe tags of a t_fields struct as json array
 * @param buffer already allocated sds string to append the response
 * @param tagsname key for the json array
 * @param tags tags to print
 * @return pointer to buffer
 */
sds print_tags_array(sds buffer, const char *tagsname, const struct t_mympd_mpd_tags *tags) {
    buffer = sdscatfmt(buffer, "\"%s\": [", tagsname);
    for (unsigned i = 0; i < tags->len; i++) {
        if (i > 0) {
            buffer = sdscatlen(buffer, ",", 1);
        }
        const char *tagname = mpd_tag_name(tags->tags[i]);
        buffer = sds_catjson(buffer, tagname, strlen(tagname));
    }
    buffer = sdscatlen(buffer, "]", 1);
    return buffer;
}

/**
 * Enables only specified mpd tag types
 * @param partition_state pointer to partition specific states
 * @param enable_tags pointer to t_fields struct
 * @return true on success, else false
 */
bool enable_mpd_tags(struct t_partition_state *partition_state, const struct t_mympd_mpd_tags *enable_tags) {
    if (partition_state->mpd_state->feat.tags == false) {
        return true;
    }
    MYMPD_LOG_INFO(partition_state->name, "Setting interesting mpd tag types");
    if (partition_state->mpd_state->feat.mpd_0_24_0 == true) {
        mpd_run_reset_tag_types(partition_state->conn, enable_tags->tags, (unsigned)enable_tags->len);
    }
    else {
        if (mpd_command_list_begin(partition_state->conn, false)) {
            if (mpd_send_clear_tag_types(partition_state->conn) == false) {
                mympd_set_mpd_failure(partition_state, "Error adding command to command list mpd_send_clear_tag_types");
            }
            if (enable_tags->len > 0) {
                if (mpd_send_enable_tag_types(partition_state->conn, enable_tags->tags, (unsigned)enable_tags->len) == false) {
                    mympd_set_mpd_failure(partition_state, "Error adding command to command list mpd_send_enable_tag_types");
                }
            }
            else {
                MYMPD_LOG_WARN(partition_state->name, "No mpd tags are enabled");
            }
            mympd_client_command_list_end_check(partition_state);
        }
    }
    return mympd_check_error_and_recover(partition_state, NULL, "mpd_send_enable_tag_types");
}

/**
 * Get's a tag value from mpd song and converts it to int
 * @param song mpd song struct
 * @param tag mdp tag type
 * @return parsed tag value
 */
int mympd_client_get_tag_value_int(const struct mpd_song *song, enum mpd_tag_type tag) {
    const char *value = mpd_song_get_tag(song, tag, 0);
    if (value == NULL) {
        return 0;
    }
    int int_value;
    enum str2int_errno rc = str2int(&int_value, value);
    return rc == STR2INT_SUCCESS
        ? int_value
        : 0;
}

/**
 * Get's a tag value from mpd song and pads it
 * @param song mpd song struct
 * @param tag mpd tag type
 * @param pad padding char
 * @param len length to pad
 * @param tag_values already allocated sds string to append
 * @return sds new sds pointer to tag_values
 */
sds mympd_client_get_tag_value_padded(const struct mpd_song *song, enum mpd_tag_type tag, const char pad, size_t len, sds tag_values) {
    const char *value = mpd_song_get_tag(song, tag, 0);
    size_t value_len = value == NULL
        ? 0
        : strlen(value);
    if (value_len < len) {
        len = len - value_len;
        for (size_t i = 0; i < len; i++) {
            tag_values = sdscatfmt(tag_values, "%c", pad);
        }
    }
    if (value != NULL) {
        tag_values = sdscatlen(tag_values, value, value_len);
    }
    return tag_values;
}

/**
 * Appends a comma separated list of tag values
 * @param song pointer to mpd song struct
 * @param tag mpd tag type to get values for
 * @param tag_values already allocated sds string to append the values
 * @return new sds pointer to tag_values
 */
sds mympd_client_get_tag_value_string(const struct mpd_song *song, enum mpd_tag_type tag, sds tag_values) {
    unsigned value_count = 0;
    tag_values = get_tag_value_string(song, tag, tag_values, &value_count);
    if (value_count == 0) {
        if (tag == MPD_TAG_TITLE) {
            //title fallback to name
            tag_values = get_tag_value_string(song, MPD_TAG_NAME, tag_values, &value_count);
            if (value_count == 0) {
                //title fallback to filename
                tag_values = sdscat(tag_values, mpd_song_get_uri(song));
                basename_uri(tag_values);
            }
        }
    }
    return tag_values;
}

/**
 * Appends a a json string/array of tag values
 * @param song pointer to mpd song struct
 * @param tag mpd tag type to get values for
 * @param tag_values already allocated sds string to append the values
 * @return new sds pointer to tag_values
 */
sds mympd_client_get_tag_values(const struct mpd_song *song, enum mpd_tag_type tag, sds tag_values) {
    const bool multi = is_multivalue_tag(tag);
    unsigned value_count = 0;
    tag_values = get_tag_values(song, tag, tag_values, multi, &value_count);
    if (value_count == 0) {
        if (tag == MPD_TAG_TITLE) {
            //title fallback to name
            tag_values = get_tag_values(song, MPD_TAG_NAME, tag_values, multi, &value_count);
            if (value_count == 0) {
                //title fallback to filename
                sds filename = sdsnew(mpd_song_get_uri(song));
                basename_uri(filename);
                tag_values = sds_catjson(tag_values, filename, sdslen(filename));
                FREE_SDS(filename);
                value_count++;
            }
        }
        else {
            //set empty tag value(s)
            tag_values = multi == true
                ? sdscatlen(tag_values, "[]", 2)
                : sdscatlen(tag_values, "\"\"", 2);
        }
    }
    return tag_values;
}

/**
 * Prints the tag values for a mpd song as json string
 * @param buffer already allocated sds string to append the values
 * @param mpd_state pointer to mpd_state
 * @param tagcols pointer to t_fields struct (tags to retrieve)
 * @param song pointer to a mpd_song struct to retrieve tags from
 * @return new sds pointer to buffer
 */
sds print_song_tags(sds buffer, const struct t_mpd_state *mpd_state, const struct t_mympd_mpd_tags *tagcols,
        const struct mpd_song *song)
{
    const char *uri = mpd_song_get_uri(song);
    if (mpd_state->feat.tags == true) {
        for (unsigned tagnr = 0; tagnr < tagcols->len; ++tagnr) {
            buffer = sdscatfmt(buffer, "\"%s\":", mpd_tag_name(tagcols->tags[tagnr]));
            buffer = mympd_client_get_tag_values(song, tagcols->tags[tagnr], buffer);
            buffer = sdscatlen(buffer, ",", 1);
        }
        if (is_streamuri(uri) == false) {
            sds albumid = album_cache_get_key(sdsempty(), song, &mpd_state->config->albums);
            buffer = tojson_sds(buffer, "AlbumId", albumid, true);
            FREE_SDS(albumid);
        }
    }
    else {
        buffer = sdscat(buffer, "\"Title\":");
        buffer = mympd_client_get_tag_values(song, MPD_TAG_TITLE, buffer);
        buffer = sdscatlen(buffer, ",", 1);
    }
    buffer = tojson_uint(buffer, "Duration", mpd_song_get_duration(song), true);
    buffer = tojson_time(buffer, "Last-Modified", mpd_song_get_last_modified(song), true);
    if (mpd_state->feat.db_added == true) {
        buffer = tojson_time(buffer, "Added", mpd_song_get_added(song), true);
    }
    buffer = tojson_char(buffer, "uri", uri, false);
    return buffer;
}

/**
 * Prints the tag values for an album as json string
 * @param buffer already allocated sds string to append the values
 * @param mpd_state pointer to mpd_state
 * @param tagcols pointer to t_tags struct (tags to retrieve)
 * @param album pointer to a mpd_song struct representing the album
 * @return new sds pointer to buffer
 */
sds print_album_tags(sds buffer, const struct t_mpd_state *mpd_state, const struct t_mympd_mpd_tags *tagcols,
        const struct mpd_song *album)
{
    buffer = print_song_tags(buffer, mpd_state, tagcols, album);
    buffer = sdscatlen(buffer, ",", 1);
    buffer = tojson_uint(buffer, "Discs", album_get_discs(album), true);
    buffer = tojson_uint(buffer, "SongCount", album_get_song_count(album), false);
    return buffer;
}

/**
 * Prints the audioformat as json object
 * @param buffer already allocated sds string to append the values
 * @param audioformat pointer to t_fields struct (tags to retrieve)
 * @return new sds pointer to buffer
 */
sds printAudioFormat(sds buffer, const struct mpd_audio_format *audioformat) {
    buffer = sdscat(buffer, "\"AudioFormat\":{");
    buffer = tojson_uint(buffer, "sampleRate", (audioformat ? audioformat->sample_rate : 0), true);
    buffer = tojson_uint(buffer, "bits", (audioformat ? audioformat->bits : 0), true);
    buffer = tojson_uint(buffer, "channels", (audioformat ? audioformat->channels : 0), false);
    buffer = sdscatlen(buffer, "}", 1);
    return buffer;
}

/**
 * Parses a taglist and adds valid values to tagtypes struct 
 * @param taglist comma separated tags to check
 * @param taglistname descriptive name of taglist
 * @param tagtypes pointer to t_tags struct to add tags from taglist
 * @param allowed_tag_types pointer to t_fields struct for allowed tags
 */
void check_tags(sds taglist, const char *taglistname, struct t_mympd_mpd_tags *tagtypes,
                const struct t_mympd_mpd_tags *allowed_tag_types)
{
    sds logline = sdscatfmt(sdsempty(), "Enabled %s: ", taglistname);
    int tokens_count = 0;
    sds *tokens = sdssplitlen(taglist, (ssize_t)sdslen(taglist), ",", 1, &tokens_count);
    for (int i = 0; i < tokens_count; i++) {
        sdstrim(tokens[i], " ");
        enum mpd_tag_type tag = mpd_tag_name_iparse(tokens[i]);
        if (tag == MPD_TAG_UNKNOWN) {
            MYMPD_LOG_WARN(NULL, "Unknown tag %s", tokens[i]);
        }
        else {
            if (mympd_client_tag_exists(allowed_tag_types, tag) == true) {
                logline = sdscatfmt(logline, "%s ", mpd_tag_name(tag));
                tagtypes->tags[tagtypes->len++] = tag;
            }
            else {
                MYMPD_LOG_DEBUG(NULL, "Disabling tag %s", mpd_tag_name(tag));
            }
        }
    }
    sdsfreesplitres(tokens, tokens_count);
    MYMPD_LOG_NOTICE(NULL, "%s", logline);
    FREE_SDS(logline);
}

/**
 * Checks if tag exists in a tagtypes struct
 * @param tagtypes tag list to check against
 * @param tag tag to check
 * @return true if tag is in tagtypes else false
 */
bool mympd_client_tag_exists(const struct t_mympd_mpd_tags *tagtypes, enum mpd_tag_type tag) {
    for (size_t i = 0; i < tagtypes->len; i++) {
        if (tagtypes->tags[i] == tag) {
            return true;
        }
    }
    return false;
}

/**
 * Private functions
 */

/**
 * Appends a comma separated list of tag values
 * @param song pointer to mpd song struct
 * @param tag mpd tag type to get values for
 * @param tag_values already allocated sds string to append the values
 * @param value_count the number of values retrieved
 * @return new sds pointer to tag_values
 */
static sds get_tag_value_string(const struct mpd_song *song, enum mpd_tag_type tag,
        sds tag_values, unsigned *value_count)
{
    const char *value;
    unsigned count = 0;
    //return comma separated tag list
    while ((value = mpd_song_get_tag(song, tag, count)) != NULL) {
        if (count++) {
            tag_values = sdscatlen(tag_values, ", ", 2);
        }
        tag_values = sdscat(tag_values, value);
    }
    *value_count = count;
    return tag_values;
}

/**
 * Appends a json string or array to tag_values.
 * Nothing is append if value is empty.
 * @param song pointer to mpd song struct
 * @param tag mpd tag type to get values for
 * @param tag_values already allocated sds string to append the values
 * @param value_count the number of values retrieved
 * @param multi true if it is a multi value string
 * @return new sds pointer to tag_values
 */
static sds get_tag_values(const struct mpd_song *song, enum mpd_tag_type tag,
        sds tag_values, bool multi, unsigned *value_count)
{
    const char *value;
    unsigned count = 0;
    size_t org_len = sdslen(tag_values);
    if (multi == true) {
        //return json array
        tag_values = sdscatlen(tag_values, "[", 1);
        if ((tag == MPD_TAG_MUSICBRAINZ_ALBUMARTISTID || tag == MPD_TAG_MUSICBRAINZ_ARTISTID) &&
            (value = mpd_song_get_tag(song, tag, 0)) != NULL &&
            mpd_song_get_tag(song, tag, 1) == NULL)
        {
            //support semicolon separated MUSICBRAINZ_ARTISTID, MUSICBRAINZ_ALBUMARTISTID
            //workaround for https://github.com/MusicPlayerDaemon/MPD/issues/687
            int token_count = 0;
            sds *tokens = sdssplitlen(value, (ssize_t)strlen(value), ";", 1, &token_count);
            for (int j = 0; j < token_count; j++) {
                if (count++) {
                    tag_values = sdscatlen(tag_values, ",", 1);
                }
                sdstrim(tokens[j], " ");
                tag_values = sds_catjson(tag_values, tokens[j], sdslen(tokens[j]));
            }
            sdsfreesplitres(tokens, token_count);
        }
        else {
            while ((value = mpd_song_get_tag(song, tag, count)) != NULL) {
                if (count++) {
                    tag_values = sdscatlen(tag_values, ",", 1);
                }
                tag_values = sds_catjson(tag_values, value, strlen(value));
            }
        }
        if (count > 0) {
            tag_values = sdscatlen(tag_values, "]", 1);
        }
        else {
            sdssubstr(tag_values, 0, org_len);
        }
    }
    else {
        //return json string
        tag_values = sdscatlen(tag_values, "\"", 1);
        while ((value = mpd_song_get_tag(song, tag, count)) != NULL) {
            if (count++) {
                tag_values = sdscatlen(tag_values, ", ", 2);
            }
            tag_values = sds_catjson_plain(tag_values, value, strlen(value));
        }
        if (count > 0) {
            tag_values = sdscatlen(tag_values, "\"", 1);
        }
        else {
            sdssubstr(tag_values, 0, org_len);
        }
    }
    *value_count = count;
    return tag_values;
}
