#!/bin/sh

install -d src
install -d include/mpd/

rsync -av /data/src/libmpdclient/src/ ./src/
rsync -av /data/src/libmpdclient/include/mpd/ ./include/mpd/

sed -e 's/@MAJOR_VERSION@/2/' -e 's/@MINOR_VERSION@/17/' -e 's/@PATCH_VERSION@/0/' \
  /data/src/libmpdclient/include/mpd/version.h.in > include/mpd/version.h
  
cp /data/src/libmpdclient/output/config.h include/config.h

