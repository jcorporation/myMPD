Docker
======

The Docker images are based on `Alpine Linux <https://alpinelinux.org>`__. They are published through the GitHub docker registry `ghcr.io <https://github.com/jcorporation?tab=packages>`__.

There are two images:

- `mympd/mympd <https://github.com/users/jcorporation/packages/container/package/mympd%2Fmympd>`__: the latest stable release
- `mympd/mympd-devel <https://github.com/users/jcorporation/packages/container/package/mympd%2Fmympd-devel>`__: development version

Available architectures:

- x86-64 (amd64)
- arm64 (aarch64)
- armv7
- armv6

Use ``docker pull ghcr.io/jcorporation/mympd/mympd:latest`` to use the latest stable image.

Usage
-----

Starts the myMPD docker container:

- Runs the docker container with uid/gid 1000
- myMPD can not find the correct IP address if it is behind a docker proxy, set ``MYMPD_SSL_SAN`` to ``IP:<host ip>``.
- Set ``MYMPD_MYMPD_URI`` to ``http://<host fqdn>:<host port>``.

Volumes
~~~~~~~

+-----------------------------+--------------------------------------------------------------------------------------+
| VOLUME                      | DESCRIPTION                                                                          |
+=============================+======================================================================================+
| ``/run/mpd/socket``         | MPD listening socket.                                                                |
+-----------------------------+--------------------------------------------------------------------------------------+
| ``/run/mygpiod/socket``     | Optional myGPIOd socket for GPIO scripting support.                                  |
+-----------------------------+--------------------------------------------------------------------------------------+
| ``/docker/mympd/workdir``   | myMPD working directory. Must exist and be writeable by uid 1000.                    |
+-----------------------------+--------------------------------------------------------------------------------------+
| ``/docker/mympd/cachedir``  | myMPD cache directory. Must exist and be writeable by uid 1000.                      |
+-----------------------------+--------------------------------------------------------------------------------------+
| ``/var/lib/mpd/music/``     | MPD music directory. Use the same path in the container to enable auto detection.    |
+-----------------------------+--------------------------------------------------------------------------------------+
| ``/var/lib/mpd/playlists/`` | MPD playlist directory. Use the same path in the container to enable auto detection. |
+-----------------------------+--------------------------------------------------------------------------------------+

You must create the ``cachedir`` and ``workdir`` before starting the container.

Docker Compose
~~~~~~~~~~~~~~

Save this as ``docker-compose.yml``:

.. code:: yaml

   services:
     mympd:
       image: ghcr.io/jcorporation/mympd/mympd
       container_name: mympd
       ports:
         - 8080:8080
         - 8443:8443
       user: 1000:1000
       environment:
         TZ: Europe/Berlin
         MYMPD_SSL_SAN: "DNS:<host fqdn>"
         MYMPD_MYMPD_URI: http://<host fqdn>:8080
       volumes:
         - /run/mpd:/run/mpd
         ## Optional for myGPIOd support
         ## - /run/mygpiod:/run/mygpiod
         - /docker/mympd/workdir:/var/lib/mympd
         - /docker/mympd/cachedir:/var/cache/mympd
         - /var/lib/mpd/music:/var/lib/mpd/music:ro
         - /var/lib/mpd/playlists:/var/lib/mpd/playlists:ro
       restart: unless-stopped

Setup: ``docker-compose up -d``

Docker CLI
~~~~~~~~~~

.. code:: sh

   docker run -d \
     --name=mympd \
     -p 8080:8080 \
     -p 8443:8443 \
     -u 1000:1000 \
     -e "TZ=Europe/Berlin" \
     -e "MYMPD_SSL_SAN=DNS:<host fqdn>" \
     -e "MYMPD_MYMPD_URI=http://<host fqdn>:8080" \
     -v /run/mpd:/run/mpd \
     ## Optional for myGPIOd support
     ## -v /run/mygpiod:/run/mygpiod \
     -v /docker/mympd/workdir:/var/lib/mympd \
     -v /docker/mympd/cachedir:/var/cache/mympd \
     -v /var/lib/mpd/music:/var/lib/mpd/music:ro \
     -v /var/lib/mpd/playlists:/var/lib/mpd/playlists:ro \
     --restart unless-stopped \
     ghcr.io/jcorporation/mympd/mympd

Logs
~~~~

.. code:: sh

   docker logs -f mympd

myMPD configuration
-------------------

You can configure general options of myMPD via command line flags and environment variables.

- `Configuration <../020-configuration/index.rst>`__
