#/bin/sh

cp dist/htdocs/js/bootstrap-native-v4.js htdocs/js/bootstrap-native-v4.min.js
cp dist/htdocs/css/bootstrap.min.css htdocs/css/

echo "Trying to link musicdir to library"
if [ -f /etc/mpd.conf ]
then
  LIBRARY=$(grep ^music_directory /etc/mpd.conf | awk {'print $2'} | sed -e 's/"//g')
  [ "$LIBRARY" != "" ] && [ ! -e htdocs/library ] && ln -s "$LIBRARY" htdocs/library
else
  echo "/etc/mpd.conf not found, you must link your music_directory manually to htdocs/library"
fi

echo "Linking pics directory"
[ -e htdocs/pics ] || ln -s /var/lib/mympd/pics htdocs/

[ -d debug ] || mkdir debug
cd debug
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr -DCMAKE_BUILD_TYPE=DEBUG ..
make
