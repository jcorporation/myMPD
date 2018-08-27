#/bin/sh

java=$(which java 2> /dev/null)

if [ -f dist/buildtools/closure-compiler.jar ] && [ "$java" != "" ]
then
  echo "Minifying javascript"
  [ htdocs/js/player.js -nt dist/htdocs/js/player.min.js ] && \
    java -jar dist/buildtools/closure-compiler.jar htdocs/js/player.js > dist/htdocs/js/player.min.js
  [ htdocs/js/mympd.js -nt  dist/htdocs/js/mympd.min.js ] && \
    java -jar dist/buildtools/closure-compiler.jar htdocs/js/mympd.js > dist/htdocs/js/mympd.min.js
  [ htdocs/sw.js -nt dist/htdocs/sw.min.js ] && \
    java -jar dist/buildtools/closure-compiler.jar htdocs/sw.js > dist/htdocs/sw.min.js
else
  echo "dist/buildtools/closure-compiler.jar not found, using non-minified files"
  [ htdocs/js/player.js -nt dist/htdocs/js/player.min.js ] && \
    cp htdocs/js/player.js dist/htdocs/js/player.min.js
  [ htdocs/js/mympd.js -nt  dist/htdocs/js/mympd.min.js ] && \
    cp htdocs/js/mympd.js  dist/htdocs/js/mympd.min.js
  [ htdocs/sw.js -nt dist/htdocs/sw.min.js ] && \
    cp htdocs/sw.js dist/htdocs/sw.min.js    
fi

if [ -f dist/buildtools/closure-stylesheets.jar ] && [ "$java" != "" ]
then
  echo "Minifying stylesheets"
  [ htdocs/css/mympd.css -nt dist/htdocs/css/mympd.min.css ] && \
    java -jar dist/buildtools/closure-stylesheets.jar --allow-unrecognized-properties htdocs/css/mympd.css > dist/htdocs/css/mympd.min.css
else
  echo "dist/buildtools/closure-stylesheets.jar not found, using non-minified files"
  [ htdocs/css/mympd.css -nt dist/htdocs/css/mympd.min.css ] && \
    cp htdocs/css/mympd.css dist/htdocs/css/mympd.min.css    
fi

echo "Replacing javascript and stylesheets with minified files"
sed -e 's/mympd\.css/mympd\.min\.css/' -e 's/mympd\.js/mympd\.min\.js/' htdocs/index.html > dist/htdocs/index.html
sed -e 's/mympd\.css/mympd\.min\.css/' -e 's/player\.js/player\.min\.js/' htdocs/player.html > dist/htdocs/player.html
sed -i -e 's/mympd\.css/mympd\.min\.css/' -e 's/mympd\.js/mympd\.min\.js/' -e 's/player\.js/player\.min\.js/' dist/htdocs/sw.min.js
sed -i -e 's/\/sw\.js/\/sw\.min\.js/' dist/htdocs/js/mympd.min.js
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
  LIBRARY=$(grep ^music_directory /etc/mpd.conf | awk {'print $2'} | sed -e 's/"//g')
  [ "$LIBRARY" != "" ] && [ ! -e /usr/share/mympd/htdocs/library ] && sudo ln -s "$LIBRARY" /usr/share/mympd/htdocs/library
else
  echo "/etc/mpd.conf not found, you must link your music_directory manually to /usr/share/mympd/htdocs/library"
fi

echo "Creating dir for cover pictures"
[ -d /usr/share/mympd/htdocs/pics ] || mkdir /usr/share/mympd/htdocs/pics

echo "Installing systemd service"
if [ -d /etc/systemd/system ]
then
  if [ contrib/mympd.service -nt /etc/systemd/system/mympd.service ]
  then
    sudo cp contrib/mympd.service /etc/systemd/system/
    sudo systemctl daemon-reload
  fi
  sudo systemctl enable mympd  
fi

if [ -f /etc/mympd/mympd.conf ]
then
  echo "/etc/mympd/mympd.conf already exists"
  cp contrib/mympd.conf /etc/mympd/mympd.conf.dist
else
  [ -d /etc/mympd ] || mkdir /etc/mympd
  cp contrib/mympd.conf /etc/mympd/mympd.conf
fi

if [ -d /etc/mympd/ssl ]
then
  echo "Certificates already created"
else
  echo "Creating certificates"
  contrib/crcert.sh
fi

echo "myMPD installed"
echo "Edit /etc/mympd/mympd.conf before starting myMPD"
