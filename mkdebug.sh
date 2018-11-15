#/bin/sh
[ -e $PWD/htdocs/sw.min.js ] || ln -s $PWD/htdocs/sw.js $PWD/htdocs/sw.min.js
[ -e $PWD/htdocs/js/mympd.min.js ] || ln -s $PWD/htdocs/js/mympd.js $PWD/htdocs/js/mympd.min.js
[ -e $PWD/htdocs/js/player.min.js ] || ln -s $PWD/htdocs/js/player.js $PWD/htdocs/js/player.min.js
[ -e $PWD/htdocs/js/bootstrap-native-v4.min.js ] || ln -s $PWD/dist/htdocs/js/bootstrap-native-v4.js $PWD/htdocs/js/bootstrap-native-v4.min.js
[ -e $PWD/htdocs/js/keymap.min.js ] || ln -s $PWD/htdocs/js/keymap.js $PWD/htdocs/js/keymap.min.js

[ -e $PWD/htdocs/css/mympd.min.css ] || ln -s $PWD/htdocs/css/mympd.css $PWD/htdocs/css/mympd.min.css
[ -e $PWD/htdocs/css/bootstrap.min.css ] || ln -s $PWD/dist/htdocs/css/bootstrap.min.css $PWD/htdocs/css/bootstrap.min.css

echo "Trying to link musicdir to library"
if [ -f /etc/mpd.conf ]
then
  LIBRARY=$(grep ^music_directory /etc/mpd.conf | awk {'print $2'} | sed -e 's/"//g')
  [ "$LIBRARY" != "" ] && [ ! -e htdocs/library ] && ln -s "$LIBRARY" htdocs/library
else
  echo "/etc/mpd.conf not found, you must link your music_directory manually to htdocs/library"
fi

echo "Linking pics directory"
[ -e $PWD/htdocs/pics ] || ln -s /var/lib/mympd/pics htdocs/

[ -d debug ] || mkdir debug
cd debug
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr -DCMAKE_BUILD_TYPE=DEBUG ..
make
