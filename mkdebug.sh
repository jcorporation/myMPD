#!/bin/sh
[ -e "$PWD/htdocs/sw.min.js" ] || ln -s "$PWD/htdocs/sw.js" "$PWD/htdocs/sw.min.js"
[ -e "$PWD/htdocs/js/mympd.min.js" ] || ln -s "$PWD/htdocs/js/mympd.js" "$PWD/htdocs/js/mympd.min.js"
[ -e "$PWD/htdocs/js/bootstrap-native-v4.min.js" ] || ln -s "$PWD/dist/htdocs/js/bootstrap-native-v4.js" "$PWD/htdocs/js/bootstrap-native-v4.min.js"
[ -e "$PWD/htdocs/js/keymap.min.js" ] || ln -s "$PWD/htdocs/js/keymap.js" "$PWD/htdocs/js/keymap.min.js"
[ -e "$PWD/htdocs/js/i18n.min.js" ] || ln -s "$PWD/dist/htdocs/js/i18n.min.js" "$PWD/htdocs/js/i18n.min.js"
[ -e "$PWD/htdocs/css/mympd.min.css" ] || ln -s "$PWD/htdocs/css/mympd.css" "$PWD/htdocs/css/mympd.min.css"
[ -e "$PWD/htdocs/css/bootstrap.min.css" ] || ln -s "$PWD/dist/htdocs/css/bootstrap.min.css" "$PWD/htdocs/css/bootstrap.min.css"

install -d debug
cd debug || exit 1
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr -DCMAKE_BUILD_TYPE=DEBUG ..
make VERBOSE=1

cd ../src/i18n || exit 1
./tojson.pl pretty > ../../dist/htdocs/js/i18n.min.js
