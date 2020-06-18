/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __LUA_MYMPD_STATE_H__
#define __LUA_MYMPD_STATE_H__

typedef struct t_lua_mympd_state {
    enum mpd_state play_state;
    int volume;
    int song_pos;
    unsigned elapsed_time;
    unsigned total_time;
    int song_id;
    int next_song_id;
    int next_song_pos;
    unsigned queue_length;
    unsigned queue_version;
    bool repeat;
    bool random;
    enum mpd_single_state single_state;
    bool consume;
    unsigned crossfade;
    float mixrampdb;
    float mixrampdelay;
    sds music_directory;
    sds varlibdir;
} t_lua_mympd_state;

void free_t_lua_mympd_state(t_lua_mympd_state *lua_mympd_state);

#endif
