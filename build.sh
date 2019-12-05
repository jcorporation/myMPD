#!/bin/sh
#
# SPDX-License-Identifier: GPL-2.0-or-later
# myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd
#

if [ "${EMBEDDED_LIBMPDCLIENT}" = "" ]
then
  export EMBEDDED_LIBMPDCLIENT="ON"
fi

if [ "${ENABLE_SSL}" = "" ]
then
  export ENABLE_SSL="ON"
fi

if [ "${ENABLE_LIBID3TAG}" = "" ]
then
  export ENABLE_LIBID3TAG="ON"
fi

if [ "${ENABLE_FLAC}" = "" ]
then
  export ENABLE_FLAC="ON"
fi

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
GZIP="$GZIPBIN -f -v -9"
GZIPCAT="$GZIPBIN -f -v -9 -c"

#perl is needed to create i18n.js
PERLBIN=$(command -v perl)
if [ "$PERLBIN" = "" ]
then
  echo "ERROR: perl not found"
  exit 1
fi

#java is optional to minify js and css
JAVABIN=$(command -v java)

#returns true if FILE1 is newer or equal than FILE2
newer() {
  M1=0
  M2=0
  [ -f "$1" ] && M1=$(stat -c%Y "$1")
  [ -f "$2" ] && M2=$(stat -c%Y "$2")
  if [ "$M1" -lt "$M2" ]
  then
    #echo "$1 is older than $2"
    return 1
  elif [ "$M1" -eq "$M2" ]
  then
    #echo "$1 is equal than $2"
    return 0
  else
    #echo "$1 is newer than $2"
    return 0
  fi
}

#returns true if FILE1 is older than FILE...
older_s() {
  FILE1=$1
  for FILE2 in "$@"
  do
    [ "$FILE1" = "$FILE2" ] && continue
    if newer "$FILE2" "$FILE1"
    then
      #echo "$FILE1 is older than $FILE2"
      return 0
    fi
  done
  #echo "$FILE1 is newer or equal than $@"
  return 1
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
  
  if newer "$DST" "$SRC"
  then
    #File already minified"
    echo "Skipping $SRC"
    return 1
  fi
  echo "Minifying $SRC"

  if [ "$TYPE" = "html" ] && [ "$PERLBIN" != "" ]
  then
    # shellcheck disable=SC2016
    $PERLBIN -pe 's/^<!--debug-->.*\n//gm; s/<!--release\s+(.+)-->/$1/g; s/<!--(.+)-->//g; s/^\s*//gm; s/\s*$//gm' "$SRC" > "${DST}.tmp"
    ERROR="$?"
    if [ "$ERROR" = "1" ]
    then
      rm -f "${DST}.tmp"
      echo "Error minifying $SRC"
      exit 1
    fi
  elif [ "$TYPE" = "js" ] && [ "$JAVABIN" != "" ]
  then
    $JAVABIN -jar dist/buildtools/closure-compiler.jar "$SRC" > "${DST}.tmp"
    ERROR="$?"
  elif [ "$TYPE" = "css" ] && [ "$JAVABIN" != "" ]
  then
    $JAVABIN -jar dist/buildtools/closure-stylesheets.jar --allow-unrecognized-properties "$SRC" > "${DST}.tmp"
    ERROR="$?"
  else
    ERROR="1"
  fi

  if [ "$ERROR" = "1" ]
  then
    if [ "$JAVABIN" = "" ]
    then
      echo "Java not found, copy $SRC to $DST"
    else
      echo "Error minifying $SRC, copy $SRC to $DST"
    fi
    rm -f "${DST}.tmp"
    cp "$SRC" "$DST"
  else
    mv "${DST}.tmp" "$DST"
  fi
  #successfull minified or copied file
  return 0
}

createi18n() {
  DST=$1
  PRETTY=$2
  cd src/i18n || exit 1
  if older_s "$DST" ./*.txt
  then
    echo "Creating i18n json"
    $PERLBIN ./tojson.pl "$PRETTY" > "$DST"
  else
    echo "Skip creating i18n json"
  fi
  cd ../.. || exit 1
}

buildrelease() {
  ASSETSCHANGED=0

  createi18n ../../dist/htdocs/js/i18n.min.js
  
  echo "Minifying javascript"
  JSSRCFILES=""
  for F in htdocs/js/*.js
  do
    [ -L "$F" ] || JSSRCFILES="$JSSRCFILES $F"
    if tail -1 "$F" | perl -npe 'exit 1 if m/\n/; exit 0'
    then
      echo "ERROR: $F don't end with newline character"
      exit 1
    fi
  done
  # shellcheck disable=SC2086
  if older_s dist/htdocs/js/mympd.js $JSSRCFILES
  then
    echo "Creating mympd.js"
    # shellcheck disable=SC2086
    # shellcheck disable=SC2002
    cat $JSSRCFILES | grep -v "\"use strict\";" > dist/htdocs/js/mympd.js
  else
    echo "Skip creating mympd.js"
  fi
  minify js htdocs/sw.js dist/htdocs/sw.min.js
  minify js htdocs/js/keymap.js dist/htdocs/js/keymap.min.js
  minify js dist/htdocs/js/bootstrap-native-v4.js dist/htdocs/js/bootstrap-native-v4.min.js
  minify js dist/htdocs/js/mympd.js dist/htdocs/js/mympd.min.js
  
  echo "Combining and compressing javascript"
  JSFILES=dist/htdocs/js/*.min.js
  for F in $JSFILES
  do
    if tail -1 "$F" | perl -npe 'exit 1 if m/\n/; exit 0'
    then
      echo "ERROR: $F don't end with newline character"
      exit 1
    fi
  done
  # shellcheck disable=SC2086
  if older_s dist/htdocs/js/combined.js.gz $JSFILES
  then
    echo "\"use strict\";" > dist/htdocs/js/combined.js
    # shellcheck disable=SC2086
    # shellcheck disable=SC2002
    cat $JSFILES >> dist/htdocs/js/combined.js
    $GZIP dist/htdocs/js/combined.js
    ASSETSCHANGED=1
  else
    echo "Skip creating dist/htdocs/js/combined.js.gz"
  fi
  if newer dist/htdocs/sw.min.js dist/htdocs/sw.js.gz
  then
    $GZIPCAT dist/htdocs/sw.min.js > dist/htdocs/sw.js.gz
    ASSETSCHANGED=1
  else
    echo "Skip dist/htdocs/sw.js.gz"
  fi
 
  echo "Minifying stylesheets"
  for F in htdocs/css/*.css
  do
    DST=$(basename "$F" .css)
    [ -L "$F" ] || minify css "$F" "dist/htdocs/css/${DST}.min.css"
  done
  
  echo "Combining and compressing stylesheets"
  CSSFILES=dist/htdocs/css/*.min.css
  # shellcheck disable=SC2086
  if older_s dist/htdocs/css/combined.css.gz $CSSFILES
  then
    # shellcheck disable=SC2086
    cat $CSSFILES > dist/htdocs/css/combined.css
    $GZIP dist/htdocs/css/combined.css
    ASSETSCHANGED=1
  else
    echo "Skip creating dist/htdocs/css/combined.css.gz"
  fi
  
  echo "Minifying and compressing html"
  if minify html htdocs/index.html dist/htdocs/index.html
  then
    $GZIPCAT dist/htdocs/index.html > dist/htdocs/index.html.gz
    ASSETSCHANGED=1
  fi

  echo "Creating other compressed assets"
  ASSETS="htdocs/mympd.webmanifest htdocs/assets/*.svg"
  for ASSET in $ASSETS
  do
    COMPRESSED="dist/${ASSET}.gz"
    if newer "$ASSET" "$COMPRESSED"
    then
      $GZIPCAT "$ASSET" > "$COMPRESSED"
      ASSETSCHANGED=1
    else
      echo "Skipping $ASSET"
    fi
  done

  echo "Compiling myMPD"
  install -d release
  cd release || exit 1
  if [ "$ASSETSCHANGED" = "1" ]
  then
    echo "Assets changed"
    #force rebuild of web_server.c with embedded assets
    rm -vf CMakeFiles/mympd.dir/src/web_server/web_server_utility.c.o
  else
    echo "Assets not changed"
  fi
  #set INSTALL_PREFIX and build myMPD
  export INSTALL_PREFIX="${MYMPD_INSTALL_PREFIX:-/usr}"
  cmake -DCMAKE_INSTALL_PREFIX:PATH="$INSTALL_PREFIX" -DCMAKE_BUILD_TYPE=RELEASE \
  	-DEMBEDDED_LIBMPDCLIENT="$EMBEDDED_LIBMPDCLIENT" -DENABLE_SSL="$ENABLE_SSL" \
  	-DENABLE_LIBID3TAG="$ENABLE_LIBID3TAG" -DENABLE_FLAC="$ENABLE_FLAC" ..
  make
}

addmympduser() {
  echo "Checking status of mympd system user and group"
  getent group mympd > /dev/null || groupadd -r mympd
  getent passwd mympd > /dev/null || useradd -r -g mympd -s /bin/false -d /var/lib/mympd mympd
}

installrelease() {
  echo "Installing myMPD"
  cd release || exit 1  
  make install DESTDIR="$DESTDIR"
  addmympduser
}

builddebug() {
  MEMCHECK=$1

  echo "Linking bootstrap css and js"
  [ -e "$PWD/htdocs/css/bootstrap.css" ] || ln -s "$PWD/dist/htdocs/css/bootstrap.css" "$PWD/htdocs/css/bootstrap.css"
  [ -e "$PWD/htdocs/js/bootstrap-native-v4.js" ] || ln -s "$PWD/dist/htdocs/js/bootstrap-native-v4.js" "$PWD/htdocs/js/bootstrap-native-v4.js"

  createi18n ../../htdocs/js/i18n.js pretty
  
  echo "Compiling myMPD"
  install -d debug
  cd debug || exit 1
  cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr -DCMAKE_BUILD_TYPE=DEBUG -DMEMCHECK="$MEMCHECK" \
  	-DEMBEDDED_LIBMPDCLIENT="$EMBEDDED_LIBMPDCLIENT" -DENABLE_SSL="$ENABLE_SSL" \
  	-DENABLE_LIBID3TAG="$ENABLE_LIBID3TAG" -DENABLE_FLAC="$ENABLE_FLAC" ..
  make VERBOSE=1
}

cleanupoldinstall() {
  if [ -d /usr/share/mympd ] || [ -d /usr/lib/mympd ]
  then
    echo "Cleaning up previous myMPD installation"
    rm -rf /usr/share/mympd
    rm -rf /var/lib/mympd/tmp
    [ -f /etc/mympd/mympd.conf ] && mv /etc/mympd/mympd.conf /etc/mympd.conf
    rm -rf /etc/mympd
    rm -f /usr/lib/systemd/system/mympd.service
    [ -d /usr/lib/systemd/system ] && rmdir --ignore-fail-on-non-empty /usr/lib/systemd/system
    rm -rf /usr/lib/mympd
  else
    echo "No old myMPD installation found"
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

cleanupdist() {
  rm -f dist/htdocs/js/i18n.min.js
  rm -f dist/htdocs/js/keymap.min.js 
  rm -f dist/htdocs/js/bootstrap-native-v4.min.js 
  rm -f dist/htdocs/js/mympd.js
  rm -f dist/htdocs/js/mympd.min.js
  rm -f dist/htdocs/js/combined.js.gz
  rm -f dist/htdocs/css/mympd.min.css
  rm -f dist/htdocs/css/theme-*.min.css
  rm -f dist/htdocs/css/combined.css.gz
  rm -f dist/htdocs/sw.min.js
  rm -f dist/htdocs/sw.js.gz
  rm -f dist/htdocs/mympd.webmanifest.gz
  rm -f dist/htdocs/index.html
  rm -f dist/htdocs/index.html.gz
  rm -f dist/htdocs/assets/*.gz
}

check () {
  CPPCHECKBIN=$(command -v cppcheck)
  [ "$CPPCHECKOPTS" = "" ] && CPPCHECKOPTS="--enable=warning"
  if [ "$CPPCHECKBIN" != "" ]
  then
    echo "Running cppcheck"
    $CPPCHECKBIN $CPPCHECKOPTS src/*.c src/*.h
    $CPPCHECKBIN $CPPCHECKOPTS src/mpd_client/*.c src/mpd_client/*.h
    $CPPCHECKBIN $CPPCHECKOPTS src/mympd_api/*.c src/mympd_api/*.h
    $CPPCHECKBIN $CPPCHECKOPTS src/plugins/*.c src/plugins/*.h src/plugins/*.cpp
  else
    echo "cppcheck not found"
  fi
  
  FLAWFINDERBIN=$(command -v flawfinder)
  [ "$FLAWFINDEROPTS" = "" ] && FLAWFINDEROPTS="-m3"
  if [ "$FLAWFINDERBIN" != "" ]
  then
    echo "Running flawfinder"
    $FLAWFINDERBIN $FLAWFINDEROPTS src
  else
    echo "flawfinder not found"
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
  OSCBIN=$(command -v osc)
  if [ "$OSCBIN" = "" ]
  then
    echo "ERROR: osc not found"
    exit 1
  fi
  
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
  osc vc
  osc commit
}

installdeps() {
  if [ -f /etc/debian_version ]
  then
    #debian
    apt-get update
    apt-get install -y --no-install-recommends \
	gcc cmake perl libssl-dev libid3tag0-dev libflac-dev \
	default-jre-headless build-essential
  elif [ -f /etc/arch-release ]
  then
    #arch
    pacman -S gcc cmake perl openssl libid3tag flac jre-openjdk-headless
  elif [ -f /etc/alpine-release ]
  then
    #alpine
    apk add gcc cmake perl openssl-dev libid3tag-dev libflac-dev \
    	openjdk11-jre-headlesslinux-headers
  elif [ -f /etc/SuSE-release ]
  then
    #suse
    zypper install gcc cmake pkgconfig perl openssl-devel libid3tag-devel flac-devel \
	java-11-openjdk-headless unzip
  elif [ -f /etc/redhat-release ]
  then  
    #fedora 	
    yum install gcc cmake pkgconfig perl openssl-devel libid3tag-devel flac-devel \
	java-11-openjdk-headless unzip
  else 
    echo "No supported distribution detected."
  fi
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
	installdeps)
	  installdeps
	;;
	cleanup)
	  cleanup
	  cleanuposc
	;;
	cleanupdist)
	  cleanupdist
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
	addmympduser)
	  addmympduser
	;;
	*)
	  echo "Usage: $0 <option>"
	  echo "Version: ${VERSION}"
	  echo ""
	  echo "Build options:"
	  echo "  release:        build release files in directory release"
	  echo "  install:        installs release files from directory release"
	  echo "                  following environment variables are respected"
	  echo "                    - DESTDIR=\"\""
	  echo "  releaseinstall: calls release and install afterwards"
	  echo "  debug:          builds debug files in directory debug,"
	  echo "                  linked with libasan3, uses assets in htdocs"
	  echo "  memcheck:       builds debug files in directory debug"
	  echo "                  for use with valgrind, uses assets in htdocs/"
	  echo "  check:          runs cppcheck and flawfinder on source files"
	  echo "                  following environment variables are respected"
	  echo "                    - CPPCHECKOPTS=\"--enable=warning\""
	  echo "                    - FLAWFINDEROPTS=\"-m3\""
	  echo "  installdeps:    installs build and run dependencies"
	  echo ""
	  echo "Cleanup options:"
	  echo "  cleanup:        cleanup source tree"
	  echo "  cleanupdist:    cleanup dist directory, forces release to build new assets"
	  echo "  cleanupoldinst: removes deprecated files"
	  echo ""
	  echo "Packaging options:"
	  echo "  pkgalpine:      creates the alpine package"
	  echo "  pkgarch:        creates the arch package"
	  echo "                  following environment variables are respected"
	  echo "                    - SIGN=\"FALSE\""
	  echo "                    - GPGKEYID=\"\""
	  echo "  pkgdebian:      creates the debian package"
	  echo "                  following environment variables are respected"
	  echo "                    - SIGN=\"FALSE\""
	  echo "                    - GPGKEYID=\"\""
	  echo "  pkgdocker:      creates the docker image (debian based)"
	  echo "  pkgrpm:         creates the rpm package"
	  echo "  pkgosc:         updates the open build service repository"
	  echo "                  following environment variables are respected"
	  echo "                    - OSC_REPO=\"home:jcorporation/myMPD\""
	  echo ""
	  echo "Misc options:"
	  echo "  setversion:     sets version and date in packaging files from CMakeLists.txt"
	  echo "  addmympduser:   adds mympd group and user"
	  echo ""
	  echo "Environment variables for building"
	  echo "  - MYMPD_INSTALL_PREFIX=\"/usr\""
	  echo "  - EMBEDDED_LIBMPDCLIENT=\"ON\""
	  echo "  - ENABLE_SSL=\"ON\""
	  echo "  - ENABLE_LIBID3TAG=\"ON\""
	  echo "  - ENABLE_FLAC=\"ON\""
	  echo ""
	;;
esac
