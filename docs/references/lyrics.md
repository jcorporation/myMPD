---
layout: page
permalink: /references/lyrics
title: Lyrics
---

myMPD supports synced and unsynced lyrics.

Lyrics can be embedded as USLT or SYLT tags in mp3 files (id3v2 tags) or in configurable vorbis comments in flac and ogg files.

Embedded lyrics only works if you enable flac/libid3tag at compile time (default in prebuild packages) and myMPD has access to the music directory.

myMPD can parse the binary SYLT id3v2 tags and converts it to the lrc format. In vorbis comments myMPD expects an embedded lrc file.

As alternative myMPD tries to get the lyrics from a file in the same directory as the song with a configurable extension.

Lyrics are displayed in the song details modal and you can add them to the playback card.

You can download lyrics with lyrics download script from [https://github.com/jcorporation/musicdb-scripts](https://github.com/jcorporation/musicdb-scripts)
