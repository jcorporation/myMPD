---
layout: page
permalink: /references/lyrics
title: Lyrics
---

myMPD supports synced and unsynced lyrics. Lyrics are displayed in the song details modal and you can add them to the playback card.

**Requirements:**

- myMPD must be compiled with flac/libid3tag support (default in prebuild packages)
- myMPD has access to the music directory

## Embedded lyrics

Lyrics can be embedded as USLT (unsynced) or SYLT (synced) tags in mp3 files (id3v2 tags) or in configurable vorbis comments in flac and ogg files. 

myMPD can parse the binary SYLT id3v2 tags and converts it to the lrc format. In vorbis comments myMPD expects an embedded lrc file for synced lyrics.

## Lyrics in extra files

As alternative myMPD tries to get the lyrics from a file in the same directory as the song with a configurable extension (default: `lrc` for synced lyrics and `.txt` for unsynced lyrics).

***

You can download lyrics with the lyrics download script from [https://github.com/jcorporation/musicdb-scripts](https://github.com/jcorporation/musicdb-scripts)
