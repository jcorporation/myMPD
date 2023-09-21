---
layout: page
permalink: /additional-topics/mpd-satellite-setup.md
title: MPD satellite setup
---

## Introduction

*This documentation is work in progress.*

The mpd satellite setup consists of following components:

- A central server hosting:
  - music collection
  - playlists
  - MPD database
  - MPD sticker database
  - myMPD images
  - myMPD webradio favorites
- A few embedded devices running mpd and myMPD to play and control the music locally

- [Discussion](https://github.com/jcorporation/myMPD/discussions/932)
- [MPD documentation](https://mpd.readthedocs.io/en/latest/user.html#satellite)

We use the same directory structure on each device.

- Music directory: `/srv/mpd/music`
- Playlist directory: `/srv/mpd/playlists`
- Images directory: `/srv/mpd/images`
- Webradio favorites directory: `/srv/mpd/webradios`

## Setup the central server

- Name: central.lan

Setting up the central server involves following steps:

1. Configure MPD
2. Export directories for
    - music files
    - playlists
    - images

### Configure MPD

**/etc/mpd.conf**
```
music_directory "/srv/mpd/music"
playlist_directory "/srv/mpd/playlists"
database {
    plugin "simple"
    path "/var/lib/mpd/tag_cache"
    cache_directory "/var/lib/mpd/cache"
}
sticker_file "/var/lib/mpd/sticker.sql"
bind_to_address "0.0.0.0"
```

## Export

We use NFS, but CIFS is also possible.

**/etc/exports**
```
/srv/mpd  *(ro,sync,no_subtree_check)
```

Create directories for central myMPD images.

```sh
mkdir /srv/mpd/images/backgrounds /srv/mpd/images/thumbs
```

## Setup a satellite

Setting up the satellite involves following steps:

1. Mount the exported directories
2. Configure MPD
3. Configure myMPD

### 1. Mount

We mount the exported directories at the os level. myMPD requires access to the music directory also.

**/etc/fstab**
```
central.lan:/srv/mpd /srv/mpd nfs soft,_netdev 0 0
```

### 2. Configure MPD

**/etc/mpd.conf**
```
music_directory "/srv/mpd/music"
playlist_directory "/srv/mpd/playlists"
database {
    plugin "proxy"
    host "central.lan"
    keepalive "yes"
}
bind_to_address "/run/mpd/socket"
```

### 3. Configure myMPD

There is no special myMPD configuration required, if each instance should work autonomously.

#### Central playback statistics

Configure myMPD to use the central MPD server for stickers. You find this setting in the connection dialog.

#### Shared images

Replace the `/var/lib/mympd/pics` directory with a link to `/srv/mpd/images`.

```sh
rm -r /var/lib/mympd/pics
ln -s /srv/mpd/images /var/lib/mympd/pics
```

#### Shared webradio favorites

Replace the `/var/lib/mympd/webradios` directory with a link to `/srv/mpd/webradios`.

```sh
rm -r /var/lib/mympd/webradios
ln -s /srv/mpd/webradios /var/lib/mympd/webradios
```

## Not working

Following functions are currently not supported with the satellite setup.

- Shared smart playlists across all myMPD instances
  - At the moment each myMPD instance creates it's MPD playlists and could overwrite MPD playlists from other instances. To prevent this set a different smart playlist prefix on each myMPD instance.
