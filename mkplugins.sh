#!/bin/sh
#
#small script to compile and install myMPD plugins
#

install -d plugins
cd plugins

g++ -shared -fPIC ../src/plugins/mympd_coverextract.cpp -ldl -o mympd_coverextract.so
gcc -Wall ../src/plugins/test_coverextract.c -o test_coverextract -ldl

sudo install -d /usr/share/mympd/lib
sudo install mympd_coverextract.so /usr/share/mympd/lib/mympd_coverextract.so

export LD_LIBRARY_PATH=/usr/share/mympd/lib/
