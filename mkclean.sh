#!/bin/sh
rm -rf release
rm -rf debug
rm -rf debian/tmp
rm -f debian/files

rm -f htdocs/library
rm -f htdocs/pics

rm -f htdocs/sw.min.js
rm -f htdocs/js/mympd.min.js
rm -f htdocs/js/player.min.js
rm -f htdocs/js/bootstrap-native-v4.min.js

rm -f htdocs/css/mympd.min.css
rm -f htdocs/css/bootstrap.min.css

find ./ -name \*~ -delete
