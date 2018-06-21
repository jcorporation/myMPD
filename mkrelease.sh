#/bin/bash

if [ -f buildtools/closure-compiler.jar ]
then
  rm -f htdocs/js/player.min.js
  java -jar buildtools/closure-compiler.jar htdocs/js/player.js > htdocs/js/player.min.js

  rm -f htdocs/js/mpd.min.js
  java -jar buildtools/closure-compiler.jar htdocs/js/mpd.js > htdocs/js/mpd.min.js
fi
  
if [ -f buildtools/closure-stylesheets.jar ]
then    
  rm -f htdocs/css/mpd.min.css
  java -jar buildtools/closure-stylesheets.jar htdocs/css/mpd.css > htdocs/css/mpd.min.css
fi

[ -d release ] || mkdir release
cd release
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr -DCMAKE_BUILD_TYPE=RELEASE ..
make
make install
