---

title: Accessing myMPD and MPD status information
---

Populates the lua table `mympd_state` with configuration values and current status of myMPD and MPD.

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

The lua table mympd_state is populated with following fields.

### mympd_state

| KEY | TYPE | DESCRIPTION |
| --- | ---- | ----------- |
| `auto_play` | Boolean | true = enabled, false = disabled |
| `consume_state` | Integer | MPD consume state: 0 = off, 1 = on, 2 = oneshot, 3 = unknown |
| `crossfade` | Integer | MPD crossfade option |
| `current_song` | Table | Current MPD song |
| `elapsed_time` | Integer | Elapsed time of current song |
| `jukebox_ignore_hated` | Boolean | Ignore hated songs for jukebox |
| `jukebox_last_played` | Integer | Don't add songs that are played in the last x hours |
| `jukebox_max_song_duration` | Integer | Only songs with this minimum length will be considered. |
| `jukebox_min_song_duration` | Integer | If greater then zero: Only songs with this maximum length will be considered. |
| `jukebox_mode` | Integer | Jukebox mode: 0 = off, 1 = song, 2 = album, 3 = script |
| `jukebox_playlist` | String | Jukebox playlist: Database or MPD playlist name |
| `jukebox_queue_length` | Integer | Number of songs in the queue before the jukebox add's more songs. |
| `jukebox_uniq_tag` | String | Build the jukebox queue with this tag as uniq constraint: Song, Album, Artist |
| `listenbrainz_token` | String | ListenBrainz Token |
| `mixrampdelay` | Float | Mixramp delay |
| `mixrampdb` | Float | Mixramp DB |
| `music_directory` | String | Path to the mpd music directory |
| `mympd_uri` | String | Canonical myMPD uri |
| `mympd_uri_plain` | String | Canonical myMPD uri (http://) |
| `next_song_id` | Integer | Next song id in queue |
| `next_song_pos` | Integer | Next song position in queue |
| `play_state` | Integer | Player state: 0 = unknown, 1 = stop, 2 = play, 3 = pause |
| `playlist_directory` | String | path to the mpd playlist directory |
| `queue_length` | Integer | Length of the queue |
| `queue_version` | Integer | Version of the queue |
| `repeat` | Boolean | MPD repeat option |
| `replaygain` | Integer | 0 = off, 1 = track, 2 = album, 3 = auto, 4 = unknown |
| `random` | Boolean | MPD random option |
| `single_state` | Integer | MPD single state: 0 = off, 1 = on, 2 = oneshot, 3 = unknown |
| `song_id` | Integer | Song id of current song |
| `song_pos` | Integer | Current song position in queue |
| `start_time` | Integer | Current song start playing timestamp |
| `total_time` | Integer | Total time of current song |
| `volume` | Integer | 0 - 100 percent |

### mympd_state.current_song

| KEY | TYPE | DESCRIPTION |
| --- | ---- | ----------- |
| `uri` | String | Song uri |
| `Duration` | Integer | Song duration in seconds |
| Tag | Tag value(s) | MPD tag name with values. |

