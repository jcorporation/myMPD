#!/bin/bash
./mkclean.sh
tar -czvf ../mympd_4.1.0.orig.tar.gz *
dpkg-buildpackage -rfakeroot
