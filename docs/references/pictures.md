---
layout: page
permalink: /references/pictures
title: Pictures
---

# Albumart

myMPD supports local coverart for albums and streams.

## Albums

myMPD needs access to the MPD music_directory or a uptodate MPD.

You can download albumart with the script from [https://github.com/jcorporation/musicdb-scripts](https://github.com/jcorporation/musicdb-scripts)

### Albumart in directories through direct music directory access

1. If connected through the local MPD socket: myMPD gets the music_directory automatically, if music directory is set to auto.
2. If connected over network: set the music directory option in the connection dialog.

Enable albumart support:

1. To support albumart, songs must be arranged in folders per album.
2. Put the pictures and thumbnails in the album folders, name it always the same, e.g. cover-sm.webp (thumbnail) and cover.webp (full image)
3. Configure the name in the settings (albumart filenames and thumbnail names), a comma separated list of filenames and only basenames (filenames without extensions) are also supported

### Albumart in directories through MPD

1. only MPD >= 0.21 supports the albumart command
2. Put the pictures in the album folders, the basename must be cover (e.g. cover.jpg)

myMPD restricts the size to 5 MB.

### Embedded albumart

myMPD can extract embedded albumart, if it can access the music files.

Supported formats:
- id3v2 tagged MP3
- Vorbis Comments for FLAC and OGG

If MPD >= 0.22 is detected myMPD can get the embedded albumart through the readpicture command.

myMPD restricts the size to 5 MB.

#### Covercache

myMPD caches extracted covers under `/var/cache/mympd/covercache`. Files in this directory can be safely deleted. myMPD houskeeps the covercache on startup and each day.

You can disable the covercache by setting the `Covercache expiration` value to `0` days.

***

# Streams

1. Images must be named as the uri of the stream, replace the characters `<>/.:?&$!#\|;=` with `_`, e.g. `http___stream_laut_fm_nonpop.png` for uri `http://stream.laut.fm/nonpop`.
2. Put these images in the `/var/lib/mympd/pics/thumbs` folder.

# Pictures for other tags

The "Browse Database" grid view can display pictures for other tags than album also. To enable this simply create a directory with the tagname in the `/var/lib/mympd/pics` directory and put pictures with filename equal the tag value in this directory.

You can download artistart with the script from [https://github.com/jcorporation/musicdb-scripts](https://github.com/jcorporation/musicdb-scripts)

## Example

Create a directory named `AlbumArtist` under `/var/lib/mympd/pics`. Add pictures with the AlbumArtist name as filename in this directory.

***

# Home icon pictures

Pictures for the home icons must be placed in the directory `/var/lib/mympd/pics/thumbs`.

# Background images

Background images must be saved in the `/var/lib/mympd/pics/backgrounds` folder.

# Supported file extensions

myMPD recognizes following file extensions:

- webp, png, jpg, jpeg, svg, avif
