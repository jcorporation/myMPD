---
layout: page
permalink: /references/filesystem-hierarchy
title: Filesystem hierarchy
---

myMPD uses GNU standard installation directories.

| DIR / FILE | DESCRIPTION |
| ---------- | ----------- |
| /etc/init.d/mympd | myMPD startscript (sysVinit or open-rc) |
| /lib/systemd/system/mympd.service | Systemd unit |
| /lib/systemd/user/mympd.service | Systemd user unit |
| /usr/bin/mympd | myMPD executable |
| /usr/bin/mympd-script | Executable to trigger and post myMPD scripts |
| /var/cache/mympd/ | myMPD cache directory |
| /var/cache/mympd/cover/ | Directory for caching embedded coverart |
| /var/cache/mympd/lyrics/ | Directory for caching lyrics |
| /var/cache/mympd/misc/ | Directory for misc caching or tmp files |
| /var/cache/mympd/webradiodb/ | Directory for caching the webradiodb json file |
| /var/lib/mympd/ | myMPD state directory |
| /var/lib/mympd/config/ | Configuration files owned by root |
| /var/lib/mympd/empty/ | Intentionally empty directory |
| /var/lib/mympd/pics/ | Root folder for images |
| /var/lib/mympd/pics/backgrounds/ | Backgroundimages |
| /var/lib/mympd/pics/playlists/ | Images for playlists |
| /var/lib/mympd/pics/thumbs/ | Folder for homeicon, webradio, playlist and stream images |
| /var/lib/mympd/pics/`<tagname>`/ | Images for <tagname> e.g. AlbumArtist, Artist, Genre, ... |
| /var/lib/mympd/scripts/ | Directory for lua scripts |
| /var/lib/mympd/smartpls/ | Directory for smart playlists |
| /var/lib/mympd/ssl/ | myMPD ssl ca and certificates, created on startup |
| /var/lib/mympd/state/ | Global state files |
| /var/lib/mympd/state/`<partition>` | Partition specific state files |
| /var/lib/mympd/tags/ | Directory for caches |
{: .table .table-sm }
