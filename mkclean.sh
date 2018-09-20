#!/bin/sh
rm -rf release
rm -rf debug
rm -rf debian/tmp
rm -f debian/files
rm -f htdocs/js/bootstrap-native-v4.min.js
rm -f htdocs/css/bootstrap.min.css
rm -f htdocs/library
rm -f htdocs/pics
find ./ -name \*~ -delete
