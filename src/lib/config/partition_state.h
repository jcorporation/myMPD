/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Central myMPD partition state for the mympd_api thread
 */

#ifndef MYMPD_PARTITION_STATE_H
#define MYMPD_PARTITION_STATE_H

#include "dist/sds/sds.h"
#include "src/lib/config/config_def.h"
#include "src/lib/config/jukebox_state.h"
#include "src/lib/config/mympd_mpd_state.h"
#include "src/lib/event.h"
#include "src/lib/jukebox.h"
#include "src/lib/list/list.h"

#include <time.h>

/**
 * Holds partition specific states
 */
struct t_partition_state {
    struct t_config *config;               //!< pointer to static config
    struct t_mpd_state *mpd_state;         //!< pointer to shared mpd state
    //mpd connection
    struct mpd_connection *conn;           //!< mpd connection object from libmpdclient
    enum mympd_mpd_conn_states conn_state; //!< mpd connection state
    //track player states
    enum mpd_state play_state;             //!< mpd player state
    int song_id;                           //!< current song id from queue
    int next_song_id;                      //!< next song id from queue
    int last_song_id;                      //!< previous song id from queue
    int song_pos;                          //!< current song pos in queue
    time_t song_duration;                  //!< current song length
    struct mpd_song *song;                 //!< current song
    struct mpd_song *last_song;            //!< previous song
    unsigned queue_version;                //!< queue version number (increments on queue change)
    unsigned queue_length;                 //!< length of the queue
    int last_skipped_id;                   //!< last skipped event was fired for this song id
    time_t song_end_time;                  //!< timestamp at which current song should end (starttime + duration)
    time_t last_song_end_time;             //!< timestamp at which previous song should end (starttime + duration)
    time_t song_start_time;                //!< timestamp at which current song has started
    time_t last_song_start_time;           //!< timestamp at which previous song has started
    time_t crossfade;                      //!< used for determine when to add next song from jukebox queue
    bool auto_play;                        //!< start play if queue changes
    bool player_error;                     //!< signals mpd player error condition
    struct t_jukebox_state jukebox;        //!< jukebox
    //partition
    sds name;                              //!< partition name
    sds highlight_color;                   //!< highlight color
    sds highlight_color_contrast;          //!< highlight contrast color
    sds state_dir;                         //!< partition state folder
    struct t_partition_state *next;        //!< pointer to next partition;
    bool is_default;                       //!< flag for the mpd default partition
    enum mpd_idle idle_mask;               //!< mpd idle mask
    bool set_conn_options;                 //!< true if mpd connection options should be changed
    //local playback
    unsigned mpd_stream_port;              //!< mpd http stream port setting
    sds stream_uri;                        //!< custom url for local playback
    //lists
    struct t_list last_played;             //!< last_played list
    struct t_list preset_list;             //!< Playback presets
    //timers
    int timer_fd_jukebox;                  //!< Timerfd for jukebox runs
    int timer_fd_scrobble;                 //!< Timerfd for scrobble event
    int timer_fd_mpd_connect;              //!< Timerfd for mpd reconnection
    //events
    enum pfd_type waiting_events;          //!< Bitmask for events
    bool *repopulate_pfds;                 //!< Pointer to repopulate state in mympd_state struct
};

/**
 * Public functions
 */
void partition_state_default(struct t_partition_state *partition_state, const char *name,
        struct t_mpd_state *mpd_state, struct t_config *config);
void partition_state_free(struct t_partition_state *partition_state);

#endif
