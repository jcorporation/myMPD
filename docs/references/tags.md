---
layout: page
permalink: /references/tags
title: Tags
---

myMPD supports all tags provided by MPD out of the box. The tags must be enabled in MPD and also in the myMPD settings.

Following tags should be enabled for the best user experience:

- Album
- AlbumArtist / MUSICBRAINZ_ALBUMID (for support of multiartist albums)
- Artist
- Date (to group songs by albums)
- Disc (to support multidisc albums)
- Genre
- Name (to display extended playlist information)
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
- If no tags are enabled, myMPD uses the basename of the filename as title.
- ID3 tags with multiple values works only for ID3v2.4, older versions uses a separator for values in one tag and that is not supported by MPD.
- MPD does not support multi-value MusicBrainz ID tags: [MPD issue](https://github.com/MusicPlayerDaemon/MPD/issues/687). myMPD implements a workaround and splits the MUSICBRAINZ_ARTISTID and MUSICBRAINZ_ALBUMARTISTID tags by semicolon.

## Albums

myMPD supports two modes for handling albums.

The default is the advanced album mode. It is the preferred one, except the mpd song database is very huge. You can configure the album mode via `/var/lib/mympd/config/album_mode`.

You can configure the tag for grouping albums by replacing the tag name in `/var/lib/mympd/config/album_group_tag`, it defaults to `Date`. Set it to `Unknown` to disable the additional grouping tag.

### Advanced album mode

The album mode queries each song to build the album cache. This can take a long time, if your mpd song database is very huge.

- If available myMPD uses the MUSICBRAINZ_ALBUMID to group songs into albums, else it falls back to AlbumArtist + Album + Date.
- To support multiartist albums you must use the MUSICBRAINZ_ALBUMID tag or the AlbumArtist tag and set it e. g. to `Various`.
- To support multiple versions of an album, you must maintain the MUSICBRAINZ_ALBUMID tag or the Date tag.
- To support multidisc albums you must use the Disc tag (numeric or in the format `1/2`).

### Simple album mode

The simple album mode uses a simple command to get all uniq album tags from mpd. It is faster than the advanced album mode, but does not provide the same feature set.

- myMPD uses the combination of Album + AlbumArtist + Date to identify uniq albums.
- All the advanced properties as genre, song count, album duration, etc. are not available.
