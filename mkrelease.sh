#/bin/bash

if [ -f buildtools/closure-compiler.jar ]
then
  [ htdocs/js/player.js -nt htdocs/js/player.min.js ] && \
    java -jar buildtools/closure-compiler.jar htdocs/js/player.js > htdocs/js/player.min.js
  [ htdocs/js/mpd.js -nt  htdocs/js/mpd.min.js ] && \
    java -jar buildtools/closure-compiler.jar htdocs/js/mpd.js > htdocs/js/mpd.min.js
else
  cp htdocs/js/player.js htdocs/js/player.min.js
  cp htdocs/js/mpd.js  htdocs/js/mpd.min.js
fi
  
if [ -f buildtools/closure-stylesheets.jar ]
then    
  [ htdocs/css/mpd.css -nt htdocs/css/mpd.min.css ] && \
    java -jar buildtools/closure-stylesheets.jar htdocs/css/mpd.css > htdocs/css/mpd.min.css
else
  cp htdocs/css/mpd.css htdocs/css/mpd.min.css    
fi

[ -d release ] || mkdir release
cd release
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr -DCMAKE_BUILD_TYPE=RELEASE ..
make
make install
sed -i -e 's/mpd\.css/mpd\.min\.css/' -e 's/mpd\.js/mpd\.min\.js/' /usr/share/mympd/htdocs/index.html
sed -i -e 's/mpd\.css/mpd\.min\.css/' -e 's/player\.js/player\.min\.js/' /usr/share/mympd/htdocs/player.html
