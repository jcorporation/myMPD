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

## Hints

1. If you not enable the AlbumArtist tag, myMPD falls back to the Artist tag. To support compilations you should use the AlbumArtist tag and set it e. g. to `Various`.
2. `Genre` is in the default configuration used to build default smart playlists.
3. If no tags are enabled, myMPD uses the basename of the filename as title.


# Lyrics

myMPD supports synced and unsynced lyrics.

Lyrics can be embedded as USLT or SYLT tags in mp3 files (id3v2 tags) or in configurable vorbis comments in flac and ogg files.

Embedded lyrics only works if you enable flac/libid3tag at compile time (default in prebuild packages) and myMPD has access to the music directory.

myMPD can parse the binary SYLT id3v2 tags and converts it to the lrc format. In vorbis comments myMPD expects an embedded lrc file.

As alternative myMPD tries to get the lyrics from a file in the same directory as the song with a configurable extension.

Lyrics are displayed in the song details modal and you can add them to the playback card.
