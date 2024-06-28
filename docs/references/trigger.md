---
layout: page
permalink: /references/trigger
title: Trigger
---

Triggers are enabled if scripts are enabled. Triggers can call scripts with arguments. Triggers starting with `TRIGGER_MPD_` are triggered from the mpd idle events.

| TRIGGER | VALUE | SCOPE | DESCRIPTION |
| ------- | ----- | ----- | ----------- |
| TRIGGER_MYMPD_SCROBBLE | -1 | Partition | The song has been played for at least half of its duration, or for 4 minutes (whichever occurs earlier). [Example](https://github.com/jcorporation/mympd-scripts/blob/main/ListenBrainz/ListenBrainz-Scrobbler.lua) |
| TRIGGER_MYMPD_START | -2 | Global | myMPD was started, but not connected to MPD. |
| TRIGGER_MYMPD_STOP | -3 | Global | myMPD is stopping. |
| TRIGGER_MYMPD_CONNECTED | -4 | Partition | MPD connection is established. |
| TRIGGER_MYMPD_DISCONNECTED | -5 | Partition | MPD is disconnected. |
| TRIGGER_MYMPD_FEEDBACK | -6 | Partition | Love, hate or rating feedback is set by user. Script is executed with arguments `uri`, `vote` and `type`. [Example](https://github.com/jcorporation/mympd-scripts/blob/main/ListenBrainz/ListenBrainz-Feedback.lua) |
| TRIGGER_MYMPD_SKIPPED | -7 | Partition | Song was skipped. |
| TRIGGER_MYMPD_LYRICS | -8 | Global | Triggers if MYMPD_API_LYRICS_GET has found no lyrics. Only one script is supported for this event. Script is executed with the argument `uri` and should return an JSONRPC response. [Example](https://github.com/jcorporation/mympd-scripts/tree/main/Lyrics) |
| TRIGGER_MYMPD_ALBUMART | -9 | Global | Triggers if no local albumart was found. Only one script is supported for this event. Script is executed with the argument `uri` and should return a raw http response. [Example](https://github.com/jcorporation/mympd-scripts/blob/main/Albumart)|
| TRIGGER_MYMPD_TAGART | -10 | Global | Triggers if no local tagart was found. Only one script is supported for this event. Script is executed with arguments `tag` and `value` and should return a raw http response. [Example](https://github.com/jcorporation/mympd-scripts/tree/main/Tagart)|
| TRIGGER_MYMPD_JUKEBOX | -11 | Partition | Triggers if jukebox is configured with the mode `script`. Only one script is supported for this event. This script must fill the jukebox queue and add songs from this queue to the MPD queue. [Example](https://github.com/jcorporation/mympd-scripts/tree/main/Jukebox) |
| TRIGGER_MPD_DATABASE | 1 | Default partition | Database has been modified. |
| TRIGGER_MPD_STORED_PLAYLIST | 2 | Global | A playlist was added, removed or changed. |
| TRIGGER_MPD_QUEUE | 4 | Partition | MPD queue has changed. |
| TRIGGER_MPD_PLAYER | 8 | Partition | MPD player state has changed. |
| TRIGGER_MPD_MIXER | 16 | Partition | MPD mixer state (volume) has changed. |
| TRIGGER_MPD_OUTPUT | 32 | Partition | Output configuration has changed. |
| TRIGGER_MPD_OPTIONS | 64 | Partition | MPD player options has changed. |
| TRIGGER_MPD_UPDATE | 128 | Default partition | MPD Database update has started or finished. |
| TRIGGER_MPD_STICKER | 256 | Global | MPD Sticker database has changed. |
| TRIGGER_MPD_SUBSCRIPTION | 512 | Global | MPD client has subscribed or unsubscribed to a channel. |
| TRIGGER_MPD_MESSAGE | 1024 | Global | A message was received on a channel this client is subscribed to. Only one script is supported for this event. |
| TRIGGER_MPD_PARTITION | 2048 | Default partition | Partition was added or removed. |
{: .table .table-sm }
