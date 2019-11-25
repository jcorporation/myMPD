#!/bin/sh

STARTDIR=$(pwd)

TMPDIR=$(mktemp -d)
cd $TMPDIR || exit 1
git clone -b libmympdclient https://github.com/jcorporation/libmpdclient.git
cd libmpdclient || exit 1
meson . output

cd $STARTDIR || exit 1
install -d src
install -d include/mpd/

rsync -av $TMPDIR/libmpdclient/src/ ./src/
rsync -av $TMPDIR/libmpdclient/include/mpd/ ./include/mpd/

rsync -av $TMPDIR/libmpdclient/output/version.h include/mpd/version.h
rsync -av $TMPDIR/libmpdclient/output/config.h include/config.h

rsync -av $TMPDIR/libmpdclient/COPYING COPYING
rsync -av $TMPDIR/libmpdclient/AUTHORS AUTHORS

rm -r $TMPDIR
