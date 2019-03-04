#!/bin/sh

g++ -shared -fPIC mympd_coverextract.cpp -ldl -o mympd_coverextract.so
gcc -Wall test_coverextract.c -o test_coverextract mympd_coverextract.so

sudo install -d /usr/share/mympd/lib
sudo install mympd_coverextract.so /usr/share/mympd/lib/mympd_coverextract.so

export LD_LIBRARY_PATH=/usr/share/mympd/lib/
