---
title: Disk caches
---

## Cover cache

Helper function to write (rename) a file to the cover cache. The source file must be on the same filesystem as the cache directory (default: `/var/cache/mympd/cover`).

```lua
local rc, name = mympd.cache_cover_write(src, uri)
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| src | string | Source file to rename. |
| uri | string | Uri to write the cover cache for. |

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
local rc, name = mympd.cache_thumbs_write(src, value)
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| src | string | Source file to rename. |
| value | string | Tag value to write the thumbs cache for. |

**Returns:**

| FIELD | TYPE | DESCRIPTION |
| ----- | ---- | ----------- |
| rc | integer | 0 = success, 1 = error |
| name | string | written filename |

## Temporary files

Generates a random tmp filename for the misc cache (default: `/var/cache/mympd/misc/XXXXXXXXXX`).

```lua
local tmp_file = mympd.tmp_file()
```

## Modification time

Updates the timestamp of a file.

```lua
local rc = mympd.mympd_caches_update_mtime(filename)
if rc == 1 then
    mympd.log(4, "Failure changing modification time of " .. filename)
fi
```
