myMPD
=====

myMPD is a lightweight MPD web client that runs without a dedicated webserver or interpreter. 
It's tuned for minimal resource usage and requires only very few dependencies.

myMPD is a fork of ympd (https://github.com/notandy/ympd).
This fork provides a reworked ui based on Bootstrap 4, a modernized backend and many new features while having the same small footprint as ympd.

**Design principles:**
 - Keep it small and simple
 - Uses only mpd as source of truth
 - Mobile first UI
 - Keep security in mind

**Featurelist:**
 - Control mpd functions (play, pause, etc.)
 - Set mpd settings (repeat, random, etc.)
 - Browse mpd database by tags
 - Albumart grid
 - Browse filesystem and playlists
 - Bookmarks for directories
 - Queue management
 - Playlist management
 - Advanced search
 - Jukebox mode (add's random songs / albums from database or playlists to queue)
 - Smart playlists and saved searches
 - Play statistics and song voting
 - Local albumart support: embedded and image per folder
 - HTTP stream support
 - Local playback of mpd http stream (html5 audio api)
 - Progressiv Web App enabled
 - Embedded Webserver (mongoose)
 - Love message for scrobbling clients
 - Localized user interface
 - Themeing

myMPD is work in progress. Feedback, bug reportes and feature requests are very welcome.
 - https://github.com/jcorporation/myMPD/issues

Screenshots
-----------

![image](https://jcgames.de/stuff/myMPD/screenshots-2019-02-23.gif)

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
 - libmpdclient2: https://github.com/jcorporation/libmpdclient/tree/libmympdclient

Dependencies
------------
 - OpenSSL (optional)
 - libid3tag (optional)
 - libflac (optional)

Build Dependencies
------------------
 - cmake 2.6
 - libasan3: for debug builds only
 - Java: to minify files
 - Perl: to create translation files

Unix Build Instructions
-----------------------
1. Get myMPD tarball from https://github.com/jcorporation/myMPD/releases/latest
2. Extract myMPD tarball and change path to this directory
3. Install dependencies (as root): ``./build.sh installdeps``
4. Build myMPD: ``./build.sh release``
5. Install myMPD (as root): ``./build.sh install``

Run
---
Adapt the configuration file ``/etc/mympd.conf`` to your needs.
``
Usage: ./mympd [/etc/mympd.conf]
``
The ``./build.sh`` script installs a startup script for systemd, openrc (Alpine Linux) or sysVinit.

Copyright
---------

myMPD: 2018-2019 <mail@jcgames.de>

ympd: 2013-2014 <andy@ndyk.de>
