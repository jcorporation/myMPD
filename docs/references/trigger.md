---
layout: page
permalink: /references/trigger
title: Trigger
---

Triggers are enabled if scripts are enabled. Triggers can call scripts with arguments. Triggers starting with `TRIGGER_MPD_` are triggered from the mpd idle events.

| TRIGGER | VALUE | SCOPE | DESCRIPTION |
| ------- | ----- | ----- | ----------- |
| TRIGGER_MYMPD_SCROBBLE | -1 | Partition | The song has been played for at least half of its duration, or for 4 minutes (whichever occurs earlier). |
| TRIGGER_MYMPD_START | -2 | Global | myMPD was started, but not connected to MPD. |
| TRIGGER_MYMPD_STOP | -3 | Global | myMPD is stopping. |
| TRIGGER_MYMPD_CONNECTED | -4 | Partition | MPD connection is established. |
| TRIGGER_MYMPD_DISCONNECTED | -5 | Partition | MPD is disconnected. |
| TRIGGER_MYMPD_FEEDBACK | -6 | Partition | Love, hate or rating feedback is set by user. Script is executed with arguments `uri`, `vote` and `type`. |
| TRIGGER_MYMPD_SKIPPED | -7 | Partition | Song was skipped. |
| TRIGGER_MYMPD_LYRICS | -8 | Global | Triggers if MYMPD_API_LYRICS_GET has found no lyrics. Script is executed with arguments `uri` and should return a JSONRPC response. |
| TRIGGER_MPD_DATABASE | 1 | Default partition | Database has been modified. |
| TRIGGER_MPD_STORED_PLAYLIST | 2 | Default partition | A playlist was added, removed or changed. |
| TRIGGER_MPD_QUEUE | 4 | Partition | MPD queue has changed. |
| TRIGGER_MPD_PLAYER | 8 | Partition | MPD player state has changed. |
| TRIGGER_MPD_MIXER | 16 | Partition | MPD mixer state (volume) has changed. |
| TRIGGER_MPD_OUTPUT | 32 | Partition | Output configuration has changed. |
| TRIGGER_MPD_OPTIONS | 64 | Partition | MPD player options has changed. |
| TRIGGER_MPD_UPDATE | 128 | Default partition | Database update has started or finished. |
| TRIGGER_MPD_STICKER | 256 | Global | Sticker database has changed. |
| TRIGGER_MPD_PARTITION | 2048 | Default partition | Partition was added or removed. |
{: .table .table-sm }
