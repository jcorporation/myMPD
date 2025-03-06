---
title: Home screen widgets
---

Scripts for home screen widges must return a valid http response. The response body will be inserted in the widget body.

You can style the widget with all [Bootstrap classes](https://getbootstrap.com/), inline css is forbidden.

Inline JavaScript is also forbidden, but you can call all available [functions](../jsdoc/index.html).

## Skeleton for a widget script

```lua
local headers ="Content-type: text/html\r\n"
local body = "<div class=\"text-center p-3\">Text</div>"

return mympd.http_reply("200", headers, body)
```

## Calling JavaScript functions

Add the `data-href` attribute to the html element. The content must be a valid serialized json object. `cmd` is the JavaScript function that is called with the given options on the click event.

```html
<!-- This calls gotoAlbum(data.AlbumId) -->
<a href="#" data-href='{"cmd":"gotoAlbum","options":[data.AlbumId]}'>Go</a>
```

### Common JavaScript functions

| FUNCTION | OPTIONS | DESCRIPTION |
| -------- | ------- | ----------- |
| `gotoAlbum` | AlbumId | Shows the album with given id. |
| `gotoAlbumList` | Tag Name, Tag Value | Shows the album list filtered by tag = value. |
| `gotoPlaylist` | uri | Playlist name. |
| `songDetails` | uri | Song URI |
