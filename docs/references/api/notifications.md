---
layout: page
permalink: /references/api/notifications
title: Websocket Notifications
---

myMPD uses the idle protocol from mpd to detect state changes. These status changes and myMPD changes are broadcasted to all open websocket connections, respecting the partition specificity.

**Websocket endpoint:** `/ws/<partition>`

| MPD IDLE EVENT | MYMPD NOTIFY | DESCRIPTION |
|----------------|--------------| ----------- |
| MPD_IDLE_DATABASE | update_database | MPD database was updated |
| MPD_IDLE_STORED_PLAYLIST | update_stored_playlist | MPD playlist was updated |
| MPD_IDLE_QUEUE | update_queue | MPD queue has changed |
| MPD_IDLE_PLAYER | update_state | MPD player state has changed |
| MPD_IDLE_MIXER | update_volume | MPD volume has changed |
| MPD_IDLE_OUTPUT | update_outputs | MPD outputs are changed |
| MPD_IDLE_OPTIONS | update_options | MPD playback options are changed |
| MPD_IDLE_UPDATE | update_started | MPD database update was started |
| MPD_IDLE_UPDATE | update_finished | MPD database update has finished |
| n/a | mpd_connected | myMPD connected to MPD |
| n/a | mpd_disconnected | myMPD disconnected from MPD |
| n/a | notify | General notification |
| n/a | update_home | Home icons are changed |
| n/a | update_jukebox | Jukebox queue was changed |
| n/a | update_lastplayed | Last played list was changed |
| n/a | update_cache_started | myMPD cache update is started |
| n/a | update_cache_finished | myMPD cache updates has finished |
{: .table .table-sm }

The websocket endpoint accepts following messages:

| MESSAGE | RESPONSE | DESCRIPTION |
| ------- | -------- | ----------- |
| ping | pong | Keepalive |
| id:`<number>` | none | Used to send the jsonrpc id generated for this session |
{: .table .table-sm }
