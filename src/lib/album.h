/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Album implementation
 */

#ifndef MYMPD_ALBUM_H
#define MYMPD_ALBUM_H

#include "src/lib/fields.h"
#include "src/lib/mpdclient.h"

#include <stdbool.h>

/**
 * An opaque representation for an album in myMPD's album cache.
 * Use the functions provided by this header to access the object's
 * attributes.
 */
struct t_album;

/**
 * Modes for the album cache
 */
enum album_modes {
    ALBUM_MODE_SIMPLE = 0,
    ALBUM_MODE_ADV
};

/**
 * Holds config for the album cache
 */
struct t_albums_config {
    enum album_modes mode;        //!< enable advanced albums
    enum mpd_tag_type group_tag;  //!< additional group tag for albums
};

struct t_album *album_new(void);
struct t_album *album_new_uri(const char *uri);
struct t_album *album_new_from_song(const struct mpd_song *song, const struct t_mympd_mpd_tags *album_tags);
void album_free(struct t_album *album);

const char *album_get_uri(const struct t_album *album);
time_t album_get_last_modified(const struct t_album *album);
time_t album_get_added(const struct t_album *album);
unsigned album_get_disc_count(const struct t_album *album);
unsigned album_get_total_time(const struct t_album *album);
unsigned album_get_song_count(const struct t_album *album);
const char *album_get_tag(const struct t_album *album, enum mpd_tag_type type, unsigned idx);

void album_set_discs(struct t_album *album, const char *disc);
void album_set_disc_count(struct t_album *album, unsigned count);
void album_set_last_modified(struct t_album *album, time_t last_modified);
void album_set_added(struct t_album *album, time_t added);
void album_set_total_time(struct t_album *album, unsigned duration);
void album_inc_total_time(struct t_album *album, unsigned duration);
void album_set_song_count(struct t_album *album, unsigned count);
void album_inc_song_count(struct t_album *album);
bool album_append_tag(struct t_album *song, enum mpd_tag_type type, const char *value);
bool album_append_tags(struct t_album *album, const struct mpd_song *song, const struct t_mympd_mpd_tags *tags);
bool album_copy_tags(struct t_album *song, enum mpd_tag_type src, enum mpd_tag_type dst);
void album_set_uri(struct t_album *album, const char *uri);
sds album_get_tag_value_string(const struct t_album *album, enum mpd_tag_type tag, sds tag_values);
sds album_get_tag_values(const struct t_album *album, enum mpd_tag_type tag, sds tag_values);
sds album_get_tag_value_padded(const struct t_album *album, enum mpd_tag_type tag, char pad, size_t len, sds tag_values);
sds print_album_tags(sds buffer, const struct t_albums_config *album_config, const struct t_mympd_mpd_tags *tagcols,
        const struct t_album *album);

#endif
