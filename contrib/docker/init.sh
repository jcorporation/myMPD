#!/bin/sh

/postinst
sed -i "s#mpdhost = 127.0.0.1#mpdhost = $MYMPD_MPDHOST#g" /etc/mympd/mympd.conf
sed -i "s#mpdport = 6600#mpdport = $MYMPD_MPDPORT#g" /etc/mympd/mympd.conf
sed -i "s#ssl = true#ssl = $MYMPD_SSL#g" /etc/mympd/mympd.conf
sed -i "s#coverimagename = folder.jpg#coverimagename = $MYMPD_COVERIMAGENAME#g" /etc/mympd/mympd.conf
sed -i "s#loglevel = 1#loglevel = $MYMPD_LOGLEVEL#g" /etc/mympd/mympd.conf
mympd /etc/mympd/mympd.conf


