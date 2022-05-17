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

### Lyrics download

myMPD delivers a lyrics download script based on the [https://github.com/MusicPlayerDaemon/ncmpc/tree/master/lyrics](ncmpc plugins).

You can find this script in the [https://github.com/jcorporation/myMPD/tree/master/contrib/scripts](contrib/scripts/) folder.

The script tries all plugins from the lyrics folder and adds all the results to a .txt file. You should afterwards edit this file and select the best result.

```
Usage: lyric.sh <file|directory>

```

- Works only for ID3v2 tagged MP3 files
- If you provide a directory the script searches recursively for MP3 files
- Name of the result file is the name of the MP3 file with .txt extension

**Depenedencies**: python, mid3v2 (mutagen)
