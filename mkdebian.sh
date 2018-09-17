#!/bin/bash
./mkclean.sh
tar -czvf ../mympd_4.2.0.orig.tar.gz *
dpkg-buildpackage -rfakeroot
