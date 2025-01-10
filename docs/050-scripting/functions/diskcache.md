---
title: Disk caches
---

## Cover cache

Helper function to write (rename) a file to the cover cache. The source file must be on the same filesystem as the cache directory (default: `/var/cache/mympd/cover`).

```lua
local rc, name = mympd.cache_cover_write(src, uri, mimetype)
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| src | string | Source file to rename. |
| uri | string | Uri to write the cover cache for. |
| mimetype [1] | string | Mime Type, e.g. `image/png`, `nil` to sniff the mime type by magic bytes. |

**Returns:**

| FIELD | TYPE | DESCRIPTION |
| ----- | ---- | ----------- |
| rc | integer | 0 = success, 1 = error |
| name | string | written filename |

## Lyrics cache

Helper function to write a entry (file) to the lyrics cache (default: `/var/cache/mympd/lyrics`).

```lua
local rc, name = mympd.mympd.cache_lyrics_write(str, uri)
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| str | string | String to save (it must be a valid lyrics json string) |
| uri | string | Uri to write the lyrics cache for. |

**Returns:**

| FIELD | TYPE | DESCRIPTION |
| ----- | ---- | ----------- |
| rc | integer | 0 = success, 1 = error |
| name | string | written filename |

## Thumbs cache

Helper function to write (rename) a file to the thumbs cache. The source file must be on the same filesystem as the cache directory (default: `/var/cache/mympd/thumbs`).

```lua
local rc, name = mympd.cache_thumbs_write(src, value, mimetype)
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| src | string | Source file to rename. |
| value | string | Tag value to write the thumbs cache for. |
| mimetype [1] | string | Mime Type, e.g. `image/png`, `nil` to sniff the mime type by magic bytes. |

**Returns:**

| FIELD | TYPE | DESCRIPTION |
| ----- | ---- | ----------- |
| rc | integer | 0 = success, 1 = error |
| name | string | written filename |

## Temporary files

Creates a temporary file for the misc cache (default: `/var/cache/mympd/misc/XXXXXXXXXX`).

```lua
local tmp_file = mympd.tmp_file()
if tmp_file ~= nil then
  -- do something
end
```

**Parameters:**

No parameters required.

**Returns:**

| FIELD | TYPE | DESCRIPTION |
| ----- | ---- | ----------- |
| filename | string | Filename or `nil` on error. |

## Modification time

Updates the modification timestamp of a file.

```lua
local rc = mympd.mympd_caches_update_mtime(filename)
if rc == 1 then
  mympd.log(4, "Failure changing modification time of " .. filename)
end
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| filename | string | Filename for update. |

**Returns:**

| FIELD | TYPE | DESCRIPTION |
| ----- | ---- | ----------- |
| rc | integer | 0 = success, 1 = error |

***

- [1]

  Supported image mime types are: image/png, image/jpeg, image/webp, image/avif, image/svg+xml
