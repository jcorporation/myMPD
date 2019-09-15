#!/bin/sh

STARTPATH=$(pwd)

#set umask
umask 0022

#get myMPD version
VERSION=$(grep VERSION_ CMakeLists.txt | cut -d\" -f2 | tr '\n' '.' | sed 's/\.$//')

#gzip is needed to compress assets for release
GZIPBIN=$(command -v gzip)
if [ "$GZIPBIN" = "" ]
then
  echo "ERROR: gzip not found"
  exit 1
fi
GZIP="$GZIPBIN -v -9 -c -"

#perl is needed to create i18n.js
PERLBIN=$(command -v perl)
if [ "$PERLBIN" = "" ]
then
  echo "ERROR: perl not found"
  exit 1
fi

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

setversion() {
  echo "Setting version to ${VERSION}"
  export LC_TIME="en_GB.UTF-8"
  
  sed -e "s/__VERSION__/${VERSION}/g" htdocs/sw.js.in > htdocs/sw.js
  sed -e "s/__VERSION__/${VERSION}/g" contrib/packaging/alpine/APKBUILD.in > contrib/packaging/alpine/APKBUILD
  sed -e "s/__VERSION__/${VERSION}/g" contrib/packaging/arch/PKGBUILD.in > contrib/packaging/arch/PKGBUILD
  DATE=$(date +"%a %b %d %Y")
  sed -e "s/__VERSION__/${VERSION}/g" -e "s/__DATE__/$DATE/g" \
  	contrib/packaging/rpm/mympd.spec.in > contrib/packaging/rpm/mympd.spec
  DATE=$(date +"%a, %d %b %Y %H:%m:%S %z")
  sed -e "s/__VERSION__/${VERSION}/g" -e "s/__DATE__/$DATE/g" \
  	contrib/packaging/debian/changelog.in > contrib/packaging/debian/changelog
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

  if [ "$TYPE" = "html" ] && [ "$PERLBIN" != "" ]
  then
    $PERLBIN -pe 's/^<!--debug-->.*\n//gm; s/<!--release\s+(.+)-->/$1/g; s/<!--(.+)-->//g; s/^\s*//gm; s/\s*$//gm' "$SRC" | $GZIP > "$DST"
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
  echo "Creating i18n json"
  cd src/i18n || exit 1
  $PERLBIN ./tojson.pl > ../../dist/htdocs/js/i18n.min.js
  cd ../.. || exit 1
  
  echo "Minifying javascript"
  minify js htdocs/sw.js dist/htdocs/sw.min.js
  minify js htdocs/js/keymap.js dist/htdocs/js/keymap.min.js
  minify js dist/htdocs/js/bootstrap-native-v4.js dist/htdocs/js/bootstrap-native-v4.min.js
  minify js htdocs/js/mympd.js dist/htdocs/js/mympd.min.js
  
  echo "Combining and compressing javascript"
  cat dist/htdocs/js/i18n.min.js \
      dist/htdocs/js/keymap.min.js \
      dist/htdocs/js/bootstrap-native-v4.min.js \
      dist/htdocs/js/mympd.min.js > dist/htdocs/js/combined.js
  
  $GZIPBIN -f -v -9 dist/htdocs/js/combined.js
  $GZIPBIN -f -v -9 -c dist/htdocs/sw.min.js > dist/htdocs/sw.js.gz
 
  echo "Minifying stylesheets"
  minify css htdocs/css/mympd.css dist/htdocs/css/mympd.min.css
  
  echo "Combining and compressing stylesheets"
  cat dist/htdocs/css/bootstrap.css \
      dist/htdocs/css/mympd.min.css > dist/htdocs/css/combined.css
  
  $GZIPBIN -f -v -9 dist/htdocs/css/combined.css
  
  echo "Minifying and compressing html"
  minify html htdocs/index.html dist/htdocs/index.html.gz

  echo "Creating other compressed assets"
  ASSETS="htdocs/mympd.webmanifest"
  ASSETS="$ASSETS htdocs/assets/coverimage-notavailable.svg htdocs/assets/coverimage-stream.svg"
  ASSETS="$ASSETS htdocs/assets/coverimage-loading.svg "
  for ASSET in $ASSETS
  do
    COMPRESSED="dist/${ASSET}.gz"
    if newer "$ASSET" "$COMPRESSED"
    then 
      $GZIPBIN -v -9 -c "$ASSET" > "$COMPRESSED"
    fi
  done

  echo "Compiling mympd"
  install -d release
  cd release || exit 1
  #force rebuild of web_server.c with embedded assets
  rm -f CMakeFiles/mympd.dir/src/web_server.c.o
  #set INSTALL_PREFIX and build myMPD
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

  echo "Linking bootstrap css and js"
  [ -e "$PWD/htdocs/css/bootstrap.css" ] || ln -s "$PWD/dist/htdocs/css/bootstrap.css" "$PWD/htdocs/css/bootstrap.css"
  [ -e "$PWD/htdocs/js/bootstrap-native-v4.js" ] || ln -s "$PWD/dist/htdocs/js/bootstrap-native-v4.js" "$PWD/htdocs/js/bootstrap-native-v4.js"

  echo "Creating i18n json"
  cd src/i18n || exit 1
  $PERLBIN ./tojson.pl pretty > ../../htdocs/js/i18n.js
  cd ../.. || exit 1
  
  echo "Compiling"
  install -d debug
  cd debug || exit 1
  cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr -DCMAKE_BUILD_TYPE=DEBUG -DMEMCHECK="$MEMCHECK" ..
  make VERBOSE=1
}

cleanupoldinstall() {
  if [ -d /usr/share/mympd ]
  then
    echo "Previous installation found"
    rm -rf /usr/share/mympd
    rm -rf /var/lib/mympd/tmp
    mv /etc/mympd/mympd.conf /etc/mympd.conf
    rm -rf /etc/mympd
    rm -f /usr/lib/systemd/system/mympd.service
    rmdir --ignore-fail-on-non-empty /usr/lib/systemd/system
  else
    echo "No old installation found"
  fi
}

cleanup() {
  #build directories
  rm -rf release
  rm -rf debug
  rm -rf package
  
  #htdocs
  rm -f htdocs/js/bootstrap-native-v4.js
  rm -f htdocs/js/i18n.js
  rm -f htdocs/css/bootstrap.css

  #tmp files
  find ./ -name \*~ -delete
}

cleanuposc() {
  rm -rf osc
}

check () {
  CPPCHECKBIN=$(command -v cppcheck)
  if [ "$CPPCHECKBIN" != "" ]
  then
    echo "Running cppcheck"
    $CPPCHECKBIN --enable=warning --inconclusive --force --inline-suppr src/*.c src/*.h
    $CPPCHECKBIN --enable=warning --inconclusive --force --inline-suppr src/plugins/*.c src/plugins/*.h src/plugins/*.cpp
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
    [ "$F" = "$STARTPATH/osc" ] && continue
    cp -a "$F" .
  done
  rm -r dist/buildtools
}

pkgdebian() {
  prepare
  cp -a contrib/packaging/debian .
  export LC_TIME="en_GB.UTF-8"
  tar -czf "../mympd_${VERSION}.orig.tar.gz" -- *
  dpkg-buildpackage -rfakeroot

  #get created package name
  PACKAGE=$(ls ../mympd_"${VERSION}"-1_*.deb)
  if [ "$PACKAGE" = "" ]
  then
    echo "Can't find package"
  fi

  if [ "$SIGN" = "TRUE" ]
  then  
    DPKGSIG=$(command -v dpkg-sig)
    if [ "$DPKGSIG" != "" ]
    then
      if [ "$GPGKEYID" != "" ]
      then
        echo "Signing package with key $GPGKEYID"
        dpkg-sig -k "$GPGKEYID" --sign builder "$PACKAGE"
      else
        echo "WARNING: GPGKEYID not set, can't sign package"
      fi
    else
      echo "WARNING: dpkg-sig not found, can't sign package"
    fi
  fi
  
  LINTIAN=$(command -v lintian)
  if [ "$LINTIAN" != "" ]
  then
    echo "Checking package with lintian"
    $LINTIAN "$PACKAGE"
  else
    echo "WARNING: lintian not found, can't check package"
  fi
}

pkgdocker() {
  prepare
  cp contrib/packaging/docker/Dockerfile .
  docker build -t mympd .
}

pkgalpine() {
  prepare
  tar -czf "mympd_${VERSION}.orig.tar.gz" -- *
  [ "$1" = "taronly" ] && return 0
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
  tar -czf "mympd-${VERSION}.tar.gz" "mympd-${VERSION}"
  [ "$1" = "taronly" ] && return 0
  install -d "$HOME/rpmbuild/SOURCES"
  mv "mympd-${VERSION}.tar.gz" ~/rpmbuild/SOURCES/
  cp ../../contrib/packaging/rpm/mympd.spec .
  rpmbuild -ba mympd.spec
  RPMLINT=$(command -v rpmlint)
  if [ "$RPMLINT" != "" ]
  then
    echo "Checking package with rpmlint"
    ARCH=$(uname -p)
    $RPMLINT "$HOME/rpmbuild/RPMS/${ARCH}/mympd-${VERSION}-0.${ARCH}.rpm"
  else
    echo "WARNING: rpmlint not found, can't check package"
  fi
}

pkgarch() {
  prepare
  tar -czf "mympd_${VERSION}.orig.tar.gz" -- *
  cp contrib/packaging/arch/* .
  makepkg
  if [ "$SIGN" = "TRUE" ]
  then
    KEYARG=""
    [ "$GPGKEYID" != "" ] && KEYARG="--key $PGPGKEYID"
    makepkg --sign "$KEYARG" mympd-*.pkg.tar.xz
  fi
  NAMCAP=$(command -v namcap)
  if [ "$NAMCAP" != "" ]
  then
    echo "Checking package with namcap"
    $NAMCAP PKGBUILD
    $NAMCAP mympd-*.pkg.tar.xz
  else
    echo "WARNING: namcap not found, can't check package"
  fi
}

pkgosc() {
  cleanup
  cleanuposc
  [ "$OSC_REPO" = "" ] && OSC_REPO="home:jcorporation/myMPD"
  
  mkdir osc
  cd osc || exit 1  
  osc checkout "$OSC_REPO"
  rm -f "$OSC_REPO"/*
  
  cd "$STARTPATH" || exit 1
  pkgrpm taronly

  cd "$STARTPATH" || exit 1
  cp "package/build/mympd-${VERSION}.tar.gz" "osc/$OSC_REPO/"
  
  if [ -f /etc/debian_version ]
  then
    pkgdebian
  else
    pkgalpine taronly
    rm -f "$OSC_REPO"/debian.*
  fi

  cd "$STARTPATH/osc" || exit 1
  cp "../package/mympd_${VERSION}.orig.tar.gz" "$OSC_REPO/"
  if [ -f /etc/debian_version ]
  then
    cp "../package/mympd_${VERSION}-1.dsc" "$OSC_REPO/"
    cp "../package/mympd_${VERSION}-1.debian.tar.xz"  "$OSC_REPO/"
  fi
  cp ../contrib/packaging/rpm/mympd.spec "$OSC_REPO/"
  cp ../contrib/packaging/arch/PKGBUILD "$OSC_REPO/"
  cp ../contrib/packaging/arch/archlinux.install "$OSC_REPO/"

  cd "$OSC_REPO" || exit 1
  osc addremove
  osc st
  osc commit
}

case "$1" in
	release)
	  buildrelease
	;;
	install)
	  cleanupoldinstall
	  installrelease
	;;
	releaseinstall)
	  buildrelease
	  cd .. || exit 1
	  cleanupoldinstall
	  installrelease
	;;
	cleanupoldinst)
	  cleanupoldinstall
	;;
	debug)
	  builddebug "FALSE"
	;;
	memcheck)
	  builddebug "TRUE"
	;;
	cleanup)
	  cleanup
	  cleanuposc
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
	setversion)
	  setversion
	;;
	pkgosc)
	  pkgosc
	;;
	*)
	  echo "Usage: $0 <option>"
	  echo "Version: ${VERSION}"
	  echo "Options:"
	  echo "  release:        build release files in directory release"
	  echo "                  following environment variables are respected"
	  echo "                    - MYMPD_INSTALL_PREFIX=\"/usr\""
	  echo "  install:        installs release files from directory release"
	  echo "                  following environment variables are respected"
	  echo "                    - DESTDIR=\"\""
	  echo "  releaseinstall: calls release and install afterwards"
	  echo "  debug:          builds debug files in directory debug"
	  echo "                  linked with libasan3, uses assets in htdocs"
	  echo "  memcheck:       builds debug files in directory debug"
	  echo "                  for use with valgrind, uses assets in htdocs/"
	  echo "  cleanupoldinst: removes deprecated files"
	  echo "  cleanup:        cleanup source tree"
	  echo "  check:          runs cppcheck"
	  echo "  pkgalpine:      creates the alpine package"
	  echo "  pkgarch:        creates the arch package"
	  echo "                  following environment variables are respected"
	  echo "                    - SIGN=\"FALSE\""
	  echo "                    - GPGKEYID=\"\""
	  echo "  pkgdebian:      creates the debian package"
	  echo "                  following environment variables are respected"
	  echo "                    - SIGN=\"FALSE\""
	  echo "                    - GPGKEYID=\"\""
	  echo "  pkgdocker:      creates the docker image (debian based with libmpdclient from git master branch)"
	  echo "  pkgrpm:         creates the rpm package"
	  echo "  setversion:     sets version and date in packaging files from CMakeLists.txt"
#Working on osc support
#	  echo "  pkgosc:            updates a osc repository"
#	  echo "                  following environment variables are respected"
#	  echo "                  OSC_REPO=\"home:jcorporation/myMPD\""
	;;
esac
