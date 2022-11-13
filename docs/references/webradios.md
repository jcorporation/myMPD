---
layout: page
permalink: /references/webradios
title: Webradios
---

## Online search

myMPD uses the great [radio-browser.info](https://www.radio-browser.info/) project for the online webradio search interface.

## Favorites

myMPD saves webradio favorites as single extended m3u files in the directory `/var/lib/mympd/webradios`. The filename is the sanitized stream uri.

MPD loads this playlist with the curl plugin, myMPD calculates the correct http uri.

### File format

```
#EXTM3U
#EXTINF:-1,<name of the webradio>
#EXTGENRE:<comma separated list of genres>
#PLAYLIST:<name of the webradio>
#EXTIMG:<coverimage filename, relative to /var/lib/mympd/pics/thumbs/ or http uri>
#HOMEPAGE:https://www.swr.de/swr1/
#COUNTRY:<country>
#LANGUAGE:<language>
#DESCRIPTION:<description>
#CODEC:<codec, eg. MP3, AAC>
#BITRATE:<bitrakte in kbit>
<stream uri>
```

### Example

```
#EXTM3U
#EXTINF:-1,SWR 1 BW
#EXTGENRE:Pop,Rock
#PLAYLIST:SWR 1 BW
#EXTIMG:https://jcorporation.github.io/webradiodb/db/pics/https___liveradio_swr_de_sw282p3_swr1bw_play_mp3.webp
#HOMEPAGE:https://www.swr.de/swr1/
#COUNTRY:Germany
#LANGUAGE:German
#DESCRIPTION:SWR 1 Baden-Württemberg
#CODEC:MP3
#BITRATE:128
https://liveradio.swr.de/sw282p3/swr1bw/play.mp3

```
