/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Central myMPD mpd state for the mympd_api thread
 */

#ifndef MYMPD_MPD_STATE_H
#define MYMPD_MPD_STATE_H

#include "dist/sds/sds.h"
#include "src/lib/fields.h"

#include <stdbool.h>

/**
 * MPD connection states
 */
enum mympd_mpd_conn_states {
    MPD_CONNECTED,     //!< mpd is connected
    MPD_DISCONNECTED,  //!< mpd is disconnected
    MPD_FAILURE        //!< mpd is in unrecoverable failure state
};

/**
 * MPD feature flags
 */
struct t_mpd_features {
    bool fingerprint;              //!< mpd supports the fingerprint command
    bool mount;                    //!< mpd supports mounts
    bool neighbor;                 //!< mpd supports neighbors command
    bool pcre;                     //!< mpd supports pcre for filter expressions
    bool playlists;                //!< mpd supports playlists
    bool stickers;                 //!< mpd supports stickers
    bool tags;                     //!< mpd tags are enabled
    bool library;                  //!< myMPD has access to the mpd music directory
    // MPD 0.25 features
    bool mpd_0_25_0;               //!< MPD protocol version is ge 0.25.0
    // MPD 0.24 features
    bool mpd_0_24_0;               //!< MPD protocol version is ge 0.24.0
    bool advsticker;               //!< mpd supports new sticker commands from MPD 0.24
    bool db_added;                 //!< mpd supports added attribute for songs (MPD 0.24)
    bool advqueue;                 //!< mpd supports the prio filter / sort for queue and the save modes (MPD 0.24)
    bool consume_oneshot;          //!< mpd supports consume oneshot mode (MPD 0.24)
    bool listplaylist_range;       //!< mpd supports the listplaylist with range parameter (MPD 0.24)
    bool playlist_dir_auto;        //!< mpd supports autodetection of playlist directory (MPD 0.24)
    bool starts_with;              //!< mpd supports starts_with filter expression (MPD 0.24)
};

/**
 * Holds MPD specific states shared across all partitions
 */
struct t_mpd_state {
    struct t_config *config;            //!< pointer to static config
    //connection configuration
    sds mpd_host;                       //!< mpd host configuration
    unsigned mpd_port;                  //!< mpd port configuration
    sds mpd_pass;                       //!< mpd password
    unsigned mpd_binarylimit;           //!< mpd binary limit to set
    unsigned mpd_timeout;               //!< mpd connection timeout
    bool mpd_keepalive;                 //!< mpd tcp keepalive flag
    bool mpd_stringnormalization;       //!< mpd stringnormalization
    sds music_directory_value;          //!< real music directory set by feature detection
    sds playlist_directory_value;       //!< real playlist directory set by feature detection
    //tags
    sds tag_list;                       //!< comma separated string of mpd tags to enable
    struct t_mympd_mpd_tags tags_mympd;       //!< tags enabled by myMPD and mpd
    struct t_mympd_mpd_tags tags_mpd;         //!< all available mpd tags
    struct t_mympd_mpd_tags tags_search;      //!< tags enabled for search
    struct t_mympd_mpd_tags tags_browse;      //!< tags enabled for browse
    struct t_mympd_mpd_tags tags_album;       //!< tags enabled for albums
    enum mpd_tag_type tag_albumartist;  //!< tag to use for AlbumArtist
    //Feature flags
    const unsigned *protocol;           //!< mpd protocol version
    struct t_mpd_features feat;         //!< feature flags
    struct t_list sticker_types;        //!< mpd sticker types
};

/**
 * Public functions
 */
void mympd_mpd_state_features_default(struct t_mpd_features *feat);
void mympd_mpd_state_features_copy(struct t_mpd_features *src, struct t_mpd_features *dst);

void mympd_mpd_state_default(struct t_mpd_state *mpd_state, struct t_config *config);
void mympd_mpd_state_copy(struct t_mpd_state *src, struct t_mpd_state *dst);
void mympd_mpd_state_free(struct t_mpd_state *mpd_state);

#endif
