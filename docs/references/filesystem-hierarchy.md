---
layout: page
permalink: /references/filesystem-hierarchy
title: Filesystem hierarchy
---

myMPD uses GNU standard installation directories.

| DIR / FILE | DESCRIPTION |
| ---------- | ----------- |
| /usr/bin/mympd | myMPD executable |
| /usr/bin/mympd-script | mympd-script executable |
| /var/cache/mympd/covercache/ | directory for caching embedded coverart |
| /var/lib/mympd/config/ | configuration files |
| /var/lib/mympd/pics/ | directory for stream coverart |
| /var/lib/mympd/pics/`<tagname>`/ | Pictures for <tagname> e.g. AlbumArtist, Artist, Genre, ... |
| /var/lib/mympd/state/ | state files |
| /var/lib/mympd/smartpls/ | directory for smart playlists |
| /var/lib/mympd/ssl/ | myMPD ssl ca and certificates, created on startup |
| /var/lib/mympd/scripts/ | directory for lua scripts|
| /var/lib/mympd/empty/ | empty directory |
| /etc/init.d/mympd | myMPD startscript (sysVinit or open-rc) |
| /usr/lib/systemd/system/mympd.service | systemd unit |
| /lib/systemd/system/mympd.service | systemd unit |
{: .table .table-sm }
