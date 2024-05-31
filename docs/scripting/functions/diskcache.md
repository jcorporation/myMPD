---
layout: page
permalink: /scripting/functions/diskcache
title: Disk caches
---

## Cover cache

Helper function to write (rename) a file to the cover cache. The source file must be on the same filesystem as the cover cache directory (default: `/var/cache/mympd/cover`).

```lua
local rc = mympd.covercache_write(src, uri)
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| src | string | Source file to rename. |
| uri | string | Uri to write the cover cache for. |
{: .table .table-sm }

**Returns:**

0 on success.

## Lyrics cache

Helper function to write a entry (file) to the lyrics cache (default: `/var/cache/mympd/lyrics`).

```lua
local rc = mympd.mympd.lyricscache_write(str, uri)
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| str | string | String to save (it must be a valid lyrics json string) |
| tagvalue | string | Tag value to write the thumbs cache for. |
{: .table .table-sm }

**Returns:**

0 on success.

## Thumbs cache

Helper function to write (rename) a file to the thumbs cache. The source file must be on the same filesystem as the covercache directory (default: `/var/cache/mympd/thumbs`).

```lua
local rc = mympd.thumbscache_write(src, uri)
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| src | string | Source file to rename. |
| tagvalue | string | Tag value to write the thumbs cache for. |
{: .table .table-sm }

**Returns:**

0 on success.

## Temporary files

Generates a random tmp filename for the misc cache (default: `/var/cache/mympd/misc/XXXXXXXXXX`).

```lua
local tmp_file = mympd.tmp_file()
```
