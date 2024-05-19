---
layout: page
permalink: /scripting/functions/mympd_state
title: Accessing myMPD and MPD status information
---

Populates the lua table `mympd_state` with configuration values and current status of myMPD and MPD.

Additionally all user defined variables are populates in this table. They are prefixed with `var_`.

```lua
mympd.init()
```

**Parameters:**

No parameters needed.

**Returns:**

| FIELD | TYPE | DESCRIPTION |
| ----- | ---- | ----------- |
| rc | integer | response code: 0 = success, 1 = error |
| result | lua table | jsonrpc result or error as lua table |
{: .table .table-sm }

The lua table mympd_state is populated with following fields. Additionally all user defined variables are populates in this table. They are prefixed with `var_`.

| KEY | TYPE | DESCRIPTION |
| --- | ---- | ----------- |
| auto_play | Boolean | true = enabled, false = disabled |
| cachedir | String | path to the myMPD cache directory (default: /var/cache/mympd) |
| consume_state | Integer | MPD consume state: 0 = off, 1 = on, 2 = oneshot, 3 = unknown |
| crossfade | Integer | MPD crossfade option |
| elapsed_time | Integer | Elapsed time of current song |
| jukebox_ignore_hated | Boolean | Ignore hated songs for jukebox |
| jukebox_last_played | Integer | Don't add songs that are played in the last x hours |
| jukebox_mode | Integer | Jukebox mode: 0 = off, 1 = song, 2 = album |
| jukebox_playlist | String | Jukebox playlist: Database or MPD playlist name |
| jukebox_queue_length | Integer | Length of the queue length to maintain |
| jukebox_uniq_tag | String | Build the jukebox queue with this tag as uniq constraint: Song, Album, Artist |
| listenbrainz_token | String | ListenBrainz Token |
| mixrampdelay | Float | Mixramp delay |
| mixrampdb | Float | Mixramp DB |
| music_directory | String | Path to the mpd music directory |
| mympd_uri | String | Canonical myMPD uri |
| next_song_id | Integer | Next song id in queue |
| next_song_pos | Integer | Next song position in queue |
| play_state | Integer | Player state: 0 = unknown, 1 = stop, 2 = play, 3 = pause |
| playlist_directory | String | path to the mpd playlist directory |
| queue_length | Integer | Length of the queue |
| queue_version | Integer | Version of the queue |
| repeat | Boolean | MPD repeat option |
| replaygain | Integer | 0 = off, 1 = track, 2 = album, 3 = auto, 4 = unknown |
| random | Boolean | MPD random option |
| single_state | Integer | MPD single state: 0 = off, 1 = on, 2 = oneshot, 3 = unknown |
| song_id | Integer | Song id of current song |
| song_pos | Integer | Current song position in queue |
| total_time | Integer | Total time of current song |
| volume | Integer | 0 - 100 percent |
| workdir | String | path to the myMPD working directory (default: /var/lib/mympd) |
{: .table .table-sm }
