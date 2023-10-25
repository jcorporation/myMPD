---
layout: page
permalink: /references/pictures
title: Pictures
---

myMPD supports local coverart for albums, streams and tags.

## Albumart

myMPD can get albumart through through direct access to the MPD music directory or through the MPD protocol.

### Direct access

The fastest and most flexible way is through direct access with albumart in folders.

This offers:

- Configurable albumart filenames
- Support of thumbnails
- Support of more than one image, e. g. cover, back, inlet

**Configure the music directory:**

1. If connected through the local MPD socket: myMPD gets the music_directory automatically, if the music directory option is set to `auto`.
2. If connected over network: set the music directory option in the connection dialog.

**Enable albumart support:**

1. Albumart in folders per album
  - Put the pictures and thumbnails in the album folders, name it always the same, e.g. cover-sm.webp (thumbnail) and cover.webp (full image)
  - Configure the name in the settings (albumart filenames and thumbnail names), a comma separated list of filenames and only basenames (filenames without extensions) are also supported
2. Embedded albumart in the music files
  - myMPD must be compiled with flac/libid3tag support (default in prebuild packages)
  - Supported formats are id3v2 for MP3 and Vorbis Comments for FLAC and OGG
  - myMPD reads all embedded images, not only the first one as MPD.

### Through MPD protocol

This is useful if myMPD does not run on the same host as MPD.

1. Albumart in folders per album
  - Only MPD >= 0.21 supports the albumart command
  - Put the albumart in the album folders, the basename must be cover (e.g. cover.jpg), this is not configurable
2. Embedded albumart in the music files
  - Only MPD >= 0.22 supports the readpicture command
  - Only first image is read

myMPD restricts the size to 5 MB.

## Streams

1. Images must be named as the uri of the stream, replace the characters `<>/.:?&$!#\|;=` with `_`, e.g. `http___stream_laut_fm_nonpop.png` for uri `http://stream.laut.fm/nonpop`.
2. Put these images in the `/var/lib/mympd/pics/thumbs` folder.

## Pictures for other tags

The "Browse Database" grid view can display pictures for other tags than album also. To enable this simply create a directory with the tagname in the `/var/lib/mympd/pics` directory and put pictures with filename equal the tag value in this directory.

You can download artistart with the script from [https://github.com/jcorporation/musicdb-scripts](https://github.com/jcorporation/musicdb-scripts)

Tag values are case sensitive and are sanitized: `/` is replaced with `_` (`AC/DC` -> `AC_DC`)

### Example

Create a directory named `AlbumArtist` under `/var/lib/mympd/pics`. Add pictures with the AlbumArtist name as filename in this directory.

## Other pictures

### Home icon pictures

Pictures for the home icons must be placed in the directory `/var/lib/mympd/pics/thumbs`.

### Background images

Background images must be saved in the `/var/lib/mympd/pics/backgrounds` folder.

## Supported file extensions

myMPD recognizes following file extensions:

- webp, png, jpg, jpeg, svg, avif

## Custom placeholder images

You can add custom placeholder images for albumart.

- `/var/lib/mympd/pics/thumbs/coverimage-booklet.webp`
- `/var/lib/mympd/pics/thumbs/coverimage-mympd.webp`
- `/var/lib/mympd/pics/thumbs/coverimage-notavailable.webp`
- `/var/lib/mympd/pics/thumbs/coverimage-stream.webp`

You can use every supported file extension.

## Covercache

myMPD caches covers in the folder `/var/cache/mympd/covercache`. Files in this folder can be safely deleted. myMPD housekeeps the covercache on startup and each day.

You can disable the covercache by setting the `Covercache expiration` value to `0` days.
