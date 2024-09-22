---
layout: page
permalink: /references/feature-matrix
title: Feature matrix
---

Certain myMPD features are only available with new MPD versions. To use all myMPD functions you should use the latest stable MPD version.

<div class="alert alert-warning">
myMPD support only MPD 0.21.0 and above.
</div>

| MYMPD FEATURE | MPD VERSION | DESCRIPTION |
| ------------- | ----------- | ----------- |
| starts_with filter expression | 0.24.0 | myMPD uses a regex for older MPD versions |
| Autodetection of pcre support | 0.24.0 | myMPD assumes pcre support for older MPD versions |
| Autoconfiguration of playlist directory | 0.24.0 | Works only for socket connections |
| Consume OneShot | 0.24.0 | OneShot mode for consume |
| Queue save modes | 0.24.0 | Save queue as new playlist, replace playlist or append to playlist |
| Queue sorting | 0.24.0 | Queue sorting and priority filter |
| Add after current song (position/whence arg for load/searchadd) | 0.23.5 | Adds items to the queue after current playing song |
| Insert into playlist (position arg for playlistadd) | 0.23.5 | Inserts songs into a playlist |
| Remove range in playlist (range arg for playlistdelete) | 0.23.3 | Removes a range of songs in a playlist |
| Partitions | 0.22.0 | Concurrent partition support |
| Embedded albumart (readpicture) | 0.22.0 | myMPD can read pictures tags directly if it has access to the musicdirectory |
{: .table .table-sm }
