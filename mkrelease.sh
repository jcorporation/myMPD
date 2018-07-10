#/bin/sh

if [ -f buildtools/closure-compiler.jar ]
then
  [ htdocs/js/player.js -nt htdocs/js/player.min.js ] && \
    java -jar buildtools/closure-compiler.jar htdocs/js/player.js > htdocs/js/player.min.js
  [ htdocs/js/mpd.js -nt  htdocs/js/mpd.min.js ] && \
    java -jar buildtools/closure-compiler.jar htdocs/js/mpd.js > htdocs/js/mpd.min.js
else
  [ htdocs/js/player.js -nt htdocs/js/player.min.js ] && \
    cp htdocs/js/player.js htdocs/js/player.min.js
  [ htdocs/js/mpd.js -nt  htdocs/js/mpd.min.js ] && \
    cp htdocs/js/mpd.js  htdocs/js/mpd.min.js
fi
  
if [ -f buildtools/closure-stylesheets.jar ]
then    
  [ htdocs/css/mpd.css -nt htdocs/css/mpd.min.css ] && \
    java -jar buildtools/closure-stylesheets.jar htdocs/css/mpd.css > htdocs/css/mpd.min.css
else
  [ htdocs/css/mpd.css -nt htdocs/css/mpd.min.css ] && \
    cp htdocs/css/mpd.css htdocs/css/mpd.min.css    
fi

[ -d release ] || mkdir release
cd release
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr -DCMAKE_BUILD_TYPE=RELEASE ..
make
sudo make install
cd ..

sudo sed -i -e 's/mpd\.css/mpd\.min\.css/' -e 's/mpd\.js/mpd\.min\.js/' /usr/share/mympd/htdocs/index.html
sudo sed -i -e 's/mpd\.css/mpd\.min\.css/' -e 's/player\.js/player\.min\.js/' /usr/share/mympd/htdocs/player.html

sudo chown nobody /var/lib/mympd

[ -d /etc/systemd/system ] && \
  sudo cp -v contrib/mympd.service /etc/systemd/system/

[ -d /etc/default ] && \
  sudo cp -v contrib/mympd.default /etc/default/mympd

[ -d /etc/mympd/ssl ] || contrib/crcert.sh
  
echo "myMPD installed"
