#/bin/bash

rm htdocs/js/player.min.js
cp -l htdocs/js/player.js htdocs/js/player.min.js

rm htdocs/js/mpd.min.js
cp -l htdocs/js/mpd.js htdocs/js/mpd.min.js

rm htdocs/css/mpd.min.css
cp -l htdocs/css/mpd.css htdocs/css/mpd.min.css    

[ -d debug ] || mkdir debug
cd debug
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr -DCMAKE_BUILD_TYPE=DEBUG ..
make
