---
title: API Documentation
---

myMPD uses a JSON-RPC 2 for the communication between frontend and backend.

## JSON-RPC Id

The json rpc is the combination of two numeric ids.

1. Client ID: Generated on initial loading of the client, 6 digits
2. Request ID: Incremented on each request, 3 digits, wraps around

The websocket connection registers the Client ID on the webserver. This registration is used to send async responses from the webserver to a specific client.

## Endpoints

**API endpoint:** `/api/<partition>`

- [API reference](methods.md)

Notifications from the backend to the frontend are sent over a websocket connection.

**Websocket endpoint:** `/ws/<partition>`

- [Notification reference](notifications.md)

## Example API calls

``` json
{"jsonrpc":"2.0","id":0,"method":"MYMPD_API_PLAYER_STATE","params":{}}
```

``` json
{"jsonrpc":"2.0","id":0,"method":"MYMPD_API_PLAYER_VOLUME_SET","params":{"volume":60}}
```

## Pin protection

If myMPD is protected with a pin some methods require authentication with a special header.

### Authenticate

``` json
{"jsonrpc":"2.0","id":0,"method":"MYMPD_API_SESSION_LOGIN","params":{"pin": "<pin>"}}
```

Use the session string in the response for authenticated requests. The session is valid for 30 minutes.

### Authenticated request

To send a request with authentication data add a `X-myMPD-Session` header to it.

```text
X-myMPD-Session: <token>
```

### Validate the token

```text
Content-Type: application/json
X-myMPD-Session: <token>

{"jsonrpc":"2.0","id":0,"method":"MYMPD_API_SESSION_VALIDATE","params":{}}
```

### Logout

```text
Content-Type: application/json
X-myMPD-Session: <token>

{"jsonrpc":"2.0","id":0,"method":"MYMPD_API_SESSION_LOGOUT","params":{}}
```
