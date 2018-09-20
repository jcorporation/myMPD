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

echo "Minifying html"
[ htdocs/index.html -nt dist/htdocs/index.html ] && \
  perl -pe 's/^\s*//gm; s/\s*$//gm' htdocs/index.html > dist/htdocs/index.html
[ htdocs/player.html -nt dist/htdocs/player.html ] && \
  perl -pe 's/^\s*//gm; s/\s*$//gm' htdocs/player.html > dist/htdocs/player.html

echo "Compiling and installing mympd"
[ -d release ] || mkdir release
cd release
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr -DCMAKE_BUILD_TYPE=RELEASE ..
make
sudo make install
cd ..

debian/postinst
