---
layout: page
permalink: /installation/docker
title: Docker
---

The Docker images are based on [Alpine Linux](https://alpinelinux.org). They are published through the GitHub docker registry [ghcr.io](https://github.com/jcorporation?tab=packages).

There are two images:
- mympd/mympd: the latest stable release
- mympd/mympd-devel: development version

Available architectures:
- x86-64 (amd64)
- arm64 (aarch64)
- armv7
- armv6

Use ``docker pull ghcr.io/jcorporation/mympd/mympd:latest`` to use the latest image.

## Usage

Docker Compose: 

```
---
version: "3.x"
services:
  mympd:
    image: ghcr.io/jcorporation/mympd/mympd
    container_name: mympd
    network_mode: "host"
    environment:
      - TZ=Europe/London
      - UMASK_SET=022 #optional
      - MYMPD_SSL=false
    volumes:
      - /path/to/mpd/socket:/run/mpd/socket #optional, use if you connect to mpd using sockets
      - /path/to/mympd/docker/dir:/var/lib/mympd/
      - /path/to/music/dir/:/music/:ro
      - /path/to/playlists/dir/:/playlists/:ro
    restart: unless-stopped
```

Docker CLI:

```
docker run -d \
  --name=mympd \
  --net="host" \
  -e TZ=Europe/London \
  -e UMASK_SET=022 \
  -e MYMPD_SSL=false \
  -v /path/to/mpd/socket:/run/mpd/socket \
  -v /path/to/mympd/docker/dir:/var/lib/mympd/ \
  -v /path/to/music/dir/:/music/:ro \
  -v /path/to/playlists/dir/:/playlists/:ro \
  --restart unless-stopped \
  ghcr.io/jcorporation/mympd/mympd
```

### myMPD configuration

You can configure some basic options of myMPD via startup options or environment variables.

- [Configuration]({{ site.baseurl }}/configuration/)

***

Since version 3.13 Alpine Linux image changes the definition of time_t on 32-bit systems. Read [Release Notes for Alpine 3.13.0](https://wiki.alpinelinux.org/wiki/Release_Notes_for_Alpine_3.13.0#time64_requirements) for further information's and a workaround.
