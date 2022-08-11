/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "tags.h"

#include "../../dist/libmpdclient/src/isong.h"
#include "../../dist/utf8/utf8.h"
#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/mem.h"
#include "../lib/sds_extras.h"
#include "../lib/utility.h"
#include "errorhandler.h"

#include <string.h>

/**
 * Private definitions
 */

static sds _mpd_client_get_tag_value_string(const struct mpd_song *song, enum mpd_tag_type tag,
        sds tag_values, unsigned *value_count);
static sds _mpd_client_get_tag_values(const struct mpd_song *song, enum mpd_tag_type tag,
        sds tag_values, bool multi, unsigned *value_count);

/**
 * Public functions
 */

/**
 * Adds a tag value to the album if value does not already exists
 * @param song pointer to a mpd_song struct
 * @param type mpd tag type
 * @param value tag value to add
 * @return true if tag is added or already there,
 *         false if the tag could not be added
 */
bool mympd_mpd_song_add_tag_dedup(struct mpd_song *song,
		enum mpd_tag_type type, const char *value)
{
	struct mpd_tag_value *tag = &song->tags[type];

	if ((int)type < 0 ||
        type >= MPD_TAG_COUNT)
    {
		return false;
    }

    if (tag->value == NULL) {
		tag->next = NULL;
		tag->value = strdup(value);
		if (tag->value == NULL) {
			return false;
        }
	}
    else {
		while (tag->next != NULL) {
            if (strcmp(tag->value, value) == 0) {
                //do not add duplicate values
                return true;
            }
			tag = tag->next;
        }
        if (strcmp(tag->value, value) == 0) {
            //do not add duplicate values
            return true;
        }
		struct mpd_tag_value *prev = tag;
		tag = malloc_assert(sizeof(*tag));

		tag->value = strdup(value);
		if (tag->value == NULL) {
		FREE_PTR(tag);
			return false;
		}

		tag->next = NULL;
		prev->next = tag;
	}

	return true;
}

/**
 * Checks if tag is a multivalue tag
 * @param tag mpd tag type
 * @return true if it is a multivalue tag else false
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
 * Maps a tag to its sort tag pedant and checks if the sort tag is enabled.
 * @param tag mpd tag type
 * @param available_tags pointer to enabled tags
 * @return sort tag if exists else the original tag
 */
enum mpd_tag_type get_sort_tag(enum mpd_tag_type tag, const struct t_tags *available_tags) {
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
    return mpd_client_tag_exists(available_tags, sort_tag) == true ? sort_tag : tag;
}

/**
 * Disables all mpd tags
 * @param partition_state pointer to partition specific states
 */
void disable_all_mpd_tags(struct t_partition_state *partition_state) {
    MYMPD_LOG_DEBUG("Disabling all mpd tag types");
    bool rc = mpd_run_clear_tag_types(partition_state->conn);
    mympd_check_rc_error_and_recover(partition_state, rc, "mpd_run_clear_tag_types");
}

/**
 * Enables all mpd tags
 * @param partition_state pointer to partition specific states
 */
void enable_all_mpd_tags(struct t_partition_state *partition_state) {
    MYMPD_LOG_DEBUG("Enabling all mpd tag types");
    bool rc = mpd_run_all_tag_types(partition_state->conn);
    mympd_check_rc_error_and_recover(partition_state, rc, "mpd_run_all_tag_types");
}

/**
 * Enables specific mpd tags
 * @param partition_state pointer to partition specific states
 * @param enable_tags pointer to t_tags struct
 */
void enable_mpd_tags(struct t_partition_state *partition_state, const struct t_tags *enable_tags) {
    MYMPD_LOG_DEBUG("Setting interesting mpd tag types");
    if (mpd_command_list_begin(partition_state->conn, false)) {
        bool rc = mpd_send_clear_tag_types(partition_state->conn);
        if (rc == false) {
            MYMPD_LOG_ERROR("Error adding command to command list mpd_send_clear_tag_types");
        }
        if (enable_tags->len > 0) {
            rc = mpd_send_enable_tag_types(partition_state->conn, enable_tags->tags, (unsigned)enable_tags->len);
            if (rc == false) {
                MYMPD_LOG_ERROR("Error adding command to command list mpd_send_enable_tag_types");
            }
        }
        if (mpd_command_list_end(partition_state->conn)) {
            mpd_response_finish(partition_state->conn);
        }
    }
    mympd_check_error_and_recover(partition_state);
}

/**
 * Appends a comma separated list of tag values
 * @param song pointer to mpd song struct
 * @param tag mpd tag type to get values for
 * @param tag_values alread allocated sds string to append the values
 * @return new sds pointer to tag_values
 */
sds mpd_client_get_tag_value_string(const struct mpd_song *song, enum mpd_tag_type tag, sds tag_values) {
    unsigned value_count = 0;
    tag_values = _mpd_client_get_tag_value_string(song, tag, tag_values, &value_count);
    if (value_count == 0) {
        if (tag == MPD_TAG_TITLE) {
            //title fallback to name
            tag_values = _mpd_client_get_tag_value_string(song, MPD_TAG_NAME, tag_values, &value_count);
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
 * @param tag_values alread allocated sds string to append the values
 * @return new sds pointer to tag_values
 */
sds mpd_client_get_tag_values(const struct mpd_song *song, enum mpd_tag_type tag, sds tag_values) {
    const bool multi = is_multivalue_tag(tag);
    unsigned value_count = 0;
    tag_values = _mpd_client_get_tag_values(song, tag, tag_values, multi, &value_count);
    if (value_count == 0) {
        if (tag == MPD_TAG_TITLE) {
            //title fallback to name
            tag_values = _mpd_client_get_tag_values(song, MPD_TAG_NAME, tag_values, multi, &value_count);
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
            //replace empty tag value(s) with dash
            if (multi == true) {
                tag_values = sdscatlen(tag_values, "[\"-\"]", 5);
            }
            else {
                tag_values = sdscatlen(tag_values, "\"-\"", 3);
            }
        }
    }
    return tag_values;
}

/**
 * Gets the tag values for a mpd song as json string
 * @param buffer alread allocated sds string to append the values
 * @param partition_state pointer to partition specific states
 * @param tagcols pointer to t_tags struct (tags to retrieve)
 * @param song pointer to a mpd_song struct to retrieve tags from
 * @return new sds pointer to buffer
 */
sds get_song_tags(sds buffer, struct t_partition_state *partition_state, const struct t_tags *tagcols,
                  const struct mpd_song *song)
{
    if (partition_state->mpd_state->feat_tags == true) {
        for (unsigned tagnr = 0; tagnr < tagcols->len; ++tagnr) {
            buffer = sdscatfmt(buffer, "\"%s\":", mpd_tag_name(tagcols->tags[tagnr]));
            buffer = mpd_client_get_tag_values(song, tagcols->tags[tagnr], buffer);
            buffer = sdscatlen(buffer, ",", 1);
        }
    }
    else {
        buffer = sdscat(buffer, "\"Title\":");
        buffer = mpd_client_get_tag_values(song, MPD_TAG_TITLE, buffer);
        buffer = sdscatlen(buffer, ",", 1);
    }

    buffer = tojson_uint(buffer, "Duration", mpd_song_get_duration(song), true);
    buffer = tojson_llong(buffer, "LastModified", (long long)mpd_song_get_last_modified(song), true);
    buffer = tojson_char(buffer, "uri", mpd_song_get_uri(song), false);
    return buffer;
}

/**
 * Gets the same json string as get_song_tags but with empty values, title is set to the basefilename
 * @param buffer alread allocated sds string to append the values
 * @param partition_state pointer to partition specific states
 * @param tagcols pointer to t_tags struct (tags to retrieve)
 * @param uri uri for printing empty tags
 * @return new sds pointer to buffer
 */
sds get_empty_song_tags(sds buffer, struct t_partition_state *partition_state, const struct t_tags *tagcols,
                        const char *uri)
{
    sds filename = sdsnew(uri);
    basename_uri(filename);
    if (partition_state->mpd_state->feat_tags == true) {
        for (unsigned tagnr = 0; tagnr < tagcols->len; ++tagnr) {
            const bool multi = is_multivalue_tag(tagcols->tags[tagnr]);
            buffer = sdscatfmt(buffer, "\"%s\":", mpd_tag_name(tagcols->tags[tagnr]));
            if (multi == true) {
                buffer = sdscatlen(buffer, "[", 1);
            }
            if (tagcols->tags[tagnr] == MPD_TAG_TITLE) {
                buffer = sds_catjson(buffer, filename, sdslen(filename));
            }
            else {
                buffer = sdscatlen(buffer, "\"-\"", 3);
            }
            if (multi == true) {
                buffer = sdscatlen(buffer, "]", 1);
            }
            buffer = sdscatlen(buffer, ",", 1);
        }
    }
    else {
        buffer = tojson_sds(buffer, "Title", filename, true);
    }
    buffer = tojson_long(buffer, "Duration", 0, true);
    buffer = tojson_long(buffer, "LastModified", 0, true);
    buffer = tojson_char(buffer, "uri", uri, false);
    FREE_SDS(filename);
    return buffer;
}

/**
 * Prints the audioformat as json object
 * @param buffer alread allocated sds string to append the values
 * @param audioformat pointer to t_tags struct (tags to retrieve)
 * @return new sds pointer to buffer
 */
sds printAudioFormat(sds buffer, const struct mpd_audio_format *audioformat) {
    buffer = sdscat(buffer, "\"AudioFormat\":{");
    buffer = tojson_uint(buffer, "sampleRate", (audioformat ? audioformat->sample_rate : 0), true);
    buffer = tojson_long(buffer, "bits", (audioformat ? audioformat->bits : 0), true);
    buffer = tojson_long(buffer, "channels", (audioformat ? audioformat->channels : 0), false);
    buffer = sdscatlen(buffer, "}", 1);
    return buffer;
}

/**
 * Parses a taglist and adds valid values to tagtypes struct 
 * @param taglist comma separated tags to check
 * @param taglistname descriptive name of taglist
 * @param tagtypes pointer to t_tags struct to add tags from taglist
 * @param allowed_tag_types pointer to t_tags struct for allowed tags
 */
void check_tags(sds taglist, const char *taglistname, struct t_tags *tagtypes,
                const struct t_tags *allowed_tag_types)
{
    sds logline = sdscatfmt(sdsempty(), "Enabled %s: ", taglistname);
    int tokens_count = 0;
    sds *tokens = sdssplitlen(taglist, (ssize_t)sdslen(taglist), ",", 1, &tokens_count);
    for (int i = 0; i < tokens_count; i++) {
        sdstrim(tokens[i], " ");
        enum mpd_tag_type tag = mpd_tag_name_iparse(tokens[i]);
        if (tag == MPD_TAG_UNKNOWN) {
            MYMPD_LOG_WARN("Unknown tag %s", tokens[i]);
        }
        else {
            if (mpd_client_tag_exists(allowed_tag_types, tag) == true) {
                logline = sdscatfmt(logline, "%s ", mpd_tag_name(tag));
                tagtypes->tags[tagtypes->len++] = tag;
            }
            else {
                MYMPD_LOG_DEBUG("Disabling tag %s", mpd_tag_name(tag));
            }
        }
    }
    sdsfreesplitres(tokens, tokens_count);
    MYMPD_LOG_NOTICE("%s", logline);
    FREE_SDS(logline);
}

/**
 * Checks if tag exists in a tagtypes struct
 * @param tagtypes tag list to check against
 * @param tag tag to check
 * @return true if tag is in tagtypes else false
 */
bool mpd_client_tag_exists(const struct t_tags *tagtypes, enum mpd_tag_type tag) {
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
 * @param tag_values alread allocated sds string to append the values
 * @param value_count the number of values retrieved
 * @return new sds pointer to tag_values
 */
static sds _mpd_client_get_tag_value_string(const struct mpd_song *song, enum mpd_tag_type tag,
        sds tag_values, unsigned *value_count)
{
    const char *value;
    unsigned count = 0;
    //return json string
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
 * Appends a json string or array to tag_values
 * @param song pointer to mpd song struct
 * @param tag mpd tag type to get values for
 * @param tag_values alread allocated sds string to append the values
 * @param value_count the number of values retrieved
 * @param multi true if it is a multi value string
 * @return new sds pointer to tag_values
 */
static sds _mpd_client_get_tag_values(const struct mpd_song *song, enum mpd_tag_type tag,
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
