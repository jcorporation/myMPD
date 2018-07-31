#/bin/sh

java=$(which java 2> /dev/null)

if [ -f dist/buildtools/closure-compiler.jar ] && [ "$java" != "" ]
then
  echo "Minifying javascript"
  [ htdocs/js/player.js -nt dist/htdocs/js/player.min.js ] && \
    java -jar dist/buildtools/closure-compiler.jar htdocs/js/player.js > dist/htdocs/js/player.min.js
  [ htdocs/js/mpd.js -nt  dist/htdocs/js/mpd.min.js ] && \
    java -jar dist/buildtools/closure-compiler.jar htdocs/js/mpd.js > dist/htdocs/js/mpd.min.js
  [ htdocs/sw.js -nt dist/htdocs/sw.min.js ] && \
    java -jar dist/buildtools/closure-compiler.jar htdocs/sw.js > dist/htdocs/sw.min.js
else
  echo "buildtools/closure-compiler.jar not found, using non-minified files"
  [ htdocs/js/player.js -nt dist/htdocs/js/player.min.js ] && \
    cp htdocs/js/player.js dist/htdocs/js/player.min.js
  [ htdocs/js/mpd.js -nt  dist/htdocs/js/mpd.min.js ] && \
    cp htdocs/js/mpd.js  dist/htdocs/js/mpd.min.js
  [ htdocs/sw.js -nt dist/htdocs/sw.min.js ] && \
    cp htdocs/sw.js dist/htdocs/sw.min.js    
fi

if [ -f dist/buildtools/closure-stylesheets.jar ] && [ "$java" != "" ]
then
  echo "Minifying stylesheets"
  [ htdocs/css/mpd.css -nt dist/htdocs/css/mpd.min.css ] && \
    java -jar dist/buildtools/closure-stylesheets.jar htdocs/css/mpd.css > dist/htdocs/css/mpd.min.css
else
  echo "buildtools/closure-stylesheets.jar not found, using non-minified files"
  [ htdocs/css/mpd.css -nt dist/htdocs/css/mpd.min.css ] && \
    cp htdocs/css/mpd.css dist/htdocs/css/mpd.min.css    
fi

echo "Replacing javascript and stylesheets with minified files"
sed -e 's/mpd\.css/mpd\.min\.css/' -e 's/mpd\.js/mpd\.min\.js/' htdocs/index.html > dist/htdocs/index.html
sed -e 's/mpd\.css/mpd\.min\.css/' -e 's/player\.js/player\.min\.js/' htdocs/player.html > dist/htdocs/player.html
sed -i -e 's/mpd\.css/mpd\.min\.css/' -e 's/mpd\.js/mpd\.min\.js/' -e 's/player\.js/player\.min\.js/' dist/htdocs/sw.min.js
sed -i -e 's/\/sw\.js/\/sw\.min\.js/' dist/htdocs/js/mpd.min.js
echo "Minifying html"
perl -i -pe 's/^\s*//gm; s/\s*$//gm' dist/htdocs/index.html
perl -i -pe 's/^\s*//gm; s/\s*$//gm' dist/htdocs/player.html

echo "Compiling and installing mympd"
[ -d release ] || mkdir release
cd release
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr -DCMAKE_BUILD_TYPE=RELEASE ..
make
sudo make install
cd ..

echo "Fixing ownership of /var/lib/mympd"
sudo chown nobody /var/lib/mympd

echo "Trying to link musicdir to library"
if [ -f /etc/mpd.conf ]
then
  LIBRARY=$(grep music /etc/mpd.conf | awk {'print $2'} | sed -e 's/"//g')
  [ "$LIBRARY" != "" ] && [ ! -e /usr/share/mympd/htdocs/library ] && ln -s "$LIBRARY" /usr/share/mympd/htdocs/library
else
  echo "/etc/mpd.conf not found, you must link your musicdir manually to /usr/share/mympd/htdocs/library"
fi

echo "Installing systemd service"
if [ -d /etc/systemd/system ]
then
  if [ contrib/mympd.service -nt /etc/systemd/system/mympd.service ]
  then
    sudo cp -v contrib/mympd.service /etc/systemd/system/
    systemctl daemon-reload
  fi
  systemctl enable mympd  
fi

if [ -d /etc/mympd/ssl ]
then
  echo "Certificates already created"
else
  echo "Creating certificates"
  contrib/crcert.sh
fi
  
echo "myMPD installed"
echo "Edit /etc/mympd/options before starting mympd"
