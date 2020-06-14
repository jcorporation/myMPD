myMPD
=====

myMPD is a standalone and lightweight MPD web client. It's tuned for minimal resource usage and requires only very few dependencies. Therefore myMPD is ideal for raspberry pis and similar devices.

The backend ist written in C and has no dependencies to external databases or webservers. The configuration is stored in plain text files and all the data is pulled on demand from MPD. The MPD database is the only source of truth for myMPD.

The frontend is mobile friendly, written as a PWA and offers on all devices the same functionality. It communicates over AJAX and websockets using the json-rpc 2 protocol. 

myMPD also integrates extended features like an advanced jukebox mode, timers and smart playlists. With the integrated lua interpreter myMPD functions can also be scripted. 

myMPD is a fork of ympd (https://github.com/notandy/ympd).

- <a href="https://jcorporation.github.io/myMPD/">Homepage</a>

**Features**
 - Control mpd functions (play, pause, etc.)
 - Set mpd options (repeat, random, etc.)
 - MPD mount and neighbors support
 - Browse mpd database by tags
 - Albumart grid
 - Browse filesystem
 - Queue management
 - Playlist management
 - Advanced search
 - Jukebox mode (add's random songs to queue)
 - Smart playlists and saved searches
 - Play statistics and song voting
 - Local albumart support: embedded and image per folder
 - Local lyrics (textfile per song or embedded)
 - Local booklet support (per album folder)
 - HTTP stream support
 - Local playback of mpd http stream (html5 audio api)
 - Timers
 - Lua scripting
 - Scrobbler integration
 - Embedded Webserver (mongoose)
 - Localized user interface
 - Themeing
 - Publishing of mpd and myMPD directories via http and webdav
 - Progressiv Web App enabled
 - Support of Media Session API

myMPD is in active development. If you like myMPD, you can help to improve it (no programming skills are required).
 - <a href="https://github.com/jcorporation/myMPD/issues/167">Help to improve myMPD</a>

To use all myMPD functions you should use the latest stable MPD version (0.21.x). myMPD already supports function of MPD 0.22.x (e.g. the readpicture command).

Screenshots
-----------

![image](https://jcorporation.github.io/myMPD/assets/myMDPv6.0.0.gif)

UI Components
-------------
 - <a href="https://getbootstrap.com/">Bootstrap 4</a>
 - <a href="https://material.io/tools/icons/">Material Design Icons</a>
 - <a href="http://thednp.github.io/bootstrap.native/">Bootstrap Native</a>

Backend
-------
 - <a href="https://github.com/cesanta/mongoose">Mongoose</a> (web server)
 - <a href="https://github.com/cesanta/frozen">Frozen</a> (json parsing)
 - <a href="https://github.com/benhoyt/inih">inih</a> (config file parsing)
 - <a href="https://github.com/graphitemaster/incbin">incbin</a> (embedding assets)
 - <a href="https://github.com/antirez/sds">sds</a> (safe string handling)
 - <a href="https://github.com/antirez/rax">rax</a> (radix tree implementation)
 - <a href="https://github.com/MersenneTwister-Lab/TinyMT">TinyMT</a> (prng)
 - <a href="https://github.com/jcorporation/libmpdclient/tree/libmympdclient">libmpdclient2</a> (mpd communication)

Dependencies
------------
 - OpenSSL >= 1.1.0 (optional): for https support
 - libid3tag (optional): to extract embedded albumart
 - libflac (optional): to extract embedded albumart

Build Dependencies
------------------
 - cmake 2.6
 - libasan3: for debug builds only
 - Java: to minify files
 - Perl: to create translation files

Quick Build Instructions
-----------------------
 1. Get myMPD tarball from https://github.com/jcorporation/myMPD/releases/latest
 2. Extract myMPD tarball and change path to this directory
 3. Install dependencies (as root): ``./build.sh installdeps``
 4. Build myMPD: ``./build.sh release``
 5. Install myMPD (as root): ``./build.sh install``

Run
---
Adapt the configuration file ``/etc/mympd.conf`` to your needs (``/etc/webapps/mympd/mympd.conf`` for Archlinux) or use the <a href="https://github.com/jcorporation/myMPD/wiki/mympd-config">mympd-config</a> tool to generate automatically a valid mympd.conf

``
Usage: ./mympd [/etc/mympd.conf]
``

The ``./build.sh`` script installs a startup script for systemd, openrc (Alpine Linux) or sysVinit.

Wiki
----
For further information on installation and configuration, see the myMPD wiki: https://github.com/jcorporation/myMPD/wiki

Copyright
---------
myMPD: 2018-2020 <mail@jcgames.de>
