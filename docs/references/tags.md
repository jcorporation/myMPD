---
layout: page
permalink: /references/tags
title: Tags
---

myMPD supports all tags provided by MPD out of the box.

Following tags should be enabled for the best user experience (in MPD and in myMPD):

- AlbumArtist
- Artist
- Album
- Title
- Track
- Disc
- Genre
- Name (for displaying extended playlist information)

If you want use ListenBrainz for scrobbling you should enable additional tags:

- MUSICBRAINZ_RELEASETRACKID
- MUSICBRAINZ_TRACKID
- MUSICBRAINZ_ARTISTID
- MUSICBRAINZ_ALBUMARTISTID

## Hints

1. If you not enable the AlbumArtist tag, myMPD falls back to the Artist tag. To support compilations you should use the AlbumArtist tag and set it e. g. to `Various`.
2. `Genre` is in the default configuration used to build default smart playlists.
3. If no tags are enabled, myMPD uses the basename of the filename as title.
4. ID3 tags with multiple values works only for IDv2.4, older versions uses a separator for values in one tag and that is not supported by MPD.
5. MPD does not support multi-value MusicBrainz ID tags (https://github.com/MusicPlayerDaemon/MPD/issues/687).
