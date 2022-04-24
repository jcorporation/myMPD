---
layout: page
permalink: /references/trigger
title: Trigger
---

Triggers are enabled if scripts are enabled. Triggers can call scripts with arguments. Triggers starting with `TRIGGER_MPD_` are triggered from the mpd idle events.

| TRIGGER | VALUE | DESCRIPTION |
| ------- | ----- | ----------- |
| TRIGGER_MYMPD_SCROBBLE | -1 | The song has been played for at least half of its duration, or for 4 minutes (whichever occurs earlier). |
| TRIGGER_MYMPD_START | -2 | |
| TRIGGER_MYMPD_STOP | -3 | |
| TRIGGER_MYMPD_CONNECTED | -4 | |
| TRIGGER_MYMPD_DISCONNECTED | -5 | |
| TRIGGER_MYMPD_FEEDBACK | -6 | Love or hate song feedback is set by user. Script is executed with arguments `uri` and `vote`. |
| TRIGGER_MPD_DATABASE | 1 | |
| TRIGGER_MPD_STORED_PLAYLIST | 2 | |
| TRIGGER_MPD_PLAYLIST | 4 | |
| TRIGGER_MPD_PLAYER | 8 | |
| TRIGGER_MPD_MIXER | 16 | |
| TRIGGER_MPD_OUTPUT | 32 | |
| TRIGGER_MPD_OPTIONS | 64 | |
| TRIGGER_MPD_UPDATE | 128 | |
| TRIGGER_MPD_STICKER | 256 | |
| TRIGGER_MPD_SUBSCRIPTION | 512 | |
| TRIGGER_MPD_MESSAGE | 1024 | |
| TRIGGER_MPD_PARTITION | 2048 | |
| TRIGGER_MPD_NEIGHBOR | 4096 | |
| TRIGGER_MPD_MOUNT | 8192 | |
{: .table .table-sm }
