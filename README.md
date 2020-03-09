myMPD
=====

myMPD is a lightweight MPD web client that runs without a dedicated webserver or interpreter. 
It's tuned for minimal resource usage and requires only very few dependencies.

myMPD is a fork of ympd (https://github.com/notandy/ympd).
This fork provides a reworked ui based on Bootstrap 4, a modernized backend and many new features while having the same small footprint as ympd.

- <a href="https://jcorporation.github.io/myMPD/">Homepage</a>

**Design principles**
 - Keep it small and simple
 - Uses only mpd as source of truth
 - Mobile first UI
 - Keep security in mind

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
 - Local lyrics (textfile per song file)
 - Local booklet support (per album folder)
 - HTTP stream support
 - Local playback of mpd http stream (html5 audio api)
 - Timers
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
 - Bootstrap 4: https://getbootstrap.com/
 - Material Design Icons: https://material.io/tools/icons/
 - Bootstrap Native: http://thednp.github.io/bootstrap.native/

Backend
-------
 - Mongoose: https://github.com/cesanta/mongoose (web server)
 - Frozen: https://github.com/cesanta/frozen (json parsing)
 - inih: https://github.com/benhoyt/inih (config file parsing)
 - incbin: https://github.com/graphitemaster/incbin (embedding assets)
 - sds: https://github.com/antirez/sds (safe string handling)
 - rax: https://github.com/antirez/rax (radix tree implementation)
 - libmpdclient2: https://github.com/jcorporation/libmpdclient/tree/libmympdclient

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

ympd: 2013-2014 <andy@ndyk.de>
