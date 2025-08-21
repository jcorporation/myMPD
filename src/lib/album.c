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

#include "dist/libmympdclient/include/mpd/client.h"
#include "dist/libmympdclient/src/isong.h"
#include "src/lib/convert.h"
#include "src/lib/mem.h"
#include "src/mympd_client/tags.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

/**
 * myMPD saves album information in the album cache as a mpd_song struct.
 * Used fields:
 *   tags: tags from all songs of the album
 *   last_modified: last_modified from newest song
 *   added: added from oldest song
 *   duration: the album total time in seconds
 *   duration_ms: the album total time in milliseconds
 *   pos: number of discs
 *   prio: number of songs
 */

/**
 * Creates and initializes a new struct for an album.
 * @return struct mpd_song* or NULL on error
 */
struct mpd_song *album_new(void) {
    return album_new_uri("albumid");
}

/**
 * Creates and initializes a new struct for an album with defined first song uri
 * @return struct mpd_song* or NULL on error
 */
struct mpd_song *album_new_uri(const char *uri) {
    return mpd_song_new(uri);
}

/**
 * Frees an album struct
 * @param album Pointer to album struct
 */
void album_free(struct mpd_song *album) {
    assert(album != NULL);
    free(album->uri);
    for (unsigned i = 0; i < MPD_TAG_COUNT; ++i) {
        struct mpd_tag_value *tag = &album->tags[i];
        struct mpd_tag_value *next;
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
 * @param album mpd_song struct representing the album
 * @return const char* first song uri
 */
const char *album_get_uri(const struct mpd_song *album) {
    return album->uri;
}

/**
 * Gets the album duration
 * @param album mpd_song struct representing the album
 * @return unsigned album duration
 */
unsigned album_get_duration(const struct mpd_song *album) {
    return album->duration;
}

/**
 * Gets the last-modified timestamp for the album
 * @param album mpd_song struct representing the album
 * @return time_t last-modified timestamp
 */
time_t album_get_last_modified(const struct mpd_song *album) {
    return album->last_modified;
}

/**
 * Gets the added timestamp for the album
 * @param album mpd_song struct representing the album
 * @return time_t added timestamp
 */
time_t album_get_added(const struct mpd_song *album) {
    return album->added;
}

/**
 * Gets the number of songs
 * @param album mpd_song struct representing the album
 * @return number of songs
 */
unsigned album_get_song_count(const struct mpd_song *album) {
    return album->prio;
}

/**
 * Gets the number of discs
 * @param album mpd_song struct representing the album
 * @return number of discs
 */
unsigned album_get_discs(const struct mpd_song *album) {
    return album->pos;
}

/**
 * Gets the total play time
 * @param album mpd_song struct representing the album
 * @return total play time
 */
unsigned album_get_total_time(const struct mpd_song *album) {
    return album->duration;
}

/**
 * Gets the album tag value at position idx
 * @param album mpd_song struct representing the album
 * @param type mpd tag type
 * @param idx index of tag value to get
 * @return const char* tag value or NULL
 */
const char *album_get_tag(const struct mpd_song *album, enum mpd_tag_type type, unsigned idx) {
    const struct mpd_tag_value *tag = &album->tags[type];

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
 * @param album mpd_song struct representing the album
 * @param disc mpd song disc tag value
 */
void album_set_discs(struct mpd_song *album, const char *disc) {
    if (disc == NULL) {
        return;
    }
    unsigned d;
    enum str2int_errno rc = str2uint(&d, disc);
    if (rc == STR2INT_SUCCESS && 
        d > album->pos)
    {
        album->pos = d;
    }
}

/**
 * Sets a fixed disc count
 * @param album mpd_song struct representing the album
 * @param count disc count
 */
void album_set_disc_count(struct mpd_song *album, unsigned count) {
    album->pos = count;
}

/**
 * Sets the albums last modified timestamp if it is newer as current album last-modified timestamp
 * @param album mpd_song struct representing the album
 * @param last_modified last-modified timestamp to set
 */
void album_set_last_modified(struct mpd_song *album, time_t last_modified) {
    if (album->last_modified < last_modified) {
        album->last_modified = last_modified;
    }
}

/**
 * Sets the albums added timestamp if it is older as current album added timestamp
 * @param album mpd_song struct representing the album
 * @param added added timestamp to set
 */
void album_set_added(struct mpd_song *album, time_t added) {
    if (album->added > added) {
        album->added = added;
    }
}

/**
 * Sets the albums duration
 * @param album mpd_song struct representing the album
 * @param duration total time to set
 */
void album_set_total_time(struct mpd_song *album, unsigned duration) {
    album->duration = duration;
    album->duration_ms = duration * 1000;
}

/**
 * Increments the albums duration
 * @param album mpd_song struct representing the album
 * @param duration seconds to increment
 */
void album_inc_total_time(struct mpd_song *album, unsigned duration) {
    album->duration += duration;
    album->duration_ms += duration * 1000;
}

/**
 * Set the song count
 * @param album mpd_song struct representing the album
 * @param count song count
 */
void album_set_song_count(struct mpd_song *album, unsigned count) {
    album->prio = count;
}

/**
 * Increments the song count
 * @param album pointer to a mpd_song struct
 */
void album_inc_song_count(struct mpd_song *album) {
    album->prio++;
}

/**
 * Appends tag values to the album
 * @param album pointer to a mpd_song struct representing the album
 * @param song song to add tag values from
 * @param tags tags to append
 * @return true on success else false
 */
bool album_append_tags(struct mpd_song *album,
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
 * @param song pointer to a mpd_song struct
 * @param type mpd tag type
 * @param value tag value to add
 * @return true if tag is added or already there,
 *         false if the tag could not be added
 */
bool album_append_tag(struct mpd_song *song,
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
 * Copies all values from a tag to another tag
 * @param song pointer to a mpd_song struct
 * @param src source tag
 * @param dst destination tag
 * @return true on success, else false
 */
bool album_copy_tags(struct mpd_song *song, enum mpd_tag_type src, enum mpd_tag_type dst) {
    const char *value;
    unsigned value_nr = 0;
    while ((value = mpd_song_get_tag(song, src, value_nr)) != NULL) {
        if (album_append_tag(song, dst, value) == false) {
            return false;
        }
        value_nr++;
    }
    return true;
}

/**
 * Replaces the uri
 * @param album pointer to a mpd_song struct
 * @param uri new uri to set
 */
void album_set_uri(struct mpd_song *album, const char *uri) {
    size_t len = strlen(uri);
    album->uri = realloc_assert(album->uri, len + 1);
    memcpy(album->uri, uri, len);
    album->uri[len] = '\0';
}
