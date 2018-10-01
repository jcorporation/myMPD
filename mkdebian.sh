#!/bin/bash

VERSION=$(grep VERSION_ CMakeLists.txt | cut -d\" -f2 | tr '\n' '.')

cat > debian/changelog << EOL
mympd (${VERSION}-1) stable; urgency=medium

  * Release from master

 -- Juergen Mang <mail@jcgames.de>  Mon, 02 Oct 2018 00:04:00 +0200
EOL

./mkclean.sh
tar -czvf ../mympd_${VERSION}.orig.tar.gz *
dpkg-buildpackage -rfakeroot
