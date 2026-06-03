/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Central myMPD partition state for the mympd_api thread
 */

#include "compile_time.h"
#include "src/lib/config/partition_state.h"

#include "src/lib/mem.h"
#include "src/lib/sds/sds_extras.h"
#include "src/lib/sds/sds_file.h"
#include "src/lib/timer.h"
#include "src/mympd_client/presets.h"

#include <string.h>

/**
 * Sets per partition state defaults
 * @param partition_state pointer to t_partition_state struct
 * @param name partition name
 * @param mpd_state pointer to shared mpd state
 * @param config pointer to static config
 */
void partition_state_default(struct t_partition_state *partition_state, const char *name,
        struct t_mpd_state *mpd_state, struct t_config *config)
{
    partition_state->name = sdsnew(name);
    partition_state->highlight_color = sdsnew(PARTITION_HIGHLIGHT_COLOR);
    partition_state->highlight_color_contrast = sdsnew(PARTITION_HIGHLIGHT_COLOR_CONTRAST);
    sds partition_dir = sdsnew(name);
    sds_sanitize_filename(partition_dir);
    partition_state->state_dir = sdscatfmt(sdsempty(), "%s/%S", DIR_WORK_STATE, partition_dir);
    FREE_SDS(partition_dir);
    partition_state->conn = NULL;
    partition_state->conn_state = MPD_DISCONNECTED;
    partition_state->play_state = MPD_STATE_UNKNOWN;
    partition_state->song_id = -1;
    partition_state->song = NULL;
    partition_state->song_pos = -1;
    partition_state->song_duration = 0;
    partition_state->next_song_id = -1;
    partition_state->last_song_id = -1;
    partition_state->last_song = NULL;
    partition_state->queue_version = 0;
    partition_state->queue_length = 0;
    partition_state->song_start_time = 0;
    partition_state->song_end_time = 0;
    partition_state->last_song_start_time = 0;
    partition_state->last_song_end_time = 0;
    partition_state->last_skipped_id = 0;
    partition_state->crossfade = 0;
    partition_state->auto_play = MYMPD_AUTO_PLAY;
    partition_state->next = NULL;
    partition_state->player_error = false;
    //jukebox
    jukebox_state_default(&partition_state->jukebox);
    //add pointer to other states
    partition_state->config = config;
    partition_state->mpd_state = mpd_state;
    //mpd idle mask
    if (strcmp(name, MPD_PARTITION_DEFAULT) == 0) {
        partition_state->is_default = true;
        //handle all
        partition_state->idle_mask = MPD_IDLE_QUEUE | MPD_IDLE_PLAYER | MPD_IDLE_MIXER | MPD_IDLE_OUTPUT | MPD_IDLE_OPTIONS |
            MPD_IDLE_UPDATE | MPD_IDLE_PARTITION | MPD_IDLE_DATABASE | MPD_IDLE_STORED_PLAYLIST | MPD_IDLE_SUBSCRIPTION | MPD_IDLE_MESSAGE |
            MPD_IDLE_NEIGHBOR | MPD_IDLE_MOUNT;
    }
    else {
        partition_state->is_default = false;
        //handle only partition specific mpd idle events
        partition_state->idle_mask = MPD_IDLE_QUEUE | MPD_IDLE_PLAYER | MPD_IDLE_MIXER | MPD_IDLE_OUTPUT | MPD_IDLE_OPTIONS;
    }
    partition_state->set_conn_options = false;
    //local playback
    partition_state->mpd_stream_port = PARTITION_MPD_STREAM_PORT;
    partition_state->stream_uri = sdsnew(PARTITION_MPD_STREAM_URI);
    //lists
    list_init(&partition_state->last_played);
    list_init(&partition_state->preset_list);
    preset_list_load(partition_state);
    //timers
    partition_state->timer_fd_jukebox = mympd_timer_create(CLOCK_MONOTONIC, 0, 0);
    partition_state->timer_fd_scrobble = mympd_timer_create(CLOCK_MONOTONIC, 0, 0);
    partition_state->timer_fd_mpd_connect = mympd_timer_create(CLOCK_MONOTONIC, 0, 0);
    //events
    partition_state->waiting_events = 0;
}

/**
 * Frees the t_partition_state struct
 * @param partition_state pointer to t_partition_state struct
 */
void partition_state_free(struct t_partition_state *partition_state) {
    FREE_SDS(partition_state->name);
    FREE_SDS(partition_state->highlight_color);
    FREE_SDS(partition_state->highlight_color_contrast);
    FREE_SDS(partition_state->state_dir);
    if (partition_state->song != NULL) {
        mpd_song_free(partition_state->song);
    }
    if (partition_state->last_song != NULL) {
        mpd_song_free(partition_state->last_song);
    }
    //jukebox
    jukebox_state_free(&partition_state->jukebox);
    //lists
    list_clear(&partition_state->last_played);
    list_clear(&partition_state->preset_list);
    //local playback
    FREE_SDS(partition_state->stream_uri);
    //timers
    mympd_timer_close(partition_state->timer_fd_jukebox);
    mympd_timer_close (partition_state->timer_fd_scrobble);
    mympd_timer_close(partition_state->timer_fd_mpd_connect);
    //struct itself
    FREE_PTR(partition_state);
}
