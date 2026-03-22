Feature matrix
==============

Certain myMPD features are only available with new MPD versions. To use all myMPD functions you should use the latest stable MPD version.

.. include:: _includes/mpd-compat.rst

+------------------------------------------+-------------+------------------------------------------------------+
| MYMPD FEATURE                            | MPD VERSION | DESCRIPTION                                          |
+==========================================+=============+======================================================+
| String normalization and diactrics       | 0.25.0      | String normaliazion for song search.                 |
| stripping                                |             |                                                      |
+------------------------------------------+-------------+------------------------------------------------------+
| Ratings for albums, playlists and tags   | 0.24.0      | Stickers for playlists, filters and some tag types.  |
+------------------------------------------+-------------+------------------------------------------------------+
| starts_with filter expression            | 0.24.0      | myMPD uses a regex for older MPD versions.           |
+------------------------------------------+-------------+------------------------------------------------------+
| Autodetection of pcre support            | 0.24.0      | myMPD assumes pcre support for older MPD versions.   |
+------------------------------------------+-------------+------------------------------------------------------+
| Autoconfiguration of playlist directory  | 0.24.0      | Works only for socket connections.                   |
+------------------------------------------+-------------+------------------------------------------------------+
| Consume OneShot                          | 0.24.0      | OneShot mode for consume.                            |
+------------------------------------------+-------------+------------------------------------------------------+
| Queue save modes                         | 0.24.0      | Save queue as new playlist, replace playlist or      |
|                                          |             | append to playlist.                                  |
+------------------------------------------+-------------+------------------------------------------------------+
| Queue sorting                            | 0.24.0      | Queue sorting and priority filter.                   |
+------------------------------------------+-------------+------------------------------------------------------+
