#!/bin/bash

VERSION=$(grep VERSION_ CMakeLists.txt | cut -d\" -f2 | tr '\n' '.' | sed 's/\.$//')

export LC_TIME="en_GB.UTF-8"
cat > debian/changelog << EOL
mympd (${VERSION}-1) stable; urgency=medium

  * Release from master

 -- Juergen Mang <mail@jcgames.de>  $(date +"%a, %d %b %Y %H:%m:%S %z")
EOL

./mkclean.sh
tar -czvf ../mympd_${VERSION}.orig.tar.gz *
dpkg-buildpackage -rfakeroot
