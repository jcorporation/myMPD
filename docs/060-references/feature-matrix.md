---
title: Feature matrix
---

Certain myMPD features are only available with new MPD versions. To use all myMPD functions you should use the latest stable MPD version.

!!! note
    myMPD support only MPD 0.22.6 and above.

| MYMPD FEATURE | MPD VERSION | DESCRIPTION |
| ------------- | ----------- | ----------- |
| Ratings for albums, playlists and tags | 0.24.0 | Stickers for playlists, filters and some tag types. |
| starts_with filter expression | 0.24.0 | myMPD uses a regex for older MPD versions. |
| Autodetection of pcre support | 0.24.0 | myMPD assumes pcre support for older MPD versions. |
| Autoconfiguration of playlist directory | 0.24.0 | Works only for socket connections. |
| Consume OneShot | 0.24.0 | OneShot mode for consume. |
| Queue save modes | 0.24.0 | Save queue as new playlist, replace playlist or append to playlist. |
| Queue sorting | 0.24.0 | Queue sorting and priority filter. |
| Add after current song (position/whence arg for load/searchadd) | 0.23.5 | Adds items to the queue after current playing song. |
| Insert into playlist (position arg for playlistadd) | 0.23.5 | Inserts songs into a playlist. |
| Remove range in playlist (range arg for playlistdelete) | 0.23.3 | Removes a range of songs in a playlist. |
