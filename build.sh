#!/bin/sh

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
    $PERLBIN -pe 's/^<!--debug-->.*\n//gm; s/<!--release\s+(.+)-->/$1/g; s/^\s*//gm; s/\s*$//gm' "$SRC" | $GZIP > "$DST"
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
  cp dist/htdocs/js/i18n.min.js dist/htdocs/js/combined.js
  cat dist/htdocs/js/keymap.min.js >> dist/htdocs/js/combined.js
  cat dist/htdocs/js/bootstrap-native-v4.min.js >> dist/htdocs/js/combined.js
  cat dist/htdocs/js/mympd.min.js >> dist/htdocs/js/combined.js
  
  rm -f dist/htdocs/js/combined.js.gz
  $GZIPBIN -v -9 dist/htdocs/js/combined.js
  rm -f dist/htdocs/sw.js.gz
  $GZIPBIN -v -9 -c dist/htdocs/sw.min.js > dist/htdocs/sw.js.gz
 
  echo "Minifying stylesheets"
  minify css htdocs/css/mympd.css dist/htdocs/css/mympd.min.css
  
  echo "Combining and compressing stylesheets"
  cp dist/htdocs/css/bootstrap.css dist/htdocs/css/combined.css
  cat dist/htdocs/css/mympd.min.css >> dist/htdocs/css/combined.css
  
  rm -f dist/htdocs/css/combined.css.gz
  $GZIPBIN -v -9 dist/htdocs/css/combined.css
  
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
  else
    echo "No old installation found"
  fi
  if [ -d /lib/systemd/system/mympd.service ]
  then
    mv /usr/lib/systemd/system/mympd.service /lib/systemd/system/mympd.service
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
  LINTIAN=$(command -v lintian)
  if [ "$LINTIAN" != "" ]
  then
    echo "Checking package with lintian"
    ARCH=$(uname -p)
    [ "$ARCH" = "x86_64" ] && ARCH="amd64"
    $LINTIAN "../mympd_${VERSION}-1_${ARCH}.deb"
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
  install -d "$HOME/rpmbuild/SOURCES"
  tar -czf "mympd_${VERSION}.orig.tar.gz" "mympd-${VERSION}"
  mv "mympd_${VERSION}.orig.tar.gz" ~/rpmbuild/SOURCES/
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
	  echo "  releaseinstall: calls release and install afterwards"
	  echo "  debug:          builds debug files in directory debug"
	  echo "                  linked with libasan3, uses assets in htdocs"
	  echo "  memcheck:       builds debug files in directory debug "
	  echo "                  for use with valgrind, uses assets in htdocs/"
	  echo "  cleanupoldinst: removes deprecated files"
	  echo "  cleanup:        cleanup source tree"
	  echo "  check:          runs cppcheck"
	  echo "  pkgalpine:      creates the alpine package"
	  echo "  pkgarch:        creates the arch package"
	  echo "  pkgdebian:      creates the debian package"
	  echo "  pkgdocker:      creates the docker image (debian based with libmpdclient from git master branch)"
	  echo "  pkgrpm:         creates the rpm package"
	;;
esac
