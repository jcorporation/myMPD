/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Album cache
 */

#include "compile_time.h"
#include "src/lib/album.h"

#include "src/lib/convert.h"
#include "src/lib/mem.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"
#include "src/mympd_client/tags.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

// Private definitions

static sds get_tag_value_string(const struct t_album *album, enum mpd_tag_type tag,
        sds tag_values, unsigned *value_count);
static sds get_tag_values(const struct t_album *album, enum mpd_tag_type tag,
        sds tag_values, bool multi, unsigned *value_count);

/**
 * Structure representing an album tag value
 */
struct t_album_tag_value {
    struct t_album_tag_value *next;  //!< Next value
    char *value;                     //!< The value
};

/**
 * Structure representing an album.
 * The values are inherited / copied from mpd songs.
 */
struct t_album {
    char *uri;                                      //!< First song uri, used to fetch AlbumArt
    struct t_album_tag_value tags[MPD_TAG_COUNT];   //!< Tag values (same struct as it is for mpd_song)
    unsigned total_time;                            //!< Total disc playtime
    unsigned disc_count;                            //!< Number of discs
    unsigned song_count;                            //!< Number of songs
    time_t last_modified;                           //!< Latest last-modified time of all songs in this album
    time_t added;                                   //!< Earliest added time of all songs in this album
};

// Public functions

/**
 * Creates and initializes a new struct for an album.
 * @return struct t_album* or NULL on error
 */
struct t_album *album_new(void) {
    return album_new_uri("albumid");
}

/**
 * Creates and initializes a new struct for an album with defined first song uri
 * @param uri First song uri
 * @return struct t_album* or NULL on error
 */
struct t_album *album_new_uri(const char *uri) {
    assert(uri);
    struct t_album *album = malloc(sizeof(struct t_album));
    album->uri = strdup(uri);

    for (unsigned i = 0; i < MPD_TAG_COUNT; ++i) {
        album->tags[i].value = NULL;
    }

    album->total_time = 0;
    album->disc_count = 0;
    album->song_count = 0;
    album->last_modified = 0;
    album->added = 0;
    return album;
}

/**
 * Creates and initializes a new struct for an album with values copied from song
 * @param song Song to create album from
 * @param album_tags Tags to use
 * @return struct t_album* or NULL on error
 */
struct t_album *album_new_from_song(const struct mpd_song *song, const struct t_mympd_mpd_tags *album_tags) {
    struct t_album *album = malloc(sizeof(struct t_album));
    album->uri = strdup(mpd_song_get_uri(song));

    for (unsigned i = 0; i < MPD_TAG_COUNT; ++i) {
        album->tags[i].value = NULL;
    }

    for (unsigned tagnr = 0; tagnr < album_tags->len; ++tagnr) {
        const char *value;
        enum mpd_tag_type tag = album_tags->tags[tagnr];
        unsigned value_nr = 0;
        while ((value = mpd_song_get_tag(song, tag, value_nr)) != NULL) {
            if (album_append_tag(album, tag, value) == false) {
                album_free(album);
                return NULL;
            }
            value_nr++;
        }
    }

    album->total_time = mpd_song_get_duration(song);
    album->disc_count = 0;
    album->song_count = 1;
    album->last_modified = mpd_song_get_last_modified(song);
    album->added = mpd_song_get_added(song);
    return album;
}

/**
 * Frees an album struct
 * @param album Pointer to album struct
 */
void album_free(struct t_album *album) {
    assert(album != NULL);
    free(album->uri);
    for (unsigned i = 0; i < MPD_TAG_COUNT; ++i) {
        struct t_album_tag_value *tag = &album->tags[i];
        struct t_album_tag_value *next;
        if (tag->value == NULL) {
            continue;
        }
        free(tag->value);
        tag = tag->next;
        while (tag != NULL) {
            assert(tag->value != NULL);
            free(tag->value);

            next = tag->next;
            free(tag);
            tag = next;
        }
    }
    free(album);
}

/**
 * Gets the album uri (uri of first song)
 * @param album t_album struct representing the album
 * @return const char* first song uri
 */
const char *album_get_uri(const struct t_album *album) {
    return album->uri;
}

/**
 * Gets the album duration
 * @param album t_album struct representing the album
 * @return unsigned album duration
 */
unsigned album_get_total_time(const struct t_album *album) {
    return album->total_time;
}

/**
 * Gets the last-modified timestamp for the album
 * @param album t_album struct representing the album
 * @return time_t last-modified timestamp
 */
time_t album_get_last_modified(const struct t_album *album) {
    return album->last_modified;
}

/**
 * Gets the added timestamp for the album
 * @param album t_album struct representing the album
 * @return time_t added timestamp
 */
time_t album_get_added(const struct t_album *album) {
    return album->added;
}

/**
 * Gets the number of songs
 * @param album t_album struct representing the album
 * @return number of songs
 */
unsigned album_get_song_count(const struct t_album *album) {
    return album->song_count;
}

/**
 * Gets the number of discs
 * @param album t_album struct representing the album
 * @return number of discs
 */
unsigned album_get_disc_count(const struct t_album *album) {
    return album->disc_count;
}

/**
 * Gets the album tag value at position idx
 * @param album t_album struct representing the album
 * @param type mpd tag type
 * @param idx index of tag value to get
 * @return const char* tag value or NULL
 */
const char *album_get_tag(const struct t_album *album, enum mpd_tag_type type, unsigned idx) {
    const struct t_album_tag_value *tag = &album->tags[type];

    if ((int)type < 0 ||
        tag->value == NULL)
    {
        return NULL;
    }

    while (idx-- > 0) {
        tag = tag->next;
        if (tag == NULL) {
            return NULL;
        }
    }
    return tag->value;
}

/**
 * Sets the albums disc count from disc song tag
 * @param album t_album struct representing the album
 * @param disc mpd song disc tag value
 */
void album_set_discs(struct t_album *album, const char *disc) {
    if (disc == NULL) {
        return;
    }
    unsigned d;
    enum str2int_errno rc = str2uint(&d, disc);
    if (rc == STR2INT_SUCCESS && 
        d > album->disc_count)
    {
        album->disc_count = d;
    }
}

/**
 * Sets a fixed disc count
 * @param album t_album struct representing the album
 * @param count disc count
 */
void album_set_disc_count(struct t_album *album, unsigned count) {
    album->disc_count = count;
}

/**
 * Sets the albums last modified timestamp if it is newer as current album last-modified timestamp
 * @param album t_album struct representing the album
 * @param last_modified last-modified timestamp to set
 */
void album_set_last_modified(struct t_album *album, time_t last_modified) {
    if (album->last_modified < last_modified) {
        album->last_modified = last_modified;
    }
}

/**
 * Sets the albums added timestamp if it is older as current album added timestamp
 * @param album t_album struct representing the album
 * @param added added timestamp to set
 */
void album_set_added(struct t_album *album, time_t added) {
    if (album->added > added) {
        album->added = added;
    }
}

/**
 * Sets the albums duration
 * @param album t_album struct representing the album
 * @param duration total time to set
 */
void album_set_total_time(struct t_album *album, unsigned duration) {
    album->total_time = duration;
}

/**
 * Increments the albums duration
 * @param album t_album struct representing the album
 * @param duration seconds to increment
 */
void album_inc_total_time(struct t_album *album, unsigned duration) {
    album->total_time += duration;
}

/**
 * Set the song count
 * @param album t_album struct representing the album
 * @param count song count
 */
void album_set_song_count(struct t_album *album, unsigned count) {
    album->song_count = count;
}

/**
 * Increments the song count
 * @param album pointer to a t_album struct
 */
void album_inc_song_count(struct t_album *album) {
    album->song_count++;
}

/**
 * Appends tag values from a song to the album
 * @param album pointer to a t_album struct representing the album
 * @param song song to add tag values from
 * @param tags tags to append
 * @return true on success else false
 */
bool album_append_tags(struct t_album *album,
        const struct mpd_song *song, const struct t_mympd_mpd_tags *tags)
{
    for (unsigned tagnr = 0; tagnr < tags->len; ++tagnr) {
        const char *value;
        enum mpd_tag_type tag = tags->tags[tagnr];
        //append only multivalue tags
        if (is_multivalue_tag(tag) == true) {
            unsigned value_nr = 0;
            while ((value = mpd_song_get_tag(song, tag, value_nr)) != NULL) {
                if (album_append_tag(album, tag, value) == false) {
                    return false;
                }
                value_nr++;
            }
        }
    }
    return true;
}

/**
 * Adds a tag value to the album if value does not already exists
 * @param album pointer to a t_album struct
 * @param type mpd tag type
 * @param value tag value to add
 * @return true if tag is added or already there,
 *         false if the tag could not be added
 */
bool album_append_tag(struct t_album *album, enum mpd_tag_type type, const char *value) {
    struct t_album_tag_value *tag = &album->tags[type];

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
        struct t_album_tag_value *prev = tag;
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
 * Copies all values from a tag to another tag
 * @param album pointer to a t_album struct
 * @param src source tag
 * @param dst destination tag
 * @return true on success, else false
 */
bool album_copy_tags(struct t_album *album, enum mpd_tag_type src, enum mpd_tag_type dst) {
    const char *value;
    unsigned value_nr = 0;
    while ((value = album_get_tag(album, src, value_nr)) != NULL) {
        if (album_append_tag(album, dst, value) == false) {
            return false;
        }
        value_nr++;
    }
    return true;
}

/**
 * Replaces the uri
 * @param album pointer to album struct
 * @param uri new uri to set
 */
void album_set_uri(struct t_album *album, const char *uri) {
    size_t len = strlen(uri);
    album->uri = realloc_assert(album->uri, len + 1);
    memcpy(album->uri, uri, len);
    album->uri[len] = '\0';
}

/**
 * Appends a comma separated list of tag values
 * @param album pointer to album struct
 * @param tag mpd tag type to get values for
 * @param tag_values already allocated sds string to append the values
 * @return new sds pointer to tag_values
 */
sds album_get_tag_value_string(const struct t_album *album, enum mpd_tag_type tag, sds tag_values) {
    unsigned value_count = 0;
    tag_values = get_tag_value_string(album, tag, tag_values, &value_count);
    if (value_count == 0) {
        if (tag == MPD_TAG_TITLE) {
            //title fallback to name
            tag_values = get_tag_value_string(album, MPD_TAG_NAME, tag_values, &value_count);
            if (value_count == 0) {
                //title fallback to filename
                tag_values = sdscat(tag_values, album_get_uri(album));
                basename_uri(tag_values);
            }
        }
    }
    return tag_values;
}

/**
 * Appends a a json string/array of tag values
 * @param album pointer to album struct
 * @param tag mpd tag type to get values for
 * @param tag_values already allocated sds string to append the values
 * @return new sds pointer to tag_values
 */
sds album_get_tag_values(const struct t_album *album, enum mpd_tag_type tag, sds tag_values) {
    const bool multi = is_multivalue_tag(tag);
    unsigned value_count = 0;
    tag_values = get_tag_values(album, tag, tag_values, multi, &value_count);
    if (value_count == 0) {
        //set empty tag value(s)
        tag_values = multi == true
            ? sdscatlen(tag_values, "[]", 2)
            : sdscatlen(tag_values, "\"\"", 2);
    }
    return tag_values;
}

// Private functions

/**
 * Appends a comma separated list of tag values
 * @param album pointer to album struct
 * @param tag mpd tag type to get values for
 * @param tag_values already allocated sds string to append the values
 * @param value_count the number of values retrieved
 * @return new sds pointer to tag_values
 */
static sds get_tag_value_string(const struct t_album *album, enum mpd_tag_type tag,
        sds tag_values, unsigned *value_count)
{
    const char *value;
    unsigned count = 0;
    //return comma separated tag list
    while ((value = album_get_tag(album, tag, count)) != NULL) {
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
 * @param album pointer to album struct
 * @param tag mpd tag type to get values for
 * @param tag_values already allocated sds string to append the values
 * @param value_count the number of values retrieved
 * @param multi true if it is a multi value string
 * @return new sds pointer to tag_values
 */
static sds get_tag_values(const struct t_album *album, enum mpd_tag_type tag,
        sds tag_values, bool multi, unsigned *value_count)
{
    const char *value;
    unsigned count = 0;
    size_t org_len = sdslen(tag_values);
    if (multi == true) {
        //return json array
        tag_values = sdscatlen(tag_values, "[", 1);
        if ((tag == MPD_TAG_MUSICBRAINZ_ALBUMARTISTID || tag == MPD_TAG_MUSICBRAINZ_ARTISTID) &&
            (value = album_get_tag(album, tag, 0)) != NULL &&
            album_get_tag(album, tag, 1) == NULL)
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
            while ((value = album_get_tag(album, tag, count)) != NULL) {
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
        while ((value = album_get_tag(album, tag, count)) != NULL) {
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
