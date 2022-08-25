---
layout: page
permalink: /references/trigger
title: Trigger
---

Triggers are enabled if scripts are enabled. Triggers can call scripts with arguments. Triggers starting with `TRIGGER_MPD_` are triggered from the mpd idle events.

| TRIGGER | VALUE | DESCRIPTION |
| ------- | ----- | ----------- |
| TRIGGER_MYMPD_SCROBBLE | -1 | Partition specific: The song has been played for at least half of its duration, or for 4 minutes (whichever occurs earlier). |
| TRIGGER_MYMPD_START | -2 | Global: myMPD was started, but not connected to MPD |
| TRIGGER_MYMPD_STOP | -3 | Global: myMPD is stopping |
| TRIGGER_MYMPD_CONNECTED | -4 | Partition specific: MPD connection is established |
| TRIGGER_MYMPD_DISCONNECTED | -5 | Partition specific: MPD is disconnected |
| TRIGGER_MYMPD_FEEDBACK | -6 | Partition specific: Love or hate song feedback is set by user. Script is executed with arguments `uri` and `vote`. |
| TRIGGER_MPD_DATABASE | 1 | Only for default partition: Database has been modified |
| TRIGGER_MPD_STORED_PLAYLIST | 2 | Only for default partition: A playlist was added, removed or changed |
| TRIGGER_MPD_QUEUE | 4 | Partition specific: MPD queue has changed |
| TRIGGER_MPD_PLAYER | 8 | Partition specific: MPD player state has changed |
| TRIGGER_MPD_MIXER | 16 | Partition specific MPD mixser state (volume) has changed |
| TRIGGER_MPD_OUTPUT | 32 | Partition specific: Ouput configuraton has changed |
| TRIGGER_MPD_OPTIONS | 64 | Partition specific: MPD player options has changed |
| TRIGGER_MPD_UPDATE | 128 | Only for default partition: Database update has started or finished |
| TRIGGER_MPD_PARTITION | 2048 | Only for default partition: Partition was added or removed |
{: .table .table-sm }
