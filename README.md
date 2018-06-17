myMPD
====

myMPD is a lightweight MPD web client that runs without a dedicated webserver or interpreter. 
It's tuned for minimal resource usage and requires only very litte dependencies.
myMPD is a fork of ympd.

This fork provides a reworked ui based on Bootstrap 4 and a modernized backend.

![image](https://jcgames.de/stuff/myMPD/screenshots.gif)

UI Components
-------------
 - Bootstrap 4: https://getbootstrap.com/
 - Bootstrap Notify: http://bootstrap-notify.remabledesigns.com/
 - Bootstrap Slider: https://github.com/seiyria/bootstrap-slider
 - Material Design Icons: https://material.io/tools/icons/?style=baseline
 - jQuery: https://jquery.com/
 - js-cookie: https://github.com/js-cookie/js-cookie

Backend
-------
 - Mongoose: https://github.com/cesanta/mongoose
 - Frozen: https://github.com/cesanta/frozen

Dependencies
------------
 - libmpdclient 2: http://www.musicpd.org/libs/libmpdclient/
 - cmake 2.6: http://cmake.org/

Unix Build Instructions
-----------------------

1. install dependencies. cmake and libmpdclient (dev) are available from all major distributions.
2. create build directory ```cd /path/to/src; mkdir build; cd build```
3. create makefile ```cmake ..  -DCMAKE_INSTALL_PREFIX:PATH=/usr -DCMAKE_BUILD_TYPE=RELEASE```
4. build ```make```
5. install ```sudo make install```
6. Link your mpd music directory to ```/usr/share/mympd/htdocs/library``` and put ```folder.jpg``` files in your album directories
7. Configure your mpd with http stream output to use the local player

Run flags
---------
```
Usage: ./mympd [OPTION]...

 -h, --host <host>             connect to mpd at host [localhost]
 -p, --port <port>             connect to mpd at port [6600]
 -w, --webport [ip:]<port>     listen interface/port for webserver [8080]
 -s, --streamport <port>       connect to mpd http stream at port [8000]
 -u, --user <username>         drop priviliges to user after socket bind
 -m, --mpdpass <password>      specifies the password to use when connecting to mpd
 -i, --coverimage <filename>   filename for coverimage [folder.jpg]
 -v, --version                 get version
 --help                        this help
```

Copyright
---------
ympd: 2013-2014 <andy@ndyk.de>

myMPD: 2018 <mail@jcgames.de>
