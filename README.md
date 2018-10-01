myMPD
=====

myMPD is a lightweight MPD web client that runs without a dedicated webserver or interpreter. 
It's tuned for minimal resource usage and requires only very litte dependencies.
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
 - Browse filesystem and playlists
 - Queue management
 - Playlist management
 - Advanced search
 - Jukebox mode (add's random songs / albums from database or playlists to queue)
 - Smart playlists and saved searches
 - Play statistics and song voting (uses mpd stickers)
 - Local coverart support (Albums and Streams)
 - HTTP stream support
 - Local playback of mpd http stream (html5 audio api)
 - Progressiv Web App enabled
 - Embedded Webserver (mongoose)

myMPD is work in progress. Bugreportes and feature requests are very welcome.
 - https://github.com/jcorporation/myMPD/issues

Screenshots
-----------

![image](https://jcgames.de/stuff/myMPD/screenshots_2018-08-27.gif)

UI Components
-------------
 - Bootstrap 4: https://getbootstrap.com/
 - Material Design Icons: https://material.io/tools/icons/
 - Bootstrap Native: http://thednp.github.io/bootstrap.native/

Backend
-------
 - Mongoose: https://github.com/cesanta/mongoose
 - Frozen: https://github.com/cesanta/frozen
 - inih: https://github.com/benhoyt/inih

Dependencies
------------
 - libmpdclient 2: http://www.musicpd.org/libs/libmpdclient/
 - OpenSSL: https://www.openssl.org/

Build Dependencies
------------------
 - cmake 2.6
 - libasan3: for debug builds only
 - Java: optional for minifying .css and .js files

Unix Build Instructions
-----------------------

1. Install dependencies: cmake, libmpdclient (dev), OpenSSL (dev) and Java are available from all major distributions.
2. Build and install: ```cd /path/to/src; ./mkrelease.sh```.
3. Link your mpd music directory to ```/usr/share/mympd/htdocs/library``` and put ```folder.jpg``` files in your album directories (mkrelease.sh tries to do this for you).
4. Configure your mpd with http stream output to use the local player.

Run Flags
---------
```
Usage: ./mympd /etc/mypd/mympd.conf
```

SSL
---

1. Create ca and certificate ```/usr/share/mympd/crcert.sh``` (mkrelease.sh does this for you).
2. Set ```ssl=true``` in /etc/mympd/mympd.conf (use default certificate under ```/etc/mympd/ssl/```).
3. Import ```/etc/mympd/ssl/ca/ca.pem``` in your browser to trust the certificate.
4. myMPD redirects http requests to https, ensure that myMPD hostname is resolvable.

Copyright
---------
ympd: 2013-2014 <andy@ndyk.de>

myMPD: 2018 <mail@jcgames.de>
