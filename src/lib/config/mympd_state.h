/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Central myMPD state for the mympd_api thread
 */

#ifndef MYMPD_STATE_H
#define MYMPD_STATE_H

#include "dist/sds/sds.h"
#include "src/lib/cache/cache_rax.h"
#include "src/lib/config/config_def.h"
#include "src/lib/config/mympd_mpd_state.h"
#include "src/lib/config/partition_state.h"
#include "src/lib/config/stickerdb_state.h"
#include "src/lib/config/timer_state.h"
#include "src/lib/event.h"
#include "src/lib/fields.h"
#include "src/lib/jukebox.h"
#include "src/lib/list/list.h"
#include "src/lib/lyrics.h"
#include "src/lib/webradio.h"

/**
 * Holds central myMPD state and configuration values.
 */
struct t_mympd_state {
    struct t_config *config;                        //!< pointer to static config
    struct t_mpd_state *mpd_state;                  //!< mpd state shared across partitions
    struct t_partition_state *partition_state;      //!< list of partition states
    struct t_stickerdb_state *stickerdb;            //!< states for stickerdb connection
    struct mympd_pfds pfds;                         //!< fds to poll in the event loop
    struct t_timer_list timer_list;                 //!< list of timers
    struct t_list home_list;                        //!< list of home icons
    struct t_list trigger_list;                     //!< list of triggers
    sds tag_list_search;                            //!< comma separated string of tags for search
    sds tag_list_browse;                            //!< comma separated string of tags for browse
    bool smartpls;                                  //!< enable smart playlists
    sds smartpls_sort;                              //!< sort smart playlists by this tag
    sds smartpls_prefix;                            //!< name prefix for smart playlists
    int smartpls_interval;                          //!< interval to refresh smart playlists in seconds
    struct t_mympd_mpd_tags smartpls_generate_tag_types;  //!< generate smart playlists for each value for this tag
    sds smartpls_generate_tag_list;                 //!< generate smart playlists for each value for this tag (string representation)
    sds view_queue_current;                         //!< view settings for the queue view
    sds view_search;                                //!< view settings for the search view
    sds view_browse_database_album_detail_info;     //!< view settings for the album detail view
    sds view_browse_database_album_detail;          //!< view settings for the album detail title list
    sds view_browse_database_album_list;            //!< view settings for the album list view
    sds view_browse_database_tag_list;              //!< view settings for the album list view
    sds view_browse_playlist_list;                  //!< view settings for the listing of playlists
    sds view_browse_playlist_detail;                //!< view settings for the listing of playlist contents
    sds view_browse_filesystem;                     //!< view settings for filesystem listing
    sds view_playback;                              //!< view settings for playback view
    sds view_queue_last_played;                     //!< view settings for last played view
    sds view_queue_jukebox_song;                    //!< view settings for the jukebox queue view for songs
    sds view_queue_jukebox_album;                   //!< view settings for the jukebox queue view for albums
    sds view_browse_radio_webradiodb;               //!< view settings for the webradiodb view
    sds view_browse_radio_favorites;                //!< view settings for the radio favorites view
    sds music_directory;                            //!< mpd music directory setting (real value is in mpd_state)
    sds playlist_directory;                         //!< mpd playlist directory (real value is in mpd_state)
    sds navbar_icons;                               //!< json string of navigation bar icons
    sds image_names_sm;                             //!< comma separated string of small coverimage names
    sds image_names_md;                             //!< comma separated string of medium coverimage names
    sds image_names_lg;                             //!< comma separated string of large coverimage names
    unsigned volume_min;                            //!< minimum mpd volume
    unsigned volume_max;                            //!< maximum mpd volume
    unsigned volume_step;                           //!< volume step for +/- buttons
    struct t_lyrics lyrics;                         //!< lyrics settings
    sds webui_settings;                             //!< settings only relevant for webui, saved as string containing json
    bool tag_disc_empty_is_first;                   //!< handle empty disc tag as disc one for albums
    sds booklet_name;                               //!< name of the booklet files
    sds info_txt_name;                              //!< name of album info files
    struct t_cache album_cache;                     //!< the album cache created by the mympd_worker thread
    unsigned last_played_count;                     //!< number of songs to keep in the last played list (disk + memory)
    struct t_webradios *webradiodb;                 //!< WebradioDB
    struct t_webradios *webradio_favorites;         //!< webradio favorites
};

/**
 * Public functions
 */
void mympd_state_save(struct t_mympd_state *mympd_state, bool free_data);
void mympd_state_default(struct t_mympd_state *mympd_state, struct t_config *config);
void mympd_state_free(struct t_mympd_state *mympd_state);

#endif
