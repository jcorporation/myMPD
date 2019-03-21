#!/bin/sh
#
#small script to compile and install myMPD plugins
#

install -d plugins
cd plugins

INSTALL_PREFIX="${MYMPD_INSTALL_PREFIX:-/usr}"
cmake -DCMAKE_INSTALL_PREFIX:PATH=$INSTALL_PREFIX -DCMAKE_BUILD_TYPE=RELEASE ../src/plugins
make
sudo make install

export LD_LIBRARY_PATH=/usr/share/mympd/lib/
