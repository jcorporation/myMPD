---
layout: page
permalink: /references/tags
title: Tags
---

myMPD supports all tags provided by MPD out of the box.

Following tags should be enabled for the best user experience (in MPD and in myMPD):

- Album
- AlbumArtist (for support of multiartist albums)
- Artist
- Disc (for support of multidisc albums)
- Genre
- Name (for displaying extended playlist information)
- Title
- Track

If you want use ListenBrainz for scrobbling you should enable additional tags:

- MUSICBRAINZ_ALBUMARTISTID
- MUSICBRAINZ_ARTISTID
- MUSICBRAINZ_RELEASETRACKID
- MUSICBRAINZ_TRACKID

I personally use [Picard](https://picard.musicbrainz.org/) to tag my music files.

## Important notes

- Disable the AlbumArtist tag if your music files are not tagged with it.
- If the AlbumArtist tag is not enabled, myMPD falls back to the Artist tag.
- If the AlbumArtist tag is empty, MPD and myMPD falls back to Artist tag for filters.
- To support multiartist albums you must use the AlbumArtist tag and set it e. g. to `Various`.
- To support multidisc albums you must use the Disc tag (numeric or in the format `1/2`).
- `Genre` is in the default configuration used to build default smart playlists.
- If no tags are enabled, myMPD uses the basename of the filename as title.
- ID3 tags with multiple values works only for IDv2.4, older versions uses a separator for values in one tag and that is not supported by MPD.
- MPD does not support multi-value MusicBrainz ID tags (https://github.com/MusicPlayerDaemon/MPD/issues/687). myMPD implements a workaround and splits the MUSICBRAINZ_ARTISTID and MUSICBRAINZ_ALBUMARTISTID tags by semicolon.
