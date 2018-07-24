#/bin/sh

if [ -f buildtools/closure-compiler.jar ]
then
  echo "Minifying javascript"
  [ htdocs/js/player.js -nt htdocs/js/player.min.js ] && \
    java -jar buildtools/closure-compiler.jar htdocs/js/player.js > htdocs/js/player.min.js
  [ htdocs/js/mpd.js -nt  htdocs/js/mpd.min.js ] && \
    java -jar buildtools/closure-compiler.jar htdocs/js/mpd.js > htdocs/js/mpd.min.js
  [ htdocs/sw.js -nt htdocs/sw.min.js ] && \
    java -jar buildtools/closure-compiler.jar htdocs/sw.js > htdocs/sw.min.js
else
  echo "buildtools/closure-compiler.jar not found, using non-minified files"
  [ htdocs/js/player.js -nt htdocs/js/player.min.js ] && \
    cp htdocs/js/player.js htdocs/js/player.min.js
  [ htdocs/js/mpd.js -nt  htdocs/js/mpd.min.js ] && \
    cp htdocs/js/mpd.js  htdocs/js/mpd.min.js
  [ htdocs/sw.js -nt htdocs/sw.min.js ] && \
    cp htdocs/sw.js htdocs/sw.min.js    
fi

if [ -f buildtools/closure-stylesheets.jar ]
then
  echo "Minifying stylesheets"
  [ htdocs/css/mpd.css -nt htdocs/css/mpd.min.css ] && \
    java -jar buildtools/closure-stylesheets.jar htdocs/css/mpd.css > htdocs/css/mpd.min.css
else
  echo "buildtools/closure-stylesheets.jar not found, using non-minified files"
  [ htdocs/css/mpd.css -nt htdocs/css/mpd.min.css ] && \
    cp htdocs/css/mpd.css htdocs/css/mpd.min.css    
fi

echo "Compiling and installing mympd"
[ -d release ] || mkdir release
cd release
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr -DCMAKE_BUILD_TYPE=RELEASE ..
make
sudo make install
cd ..

echo "Replacing javascript and stylesheets with minified files"
sudo sed -i -e 's/mpd\.css/mpd\.min\.css/' -e 's/mpd\.js/mpd\.min\.js/' /usr/share/mympd/htdocs/index.html
sudo sed -i -e 's/mpd\.css/mpd\.min\.css/' -e 's/player\.js/player\.min\.js/' /usr/share/mympd/htdocs/player.html
sudo sed -i -e 's/mpd\.css/mpd\.min\.css/' -e 's/mpd\.js/mpd\.min\.js/' -e 's/player\.js/player\.min\.js/' /usr/share/mympd/htdocs/sw.min.js
sudo sed -i -e 's/\/sw\.js/\/sw\.min\.js/' /usr/share/mympd/htdocs/js/mpd.min.js
echo "Minifying html"
sudo perl -i -pe 's/^\s*//gm; s/\s*$//gm' /usr/share/mympd/htdocs/index.html
sudo perl -i -pe 's/^\s*//gm; s/\s*$//gm' /usr/share/mympd/htdocs/player.html

echo "Fixing ownership of /var/lib/mympd"
sudo chown nobody /var/lib/mympd

echo "Trying to link musicdir to library"
if [ -f /etc/mpd.conf ]
then
  LIBRARY=$(grep music /etc/mpd.conf | awk {'print $2'})
  [ "$LIBRARY" != "" ] && [ ! -e /usr/share/mympd/htdocs/library ] && ln -s $LIBRARY /usr/share/mympd/htdocs/library
else
  echo "/etc/mpd.conf not found, you must link your musicdir manually to /usr/share/mympd/htdocs/library"
fi

echo "Installing systemd service"
if [ -d /etc/systemd/system ]
then
  if [ contrib/mympd.service -nt /etc/systemd/system/mympd.service ]
  then
    sudo cp -v contrib/mympd.service /etc/systemd/system/
    sudo systemctl daemon-reload
  fi
  sudo systemctl enable mympd  
fi

if [ -d /etc/mympd/ssl ]
then
  echo "Certificates already created"
else
  echo "Creating certificates"
  sudo contrib/crcert.sh
fi
  
echo "myMPD installed"
echo "Edit /etc/mympd/options before starting mympd"
