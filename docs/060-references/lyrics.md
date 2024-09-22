---

title: Lyrics
---

myMPD supports synced and unsynced lyrics. Lyrics are displayed in the song details modal and you can add them to the playback card.

**Requirements:**

- myMPD must be compiled with flac / libid3tag support (default in pre-build packages)
- myMPD has access to the music directory

## Embedded lyrics

Lyrics can be embedded as USLT (unsynced) or SYLT (synced) tags in mp3 files (id3v2 tags) or in configurable vorbis comments in flac and ogg files.

myMPD can parse the binary SYLT id3v2 tags and converts it to the lrc format. In vorbis comments myMPD expects an embedded lrc file for synced lyrics.

## Lyrics in extra files

As alternative myMPD tries to get the lyrics from a file in the same directory as the song with the same basename and a configurable extension (default: `lrc` for synced lyrics and `.txt` for unsynced lyrics).

You can download lyrics with the lyrics download script from [https://github.com/jcorporation/musicdb-scripts](https://github.com/jcorporation/musicdb-scripts)

## Script to fetch lyrics on demand

If no local lyrics are found, myMPD emits the `mympd_lyrics` trigger. Attach a script to fetch and deliver lyrics to it. Only one script is supported for this event.

A fully working example implementation can be found in the [mympd-scripts repository](https://github.com/jcorporation/mympd-scripts/tree/main/Lyrics).

This downloaded lyrics are saved in the lyrics cache `/var/cache/mympd/lyrics`.

You can use the following script to regularly move the downloaded lyrics to your music directory.

```sh
#!/bin/bash

# Define your MPD music directory here
MUSIC_DIR="/var/lib/mpd/music"

for F in /var/cache/mympd/lyrics/*.json
do
    [ ! -f "$F" ] && break
    URI=$(jq -r '.uri' < "$F")
    SYNCED=$(jq -r '.synced' < "$F")
    TEXT=$(jq -r '.text' < "$F")
    if [ "$SYNCED" == "true" ]
    then
        EXT="lrc"
    else
        EXT="txt"
    fi
    LYRICS_FILE="${URI%.*}.$EXT"
    if [ ! -f "$MUSIC_DIR/$LYRICS_FILE" ]
    then
        echo "Writing $MUSIC_DIR/$LYRICS_FILE"
        printf "%s" "$TEXT" > "$MUSIC_DIR/$LYRICS_FILE"
        rm "$F"
    fi
done
```
