---
layout: page
permalink: /references/api/
title: API
---

myMPD uses a json-rpc 2 api for the communication between frontend and backend.

**API endpoint:** `/api/<partition>`

- [API reference]({{ site.baseurl }}/references/api/methods)

### Example API call

```
{"jsonrpc":"2.0","id":0,"method":"MYMPD_API_PLAYER_STATE","params":{}}
```

```
{"jsonrpc":"2.0","id":0,"method":"MYMPD_API_PLAYER_VOLUME_SET","params":{"volume":60}}
```

## Pin protection

If myMPD is protected with a pin some methods require authentication with a special header.

### Authenticate

```
{"jsonrpc":"2.0","id":0,"method":"MYMPD_API_SESSION_LOGIN","params":{"pin": "<pin>"}}
```

Use the session string in the response for authenticated requests. The session is valid for 30 minutes.

### Authenticated request

To send a request with authentication data add an `X-myMPD-Session` header to it.

```
X-myMPD-Session: <token>
```

### Validate the token

```
Content-Type: application/json
X-myMPD-Session: <token>

{"jsonrpc":"2.0","id":0,"method":"MYMPD_API_SESSION_VALIDATE","params":{}}
```

### Logout

```
Content-Type: application/json
X-myMPD-Session: <token>

{"jsonrpc":"2.0","id":0,"method":"MYMPD_API_SESSION_LOGOUT","params":{}}
```

## Websocket Notifications

myMPD uses the idle protocol from mpd to detect state changes. These status changes are broadcasted to all open websocket connections.

**Websocket endpoint:** `/ws/<partition>`

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
| n/a | update_home |
{: .table .table-sm }
