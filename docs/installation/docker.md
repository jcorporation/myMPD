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

Use `docker pull ghcr.io/jcorporation/mympd/mympd:latest` to use the latest image.

## Usage

Starts the myMPD docker container:

- Runs the docker container with uid/gid 1000
- Disables SSL
- Listen on port 8080

Docker Compose:

```
---
version: "3.x"
services:
  mympd:
    image: ghcr.io/jcorporation/mympd/mympd
    container_name: mympd
    network_mode: "host"
    user: 1000:1000
    environment:
      - UMASK_SET=022
      - MYMPD_SSL=false
      - MYMPD_HTTP_PORT=8080
    volumes:
      - /path/to/mpd/socket:/run/mpd/socket
      - /path/to/mympd/docker/workdir:/var/lib/mympd/
      - /path/to/mympd/docker/cachedir:/var/lib/mympd/
      - /path/to/music/dir/:/music/:ro
      - /path/to/playlists/dir/:/playlists/:ro
    restart: unless-stopped
```

Docker CLI:

```
docker run -d \
  --name=mympd \
  --net="host" \
  -u 1000:1000 \
  -e UMASK_SET=022 \
  -e MYMPD_SSL=false \
  -e MYMPD_HTTP_PORT=8080 \
  -v /path/to/mpd/socket:/run/mpd/socket \
  -v /path/to/mympd/docker/workdir:/var/lib/mympd/ \
  -v /path/to/mympd/docker/cachedir:/var/lib/mympd/ \
  -v /path/to/music/dir/:/music/:ro \
  -v /path/to/playlists/dir/:/playlists/:ro \
  --restart unless-stopped \
  ghcr.io/jcorporation/mympd/mympd
```

## myMPD configuration

You can configure some basic options of myMPD via startup options or environment variables.

- [Configuration]({{ site.baseurl }}/configuration/)
