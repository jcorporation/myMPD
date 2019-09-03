#!/bin/sh

VERSION=$(grep VERSION_ CMakeLists.txt | cut -d\" -f2 | tr '\n' '.' | sed 's/\.$//')

newer() {
  M1=0
  M2=0
  [ -f "$1" ] && M1=$(stat -c%Y "$1")
  [ -f "$2" ] && M2=$(stat -c%Y "$2")
  if [ "$M1" -gt "$M2" ]
  then
    return 0
  else
    return 1
  fi
}

minify() {
  TYPE="$1"
  SRC="$2"
  DST="$3"
  ERROR="1"
  
  JAVABIN=$(command -v java)

  if newer "$DST" "$SRC"
  then
    echo "Skipping $SRC"
    return
  fi
  echo "Minifying $SRC"

  if [ "$TYPE" = "html" ]
  then
    perl -pe 's/^\s*//gm; s/\s*$//gm' "$SRC" > "$DST"
    ERROR="$?"
  elif [ "$TYPE" = "js" ] && [ "$JAVABIN" != "" ]
  then
    $JAVABIN -jar dist/buildtools/closure-compiler.jar "$SRC" > "$DST"
    ERROR="$?"
  elif [ "$TYPE" = "css" ] && [ "$JAVABIN" != "" ]
  then
    $JAVABIN -jar dist/buildtools/closure-stylesheets.jar --allow-unrecognized-properties "$SRC" > "$DST"
    ERROR="$?"
  else
    ERROR="1"
  fi

  if [ "$ERROR" = "1" ]
  then
    echo "Error minifying $SRC, copy $SRC to $DST"
    cp "$SRC" "$DST"
  fi
}

buildrelease() {
  echo "Minifying javascript"
  minify js htdocs/js/mympd.js dist/htdocs/js/mympd.min.js
  minify js htdocs/sw.js dist/htdocs/sw.min.js
  minify js htdocs/js/keymap.js dist/htdocs/js/keymap.min.js
  minify js dist/htdocs/js/bootstrap-native-v4.js dist/htdocs/js/bootstrap-native-v4.min.js

  echo "Minifying stylesheets"
  minify css htdocs/css/mympd.css dist/htdocs/css/mympd.min.css

  echo "Minifying html"
  minify html htdocs/index.html dist/htdocs/index.html

  echo "Creating i18n json"
  cd src/i18n || exit 1
  ./tojson.pl > ../../dist/htdocs/js/i18n.min.js
  cd ../.. || exit 1

  echo "Compiling mympd"
  install -d release
  cd release || exit 1
  export INSTALL_PREFIX="${MYMPD_INSTALL_PREFIX:-/usr}"
  cmake -DCMAKE_INSTALL_PREFIX:PATH="$INSTALL_PREFIX" -DCMAKE_BUILD_TYPE=RELEASE ..
  make
}

installrelease() {
  echo "Installing mympd"
  cd release || exit 1  
  make install DESTDIR="$DESTDIR"
  ../contrib/packaging/debian/postinst 
}

builddebug() {
  MEMCHECK=$1

  echo "Linking non-minfied files"
  [ -e "$PWD/htdocs/sw.min.js" ] || ln -s "$PWD/htdocs/sw.js" "$PWD/htdocs/sw.min.js"
  [ -e "$PWD/htdocs/js/mympd.min.js" ] || ln -s "$PWD/htdocs/js/mympd.js" "$PWD/htdocs/js/mympd.min.js"
  [ -e "$PWD/htdocs/js/bootstrap-native-v4.min.js" ] || ln -s "$PWD/dist/htdocs/js/bootstrap-native-v4.js" "$PWD/htdocs/js/bootstrap-native-v4.min.js"
  [ -e "$PWD/htdocs/js/keymap.min.js" ] || ln -s "$PWD/htdocs/js/keymap.js" "$PWD/htdocs/js/keymap.min.js"
  [ -e "$PWD/htdocs/css/mympd.min.css" ] || ln -s "$PWD/htdocs/css/mympd.css" "$PWD/htdocs/css/mympd.min.css"
  [ -e "$PWD/htdocs/css/bootstrap.min.css" ] || ln -s "$PWD/dist/htdocs/css/bootstrap.min.css" "$PWD/htdocs/css/bootstrap.min.css"

  echo "Creating i18n json"
  cd src/i18n || exit 1
  ./tojson.pl pretty > ../../htdocs/js/i18n.min.js
  cd ../.. || exit 1
  
  echo "Compiling"
  install -d debug
  cd debug || exit 1
  cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr -DCMAKE_BUILD_TYPE=DEBUG -DMEMCHECK="$MEMCHECK" ..
  make VERBOSE=1
}

cleanup() {
  rm -rf release
  rm -rf debug
  rm -rf package

  rm -f htdocs/sw.min.js
  rm -f htdocs/js/mympd.min.js
  rm -f htdocs/js/bootstrap-native-v4.min.js
  rm -f htdocs/js/keymap.min.js
  rm -f htdocs/js/i18n.min.js

  rm -f htdocs/css/mympd.min.css
  rm -f htdocs/css/bootstrap.min.css

  find ./ -name \*~ -delete
}

check () {
  if [ -x /usr/bin/cppcheck ]
  then
    echo "Running cppcheck"
    cppcheck --enable=warning --inconclusive --force --inline-suppr src/*.c src/*.h
    cppcheck --enable=warning --inconclusive --force --inline-suppr src/plugins/*.c src/plugins/*.h src/plugins/*.cpp
  else
    echo "cppcheck not found"
  fi
}

prepare() {
  cleanup
  SRC=$(ls -d "$PWD"/* -1)
  mkdir -p package/build
  cd package/build || exit 1
  for F in $SRC
  do
    cp -a "$F" .
  done
}

pkgdebian() {
  prepare
  cp -a contrib/packaging/debian .
  export LC_TIME="en_GB.UTF-8"
  cat > debian/changelog << EOL
mympd (${VERSION}-1) stable; urgency=medium

  * Release from master

 -- Juergen Mang <mail@jcgames.de>  $(date +"%a, %d %b %Y %H:%m:%S %z")
EOL

  tar -czf "../mympd_${VERSION}.orig.tar.gz" -- *
  dpkg-buildpackage -rfakeroot
}

pkgdocker() {
  prepare
  cp contrib/packaging/docker/Dockerfile .
  docker build -t mympd .
}

pkgalpine() {
  prepare
  tar -czf "mympd_${VERSION}.orig.tar.gz" -- *
  cp contrib/packaging/alpine/* .
  abuild checksum
  abuild -r
}

pkgrpm() {
  prepare
  SRC=$(ls)
  mkdir "mympd-${VERSION}"
  for F in $SRC
  do
    mv "$F" "mympd-${VERSION}"
  done
  install -d ~/rpmbuild/SOURCES
  tar -czf "mympd_${VERSION}.orig.tar.gz" "mympd-${VERSION}"
  mv "mympd_${VERSION}.orig.tar.gz" ~/rpmbuild/SOURCES/
  cp ../../contrib/packaging/rpm/mympd.spec .
  rpmbuild -ba mympd.spec
  RPMLINT=$(command -v rpmlint)
  if [ "$RPMLINT" != "" ]
  then
    ARCH=$(uname -p)
    rpmlint "~/rpmbuild/RPMS/${ARCH}/mympd-${VERSION}-0.${ARCH}.rpm"
  else
    echo "WARNING: rpmlint not found, can't check package"
  fi
}

pkgarch() {
  prepare
  tar -czf "mympd_${VERSION}.orig.tar.gz" -- *
  cp contrib/packaging/arch/* .
  makepkg
  NAMCAP=$(command -v namcap)
  if [ "$NAMCAP" != "" ]
  then
    namcap PKGBUILD
    namcap mympd-*.pkg.tar.xz
  else
    echo "WARNING: namcap not found, can't check package"
  fi
}

case "$1" in
	release)
	  buildrelease
	;;
	install)
	  installrelease
	;;
	releaseinstall)
	  buildrelease
	  cd .. || exit 1
	  installrelease
	;;
	debug)
	  builddebug "FALSE"
	;;
	memcheck)
	  builddebug "TRUE"
	;;
	cleanup)
	  cleanup
	;;
	check)
	  check
	;;
	pkgdebian)
	  pkgdebian
	;;
	pkgdocker)
	  pkgdocker
	;;
	pkgalpine)
	  pkgalpine
	;;
	pkgrpm)
	  pkgrpm
	;;
	pkgarch)
	  pkgarch
	;;
	*)
	  echo "Usage: $0 <option>"
	  echo "Options:"
	  echo "  release:        build release files in directory release"
	  echo "                  following environment variables are respected"
	  echo "                  MYMPD_INSTALL_PREFIX=/usr"
	  echo "  install:        installs release files from directory release"
	  echo "                  following environment variables are respected"
	  echo "                  DESTDIR=\"\""
	  echo "  installrelease: calls release and install afterwards"
	  echo "  debug:          builds debug files in directory debug"
	  echo "                  linked with libasan3, uses assets in htdocs"
	  echo "  memcheck:       builds debug files in directory debug "
	  echo "                  for use with valgrind, uses assets in htdocs/"
	  echo "  cleanup:        cleanup source tree"
	  echo "  check:          runs cppcheck"
	  echo "  pkgalpine:      creates the alpine package"
	  echo "  pkgarch:        creates the arch package"
	  echo "  pkgdebian:      creates the debian package"
	  echo "  pkgdocker:      creates the docker image (debian based with libmpdclient from git master branch)"
	  echo "  pkgrpm:         creates the rpm package"
	;;
esac
