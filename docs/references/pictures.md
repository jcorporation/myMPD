---
layout: page
permalink: /references/pictures
title: Pictures
---

# Albumart

myMPD supports local coverart for albums and streams.

## Albums

myMPD needs access to the MPD music_directory or a uptodate MPD.

### Albumart in directories through direct music directory access

1. If connected over the local MPD socket: myMPD gets the music_directory automatically, if music directory is set to auto.
2. If connected over tcp/ip: set the music directory option in the connection dialog.

Enable albumart support:

1. To support albumart, songs must be arranged in folders per album.
2. Put the pictures in the album folders, name it always the same, e.g. folder.jpg
3. Configure the name in the settings, a comma separated list and basenames are also supported
4. Supported file extensions are png, jpg, jpeg, tiff, bmp, svg, webp

### Albumart in directories through MPD

1. only MPD >= 0.21 supports the albumart command
2. Put the pictures in the album folders, the basename must be cover (e.g. cover.jpg)
3. Supported file extensions are png, jpg, jpeg, tiff, bmp

### Embedded albumart

myMPD can extract embedded albumart, if it can access the music files. If MPD >= 0.22 is detected myMPD can get the embedded albumart through the readpicture command.

#### Covercache

myMPD caches extracted covers under `/var/lib/mympd/covercache`. Files in this directory can be safely deleted. myMPD houskeeps the covercache on startup and each two hours.

***

# Streams

1. Images must be named as the uri of the stream, remove http:// and replace dots, colons and slashes with underscores, e.g. stream_laut_fm_nonpop.png for uri http://stream.laut.fm/nonpop.
2. Put these images under `/var/lib/mympd/pics/`.
3. Supported file extensions are png, jpg, jpeg, tiff, bmp, svg, webp

# Pictures for other tags

The "Browse Database" grid view can display pictures for other tags than album also. To enable this simply create a directory with the tagname in the `/var/lib/mympd/pics` directory and put pictures with filename equal the tag value in this directory. 

Supported file extensions are png, jpg, jpeg, tiff, bmp, svg, webp.

## Example

Create a directory named `AlbumArtist` under `/var/lib/mympd/pics`. Add pictures with the AlbumArtist name as filename in this directory.

***

# Home icon pictures

Pictures for the home icons mus be placed in the directory `/var/lib/mympd/pics`. Supported file extensions are png, jpg, jpeg, tiff, bmp, svg and webp.
