#/bin/sh

cp dist/htdocs/js/bootstrap-native-v4.min.js htdocs/js/
cp dist/htdocs/css/bootstrap.min.css htdocs/css/

[ -d debug ] || mkdir debug
cd debug
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr -DCMAKE_BUILD_TYPE=DEBUG ..
make
