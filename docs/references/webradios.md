---
layout: page
permalink: /references/webradios
title: Webradios
---

## Online search

myMPD uses the great [radio-browser.info](https://www.radio-browser.info/) project for the online webradio search interface.

## Favorites

myMPD saves webradio favorites as single extended m3u files in the directory `/var/lib/mympd/webradios`. The filename is the sanitized stream uri.

MPD loads this playlist with the curl plugin.

### File format

```
#EXTM3U
#EXTINF:-1,<name of the webradio>
#EXTGENRE:
#PLAYLIST:<name of the webradio>
#EXTIMG:<coverimage url, relative to /var/lib/mympd/pics/ or http uri>
#RADIOBROWSERUUID:<station uid from radio-browser.info>
<stream uri>
```

### Example
```
#EXTM3U
#EXTINF:-1,SWR1 BW
#EXTGENRE:
#PLAYLIST:SWR1 BW
#EXTIMG:swr-swr1-bw_cast_addradio_de_swr_swr1_bw_mp3_128_stream_mp3.jpg
#RADIOBROWSERUUID:d8f01eea-26be-4e3d-871d-7596e3ab8fb1
https://liveradio.swr.de/sw331ch/swr1bw/play.mp3
```
