---
layout: page
permalink: /scripting/lua-table-mympd_state
title: Lua table mympd_state
---

This is the description of the lua table mympd_state. This table must be initialized with `mympd.init()`.

| KEY | TYPE | DESCRIPTION |
| --- | ---- | ----------- |
| play_state | Integer | Player state: 0 = unknown, 1 = stop, 2 = play, 3 = pause |
| volume | Integer | 0 - 100 percent |
| song_pos | Integer | Current song position in queue |
| elapsed_time | Integer | Elapsed time of current song |
| total_time | Integer | Total time of current song |
| song_id | Integer | Song id of current song |
| next_song_id | Integer | Next song id in queue |
| next_song_pos | Integer | Next song position in queue |
| queue_length | Integer | Length of the queue |
| queue_version | Integer | Version of the queue |
| repeat | Boolean | MPD repeat option |
| random | Boolean | MPD random option |
| single_state | Integer | MPD single state: 0 = off, 1 = on, 2 = oneshot, 3 = unknown |
| consume_state | Integer | MPD consume state: 0 = off, 1 = on, 2 = oneshot, 3 = unknown |
| crossfade | Integer | MPD crossfade option |
| mixrampdelay | Float | Mixramp delay |
| mixrampdb | Float | Mixramp DB |
| replaygain | Integer | 0 = off, 1 = track, 2 = album, 3 = auto, 4 = unknown |
| music_directory | String | path to the mpd music directory |
| playlist_directory | String | path to the mpd playlist directory |
| workdir | String | path to the myMPD working directory (default: /var/lib/mympd) |
| cachedir | String | path to the myMPD cache directory (default: /var/cache/mympd) |
| auto_play | Boolean | true = enabled, false = disabled |
| jukebox_mode | Integer | Jukebox mode: 0 = off, 1 = song, 2 = album |
| jukebox_playlist | String | Jukebox playlist: Database or MPD playlist name |
| jukebox_queue_length | Integer | Length of the queue length to maintain |
| jukebox_last_played | Integer | Don't add songs that are played in the last x hours |
| jukebox_unique_tag | String | Build the jukebox queue with this tag as unique constraint: Song, Album, Artist |
| jukebox_ignore_hated | Boolean | Ignore hated songs for jukebox |
| listenbrainz_token | ListenBrainz Token |
{: .table .table-sm }
