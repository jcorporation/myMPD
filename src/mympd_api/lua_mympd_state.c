/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD state for Lua scripts
 */

#include "compile_time.h"
#include "src/mympd_api/lua_mympd_state.h"

#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"
#include "src/mpd_client/errorhandler.h"
#include "src/mpd_client/shortcuts.h"
#include "src/mympd_api/status.h"

/**
 * Private definitions
 */
static void lua_mympd_state_free_user_data(struct t_list_node *current);

/**
 * Public functions
 */

/**
 * Copies mpd and myMPD states to the lua_mympd_state struct
 * @param lua_partition_state pointer to struct t_list
 * @param mympd_state pointer to mympd_state
 * @param partition_state pointer to partition state
 * @return true on success, else false
 */
bool mympd_api_status_lua_mympd_state_set(struct t_list *lua_partition_state, struct t_mympd_state *mympd_state,
        struct t_partition_state *partition_state)
{
    // MPD state
    if (mpd_command_list_begin(partition_state->conn, true)) {
        if (mpd_send_status(partition_state->conn) == false) {
            mympd_set_mpd_failure(partition_state, "Error adding command to command list mpd_send_status");
        }
        if (mpd_send_replay_gain_status(partition_state->conn) == false) {
            mympd_set_mpd_failure(partition_state, "Error adding command to command list mpd_send_replay_gain_status");
        }
        mpd_client_command_list_end_check(partition_state);
    }
    struct mpd_status *status = mpd_recv_status(partition_state->conn);
    enum mpd_replay_gain_mode replay_gain_mode = MPD_REPLAY_UNKNOWN;
    if (mpd_response_next(partition_state->conn)) {
        replay_gain_mode = mpd_recv_replay_gain_status(partition_state->conn);
    }
    if (status != NULL) {
        lua_mympd_state_set_i(lua_partition_state, "play_state", mpd_status_get_state(status));
        lua_mympd_state_set_i(lua_partition_state, "volume", mpd_status_get_volume(status));
        lua_mympd_state_set_i(lua_partition_state, "song_pos", mpd_status_get_song_pos(status));
        lua_mympd_state_set_u(lua_partition_state, "elapsed_time", mympd_api_get_elapsed_seconds(status));
        lua_mympd_state_set_u(lua_partition_state, "total_time", mpd_status_get_total_time(status));
        lua_mympd_state_set_i(lua_partition_state, "song_id", mpd_status_get_song_id(status));
        lua_mympd_state_set_i(lua_partition_state, "next_song_id", mpd_status_get_next_song_id(status));
        lua_mympd_state_set_i(lua_partition_state, "next_song_pos", mpd_status_get_next_song_pos(status));
        lua_mympd_state_set_u(lua_partition_state, "queue_length", mpd_status_get_queue_length(status));
        lua_mympd_state_set_u(lua_partition_state, "queue_version", mpd_status_get_queue_version(status));
        lua_mympd_state_set_b(lua_partition_state, "repeat", mpd_status_get_repeat(status));
        lua_mympd_state_set_b(lua_partition_state, "random", mpd_status_get_random(status));
        lua_mympd_state_set_i(lua_partition_state, "single_state", mpd_status_get_single_state(status));
        lua_mympd_state_set_i(lua_partition_state, "consume_state", mpd_status_get_consume_state(status));
        lua_mympd_state_set_u(lua_partition_state, "crossfade", mpd_status_get_crossfade(status));
        lua_mympd_state_set_f(lua_partition_state, "mixrampdelay", mpd_status_get_mixrampdelay(status));
        lua_mympd_state_set_f(lua_partition_state, "mixrampdb", mpd_status_get_mixrampdb(status));
        lua_mympd_state_set_i(lua_partition_state, "replaygain", replay_gain_mode);
        if (partition_state->mpd_state->feat.partitions == true) {
            lua_mympd_state_set_p(lua_partition_state, "partition", mpd_status_get_partition(status));
        }
        mpd_status_free(status);
    }
    mpd_response_finish(partition_state->conn);
    bool rc = mympd_check_error_and_recover(partition_state, NULL, "mpd_run_status");
    if (rc == false) {
        MYMPD_LOG_ERROR(partition_state->name, "Error getting mpd state for script execution");
    }
    // myMPD state
    lua_mympd_state_set_p(lua_partition_state, "music_directory", partition_state->mpd_state->music_directory_value);
    lua_mympd_state_set_p(lua_partition_state, "playlist_directory", partition_state->mpd_state->playlist_directory_value);
    lua_mympd_state_set_b(lua_partition_state, "auto_play", partition_state->auto_play);
    lua_mympd_state_set_i(lua_partition_state, "jukebox_mode", partition_state->jukebox.mode);
    lua_mympd_state_set_p(lua_partition_state, "jukebox_playlist", partition_state->jukebox.playlist);
    lua_mympd_state_set_i(lua_partition_state, "jukebox_queue_length", partition_state->jukebox.queue_length);
    lua_mympd_state_set_i(lua_partition_state, "jukebox_last_played", partition_state->jukebox.last_played);
    lua_mympd_state_set_b(lua_partition_state, "jukebox_ignore_hated", partition_state->jukebox.ignore_hated);
    lua_mympd_state_set_p(lua_partition_state, "jukebox_uniq_tag", mpd_tag_name(partition_state->jukebox.uniq_tag.tags[0]));
    lua_mympd_state_set_i(lua_partition_state, "jukebox_min_song_duration", partition_state->jukebox.min_song_duration);
    lua_mympd_state_set_i(lua_partition_state, "jukebox_max_song_duration", partition_state->jukebox.max_song_duration);
    //myMPD uri
    sds uri = sdsnew("mympd://");
    uri = resolv_mympd_uri(uri, mympd_state->mpd_state->mpd_host, mympd_state->config, true);
    lua_mympd_state_set_p(lua_partition_state, "mympd_uri", uri);
    FREE_SDS(uri);
    return rc;
}

/**
 * Pushes a string to the lua_mympd_state list
 * @param lua_mympd_state pointer to a t_list struct
 * @param k variable name
 * @param v variable value
 */
void lua_mympd_state_set_p(struct t_list *lua_mympd_state, const char *k, const char *v) {
    struct t_lua_mympd_state_value *value = malloc_assert(sizeof(struct t_lua_mympd_state_value));
    value->p = sdsnew(v);
    list_push(lua_mympd_state, k, LUA_TYPE_STRING, NULL, value);
}

/**
 * Pushes an int64_t value to the lua_mympd_state list
 * @param lua_mympd_state pointer to a t_list struct
 * @param k variable name
 * @param v variable value
 */
void lua_mympd_state_set_i(struct t_list *lua_mympd_state, const char *k, int64_t v) {
    struct t_lua_mympd_state_value *value = malloc_assert(sizeof(struct t_lua_mympd_state_value));
    value->i = v;
    list_push(lua_mympd_state, k, LUA_TYPE_INTEGER, NULL, value);
}

/**
 * Pushes a double value to the lua_mympd_state list
 * @param lua_mympd_state pointer to a t_list struct
 * @param k variable name
 * @param v variable value
 */
void lua_mympd_state_set_f(struct t_list *lua_mympd_state, const char *k, double v) {
    struct t_lua_mympd_state_value *value = malloc_assert(sizeof(struct t_lua_mympd_state_value));
    value->f = v;
    list_push(lua_mympd_state, k, LUA_TYPE_INTEGER, NULL, value);
}

/**
 * Pushes a unsigned int value to the lua_mympd_state list
 * @param lua_mympd_state pointer to a t_list struct
 * @param k variable name
 * @param v variable value
 */
void lua_mympd_state_set_u(struct t_list *lua_mympd_state, const char *k, unsigned v) {
    struct t_lua_mympd_state_value *value = malloc_assert(sizeof(struct t_lua_mympd_state_value));
    value->i = (int64_t)v;
    list_push(lua_mympd_state, k, LUA_TYPE_INTEGER, NULL, value);
}

/**
 * Pushes a bool value to the lua_mympd_state list
 * @param lua_mympd_state pointer to a t_list struct
 * @param k variable name
 * @param v variable value
 */
void lua_mympd_state_set_b(struct t_list *lua_mympd_state, const char *k, bool v) {
    struct t_lua_mympd_state_value *value = malloc_assert(sizeof(struct t_lua_mympd_state_value));
    value->b = v;
    list_push(lua_mympd_state, k, LUA_TYPE_BOOLEAN, NULL, value);
}

/**
 * Frees the lua_mympd_state list
 * @param lua_mympd_state pointer to the list
 * @return NULL
 */
void *lua_mympd_state_free(struct t_list *lua_mympd_state) {
    return list_free_user_data(lua_mympd_state, lua_mympd_state_free_user_data);
}

/**
 * Private functions
 */

/**
 * Callback for lua_mympd_state_free to free string values
 * @param current pointer to the list node
 */
static void lua_mympd_state_free_user_data(struct t_list_node *current) {
    if (current->value_i == LUA_TYPE_STRING) {
        struct t_lua_mympd_state_value *user_data = (struct t_lua_mympd_state_value *)current->user_data;
        FREE_SDS(user_data->p);
    }
    FREE_PTR(current->user_data);
}
