---
layout: page
permalink: /references/api/
title: API Documentation
---

myMPD uses a JSON-RPC 2 for the communication between frontend and backend.

**API endpoint:** `/api/<partition>`

- [API reference]({{ site.baseurl }}/references/api/methods)

Notifications from the backend to the frontend are sent over a websocket connection.

**Websocket endpoint:** `/ws/<partition>`

- [Notification reference]({{ site.baseurl }}/references/api/notifications)

## Example API calls

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

To send a request with authentication data add a `X-myMPD-Session` header to it.

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
