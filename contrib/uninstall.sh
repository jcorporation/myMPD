#!/bin/sh
rm -r /etc/mympd
rm -r /var/lib/mympd
rm -r /usr/share/mympd
rm -f /usr/lib/systemd/system/mympd.service

userdel mympd
groupdel mympd
