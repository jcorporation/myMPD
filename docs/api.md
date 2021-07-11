---
layout: page
permalink: /api
title: API
---

myMPD uses a json-rpc 2 api for the communication between frontend and backend.

**API endpoint:** /api/

- [All API calls]({{ site.baseurl }}/api_methods)

### Example API call

```
{"jsonrpc":"2.0","id":0,"method":"MYMPD_API_PLAYER_STATE","params":{}}
```

## Websocket Notifications

myMPD uses the idle protocol from mpd to detect state changes. These status changes are broadcasted to all open websocket connections.

**Websocket endpoint:** /ws/

| MPD IDLE EVENT | MYMPD NOTIFY |
|----------------|--------------|
| MPD_IDLE_DATABASE | update_database |
| MPD_IDLE_STORED_PLAYLIST | update_stored_playlist |
| MPD_IDLE_QUEUE | update_queue |
| MPD_IDLE_PLAYER | update_state |
| MPD_IDLE_MIXER | update_volume |
| MPD_IDLE_OUTPUT | update_outputs |
| MPD_IDLE_OPTIONS | update_options |
| MPD_IDLE_UPDATE | update_started or update_finished |
| n/a | mpd_disconnected |
| n/a | mpd_connected |
| n/a | update_lastplayed |
| n/a | update_jukebox |
