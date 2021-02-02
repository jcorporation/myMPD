#!/bin/sh
#
# SPDX-License-Identifier: GPL-2.0-or-later
# myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd
#

#exit on error
set -e

#exit on undefined variable
set -u

#print out commands
[ -z "${DEBUG+x}" ] || set -x

#default compile settings
if [ -z "${ENABLE_SSL+x}" ]
then
  export ENABLE_SSL="ON"
fi

if [ -z "${ENABLE_LIBID3TAG+x}" ]
then
  export ENABLE_LIBID3TAG="ON"
fi

if [ -z "${ENABLE_FLAC+x}" ]
then
  export ENABLE_FLAC="ON"
fi

if [ -z "${ENABLE_LUA+x}" ]
then
  export ENABLE_LUA="ON"
fi

#save startpath
STARTPATH=$(pwd)

#set umask
umask 0022

#get myMPD version
VERSION=$(grep CPACK_PACKAGE_VERSION_ CMakeLists.txt | cut -d\" -f2 | tr '\n' '.' | sed 's/\.$//')

#check for command
check_cmd() {
  for DEPENDENCY in "$@"
  do
	if ! command -v "${DEPENDENCY}" > /dev/null
    then
      echo "ERROR: ${DEPENDENCY} not found"
      return 1
    fi
  done
  return 0
}

#get action
if [ -z "${1+x}" ]
then
  ACTION=""
else
  ACTION="$1"
fi

if [ "$ACTION" != "installdeps" ] && [ "$ACTION" != "" ]
then
  check_cmd gzip bzcat perl

  GZIP="gzip -f -v -9"
  GZIPCAT="gzip -f -v -9 -c"
fi

#java is optional to minify js and css
JAVA="0"
if check_cmd java
then
  JAVA="1"
fi

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
  check_cmd bzcat
  echo "Setting version to ${VERSION}"
  export LC_TIME="en_GB.UTF-8"
  DATE_F1=$(date +"%a %b %d %Y")
  DATE_F2=$(date +"%a, %d %b %Y %H:%m:%S %z")
  DATE_F3=$(date +"%d %b %Y")
  for F in htdocs/sw.js contrib/packaging/alpine/APKBUILD contrib/packaging/arch/PKGBUILD \
  		   contrib/packaging/rpm/mympd.spec contrib/packaging/debian/changelog contrib/man/mympd.1 \
  		   contrib/man/mympd-config.1 contrib/man/mympd-script.1
  do
  	if ! newer "$F.in" "$F"
  	then 
  	  echo "Warning: $F is newer than $F.in"
  	else
  	  echo "$F"
  	  sed -e "s/__VERSION__/${VERSION}/g" -e "s/__DATE_F1__/$DATE_F1/g" -e "s/__DATE_F2__/$DATE_F2/g" \
  	  	-e "s/__DATE_F3__/$DATE_F3/g" "$F.in" > "$F"
  	  #Adjust file modification date
  	  TS=$(stat -c%Y "$F.in")
  	  touch -d@"$TS" "$F"
  	fi
  done
  #compress manpages
  for F in contrib/man/mympd.1 contrib/man/mympd-config.1 contrib/man/mympd-script.1
  do
    gzip -n -9 -c "$F" > "$F.gz"
    bzcat -c -z "$F" > "$F.bz2"
  done

  #genoo ebuild must be moved only
  [ -f "contrib/packaging/gentoo/mympd-${VERSION}.ebuild" ] || \
  	mv -f contrib/packaging/gentoo/mympd-*.ebuild "contrib/packaging/gentoo/mympd-${VERSION}.ebuild"
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

  if [ "$TYPE" = "html" ]
  then
    #shellcheck disable=SC2016
    perl -pe 's/^<!--debug-->.*\n//gm; s/<!--release\s+(.+)-->/$1/g; s/<!--(.+)-->//g; s/^\s*//gm; s/\s*$//gm' "$SRC" > "${DST}.tmp"
    ERROR="$?"
    if [ "$ERROR" = "1" ]
    then
      rm -f "${DST}.tmp"
      echo "Error minifying $SRC"
      exit 1
    fi
  elif [ "$TYPE" = "js" ] && [ "$JAVA" = "1" ]
  then
    java -jar dist/buildtools/closure-compiler.jar "$SRC" > "${DST}.tmp"
    ERROR="$?"
  elif [ "$TYPE" = "css" ] && [ "$JAVA" = "1" ]
  then
    java -jar dist/buildtools/closure-stylesheets.jar --allow-unrecognized-properties "$SRC" > "${DST}.tmp"
    ERROR="$?"
  else
    ERROR="1"
  fi

  if [ "$ERROR" = "1" ]
  then
    if [ "$JAVA" = "0" ]
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
    perl ./tojson.pl "$PRETTY" > "$DST"
  else
    echo "Skip creating i18n json"
  fi
  cd ../.. || exit 1
}

createdistfiles() {
  echo "Creating dist files"
  ASSETSCHANGED=0

  createi18n ../../dist/htdocs/js/i18n.min.js ""
  
  echo "Minifying javascript"
  JSSRCFILES=""
  for F in htdocs/js/*.js
  do
    [ "$F" = "htdocs/js/i18n.js" ] && continue
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
  minify js dist/htdocs/js/bootstrap-native.js dist/htdocs/js/bootstrap-native.min.js
  minify js dist/htdocs/js/mympd.js dist/htdocs/js/mympd.min.js
  
  echo "Combining and compressing javascript"
  JSFILES="dist/htdocs/js/*.min.js"
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
  CSSFILES="dist/htdocs/css/*.min.css"
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
  return $ASSETSCHANGED
}

buildrelease() {
  if [ -f .git/HEAD ] && grep -q "devel" .git/HEAD
  then
  	echo "In devel branch, running cleanupdist"
  	cleanupdist
  fi

  if createdistfiles
  then
  	ASSETSCHANGED="0"
  else
    ASSETSCHANGED="1"
  fi

  echo "Compiling myMPD"
  install -d release
  cd release || exit 1
  if [ "$ASSETSCHANGED" = "1" ]
  then
    echo "Assets changed"
    #force rebuild of web_server with embedded assets
    rm -vf CMakeFiles/mympd.dir/src/web_server/web_server_utility.c.o
  else
    echo "Assets not changed"
  fi
  #set INSTALL_PREFIX and build myMPD
  export INSTALL_PREFIX="${MYMPD_INSTALL_PREFIX:-/usr}"
  cmake -DCMAKE_INSTALL_PREFIX:PATH="$INSTALL_PREFIX" -DCMAKE_BUILD_TYPE=RELEASE \
  	-DENABLE_SSL="$ENABLE_SSL" -DENABLE_LIBID3TAG="$ENABLE_LIBID3TAG" \
  	-DENABLE_FLAC="$ENABLE_FLAC" -DENABLE_LUA="$ENABLE_LUA" ..
  make
}

addmympduser() {
  echo "Checking status of mympd system user and group"
  if ! getent group mympd > /dev/null
  then
    if check_cmd groupadd
    then
      groupadd -r mympd
    elif check_cmd addgroup
    then
      #alpine
      addgroup -S mympd 2>/dev/null
    else
      echo "Can not add group mympd"
      return 1
    fi
  fi

  if ! getent passwd mympd > /dev/null
  then
    if check_cmd useradd
    then
      useradd -r -g mympd -s /bin/false -d /var/lib/mympd mympd
    elif check_cmd adduser
    then
      #alpine
      adduser -S -D -H -h /var/lib/mympd -s /sbin/nologin -G mympd -g myMPD mympd
    else
      echo "Can not add user mympd"
      return 1
    fi
  fi
  return 0
}

installrelease() {
  echo "Installing myMPD"
  cd release || exit 1  
  make install DESTDIR="$DESTDIR"
  addmympduser
  echo "myMPD installed"
  echo "Modify mympd.conf to suit your needs or use the"
  echo "mympd-config tool to generate a valid mympd.conf automatically."
}

builddebug() {
  MEMCHECK=$1

  echo "Linking bootstrap css and js"
  [ -e "$PWD/htdocs/css/bootstrap.css" ] || ln -s "$PWD/dist/htdocs/css/bootstrap.css" "$PWD/htdocs/css/bootstrap.css"
  [ -e "$PWD/htdocs/js/bootstrap-native.js" ] || ln -s "$PWD/dist/htdocs/js/bootstrap-native.js" "$PWD/htdocs/js/bootstrap-native.js"
  [ -e "$PWD/htdocs/js/long-press-event.min.js" ] || ln -s "$PWD/dist/htdocs/js/long-press-event.min.js" "$PWD/htdocs/js/long-press-event.min.js"

  createi18n ../../htdocs/js/i18n.js pretty
  
  echo "Compiling myMPD"
  install -d debug
  cd debug || exit 1
  cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr -DCMAKE_BUILD_TYPE=DEBUG -DMEMCHECK="$MEMCHECK" \
  	-DENABLE_SSL="$ENABLE_SSL" -DENABLE_LIBID3TAG="$ENABLE_LIBID3TAG" -DENABLE_FLAC="$ENABLE_FLAC" \
  	-DENABLE_LUA="$ENABLE_LUA" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
  make VERBOSE=1
  echo "Linking compilation database"
  sed -e 's/\\t/ /g' -e 's/-Wformat-overflow=2//g' -e 's/-fsanitize=bounds-strict//g' -e 's/-static-libasan//g' compile_commands.json > ../src/compile_commands.json
}

buildtest() {
  install -d test/build
  cd test/build || exit 1
  cmake ..
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
  rm -rf test/build
  
  #htdocs
  rm -f htdocs/js/bootstrap-native-v4.js
  rm -f htdocs/js/bootstrap-native.js
  rm -f htdocs/js/long-press-event.min.js
  rm -f htdocs/js/i18n.js
  rm -f htdocs/css/bootstrap.css

  #tmp files
  find ./ -name \*~ -delete
  
  #compilation database
  rm -f src/compile_commands.json
  #clang tidy
  rm -f clang-tidy.out
}

cleanuposc() {
  rm -rf osc
}

cleanupdist() {
  rm -f dist/htdocs/js/i18n.min.js
  rm -f dist/htdocs/js/keymap.min.js 
  rm -f dist/htdocs/js/bootstrap-native-v4.min.js 
  rm -f dist/htdocs/js/bootstrap-native.min.js 
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

check() {
  if check_cmd cppcheck
  then
    echo "Running cppcheck"
    [ -z "${CPPCHECKOPTS+z}" ] && CPPCHECKOPTS="--enable=warning"
    cppcheck $CPPCHECKOPTS src/*.c src/*.h
    cppcheck $CPPCHECKOPTS src/mpd_client/*.c src/mpd_client/*.h
    cppcheck $CPPCHECKOPTS src/mympd_api/*.c src/mympd_api/*.h
    cppcheck $CPPCHECKOPTS src/web_server/*.c src/web_server/*.h
    cppcheck $CPPCHECKOPTS cli_tools/*.c
  else
    echo "cppcheck not found"
  fi
  
  if check_cmd flawfinder
  then
    echo "Running flawfinder"
    [ -z "${FLAWFINDEROPTS+z}" ] && FLAWFINDEROPTS="-m3"
    flawfinder $FLAWFINDEROPTS src
    flawfinder $FLAWFINDEROPTS cli_tools
  else
    echo "flawfinder not found"
  fi

  if [ ! -f src/compile_commands.json ]
  then
    echo "src/compile_commands.json not found"
    echo "run: ./build.sh debug"
    exit 1
  fi
  
  if check_cmd clang-tidy
  then
    echo "Running clang-tidy, output goes to clang-tidy.out"
    rm -f clang-tidy.out
    cd src || exit 1
    find ./ -name '*.c' -exec clang-tidy \
    	--checks="*,-google-readability-todo,-llvmlibc-restrict-system-libc-headers,-bugprone-reserved-identifier,-cert-dcl37-c,-cert-dcl51-cpp,-readability-isolate-declaration,-hicpp-multiway-paths-covered,-readability-uppercase-literal-suffix,-hicpp-uppercase-literal-suffix,-cert-msc51-cpp,-cert-msc32-c,-hicpp-no-assembler,-android*,-cert-env33-c,-cert-msc50-cpp,-bugprone-branch-clone,-misc-misplaced-const,-readability-non-const-parameter,-cert-msc30-c,-hicpp-signed-bitwise,-readability-magic-numbers,-readability-avoid-const-params-in-decls,-llvm-include-order,-bugprone-macro-parentheses,-modernize*,-cppcoreguidelines*,-llvm-header-guard,-clang-analyzer-optin.performance.Padding,-clang-diagnostic-embedded-directive" \
    	-header-filter='.*' {}  \; >> ../clang-tidy.out
  else
    echo "clang-tidy not found"  
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
    [ "$F" = "$STARTPATH/builder" ] && continue
    cp -a "$F" .
  done
  rm -r dist/buildtools
}

pkgdebian() {
  check_cmd dpkg-buildpackage
  prepare
  cp -a contrib/packaging/debian .
  export LC_TIME="en_GB.UTF-8"
  tar -czf "../mympd_${VERSION}.orig.tar.gz" -- *

  SIGNOPT="--no-sign"
  if [ ! -z "${SIGN+x}" ] && [ "$SIGN" = "TRUE" ]
  then
	SIGNOPT="--sign-key=$GPGKEYID"  
  else
    echo "Package would not be signed"
  fi
  #shellcheck disable=SC2086
  dpkg-buildpackage -rfakeroot $SIGNOPT

  #get created package name
  PACKAGE=$(ls ../mympd_"${VERSION}"-1_*.deb)
  if [ "$PACKAGE" = "" ]
  then
    echo "Can't find package"
  fi

  if check_cmd lintian
  then
    echo "Checking package with lintian"
    lintian "$PACKAGE"
  else
    echo "WARNING: lintian not found, can't check package"
  fi
}

pkgdocker() {
  check_cmd docker
  [ "$DOCKERFILE" = "" ] && DOCKERFILE="Dockerfile.alpine"
  prepare
  cp contrib/packaging/docker/"$DOCKERFILE" Dockerfile
  docker build -t mympd .
}

pkgbuildx() {
  check_cmd docker
  if [ ! -x ~/.docker/cli-plugins/docker-buildx ]
  then
    echo "Docker buildx not found"
    echo "Quick start:"
    echo "  1. Download plugin: https://github.com/docker/buildx/releases/latest"
    echo "  2. Save it to file: ~/.docker/cli-plugins/docker-buildx"
    echo "  3. Make it executeable: chmod +x ~/.docker/cli-plugins/docker-buildx"
    echo ""
    echo "More info: https://www.docker.com/blog/getting-started-with-docker-for-arm-on-linux/"
    exit 1
  fi
  [ "$DOCKERFILE" = "" ] && DOCKERFILE="Dockerfile.alpine"
  [ "$PLATFORMS" = "" ] && PLATFORMS="linux/amd64,linux/arm64,linux/arm/v7,linux/arm/v6"
  prepare
  cp contrib/packaging/docker/"$DOCKERFILE" Dockerfile
  docker run --rm --privileged docker/binfmt:820fdd95a9972a5308930a2bdfb8573dd4447ad3
  docker buildx create --name mympdbuilder
  docker buildx use mympdbuilder
  docker buildx inspect --bootstrap
  docker buildx build -t mympd --platform "$PLATFORMS" .
}

pkgalpine() {
  check_cmd abuild
  prepare
  tar -czf "mympd_${VERSION}.orig.tar.gz" -- *
  [ "$1" = "taronly" ] && return 0
  cp contrib/packaging/alpine/* .
  abuild checksum
  abuild -r
}

pkgrpm() {
  check_cmd rpmbuild
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
  if check_cmd rpmlint
  then
    echo "Checking package with rpmlint"
    ARCH=$(uname -p)
    rpmlint "$HOME/rpmbuild/RPMS/${ARCH}/mympd-${VERSION}-0.${ARCH}.rpm"
  else
    echo "WARNING: rpmlint not found, can't check package"
  fi
}

pkgarch() {
  check_cmd makepkg
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
  if check_cmd namcap
  then
    echo "Checking package with namcap"
    namcap PKGBUILD
    namcap mympd-*.pkg.tar.xz
  else
    echo "WARNING: namcap not found, can't check package"
  fi
}

pkgosc() {
  check_cmd osc
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
  echo "Platform: $(uname -m)"
  if [ -f /etc/debian_version ]
  then
    #debian
    JAVADEB="default-jre-headless"
    #issue 234
    [ "$(uname -m)" = "armv6l" ] && JAVADEB="openjdk-8-jre-headless"
    apt-get update
    apt-get install -y --no-install-recommends \
	gcc cmake perl libssl-dev libid3tag0-dev libflac-dev \
	build-essential liblua5.3-dev pkg-config $JAVADEB \
	libpcre3-dev
  elif [ -f /etc/arch-release ]
  then
    #arch
    pacman -S gcc cmake perl openssl libid3tag flac jre-openjdk-headless lua pkgconf pcre
  elif [ -f /etc/alpine-release ]
  then
    #alpine
    JAVADEB="openjdk11-jre-headless"
    #issue 234
    [ "$(uname -m)" = "armv7l" ] && JAVADEB="java-common"
    apk add cmake perl openssl-dev libid3tag-dev flac-dev lua5.3-dev \
    	alpine-sdk linux-headers pkgconf $JAVADEB pcre-dev
  elif [ -f /etc/SuSE-release ]
  then
    #suse
    zypper install gcc cmake pkgconfig perl openssl-devel libid3tag-devel flac-devel \
	lua-devel java-11-openjdk-headless unzip pcre-devel
  elif [ -f /etc/redhat-release ]
  then  
    #fedora 	
    yum install gcc cmake pkgconfig perl openssl-devel libid3tag-devel flac-devel \
	lua-devel java-11-openjdk-headless unzip pcre-devel
  else 
    echo "Unsupported distribution detected."
    echo "You should manually install:"
    echo "  - gcc"
    echo "  - cmake"
    echo "  - perl"
    echo "  - java"
    echo "  - openssl (devel)"
    echo "  - flac (devel)"
    echo "  - libid3tag (devel)"
    echo "  - lua53 (devel)"
    echo "  - libpcre3 (devel)"
  fi
}

updatelibmympdclient() {
  check_cmd git meson

  cd dist/src/libmpdclient || exit 1
  STARTDIR=$(pwd)

  TMPDIR=$(mktemp -d)
  cd "$TMPDIR" || exit 1
  git clone -b libmympdclient https://github.com/jcorporation/libmpdclient.git
  cd libmpdclient || exit 1
  meson . output

  cd "$STARTDIR" || exit 1
  install -d src
  install -d include/mpd/

  rsync -av --delete "$TMPDIR/libmpdclient/src/" ./src/
  rsync -av --delete "$TMPDIR/libmpdclient/include/mpd/" ./include/mpd/

  rsync -av "$TMPDIR/libmpdclient/output/version.h" include/mpd/version.h
  rsync -av "$TMPDIR/libmpdclient/output/config.h" include/config.h

  rsync -av "$TMPDIR/libmpdclient/COPYING" COPYING
  rsync -av "$TMPDIR/libmpdclient/AUTHORS" AUTHORS

  rm -rf "$TMPDIR"
}

# Also deletes stale installations in other locations.
#
uninstall() {
  # cmake does not provide an uninstall target,
  # instead its manifest is of use at least for
  # the binaries
  if [ -f release/install_manifest.txt ]
  then
    xargs rm < release/install_manifest.txt
  fi

  #MYMPD_INSTALL_PREFIX="/usr"
  rm -f "$DESTDIR/usr/bin/mympd"
  rm -f "$DESTDIR/usr/bin/mympd-config"
  rm -f "$DESTDIR/usr/bin/mympd-script"
  rm -f "$DESTDIR/usr/share/man/man1/mympd.1.gz"
  rm -f "$DESTDIR/usr/share/man/man1/mympd-config.1.gz"
  rm -f "$DESTDIR/usr/share/man/man1/mympd-script.1.gz"
  #MYMPD_INSTALL_PREFIX="/usr/local"
  rm -f "$DESTDIR/usr/local/bin/mympd"
  rm -f "$DESTDIR/usr/local/bin/mympd-config"
  rm -f "$DESTDIR/usr/local/bin/mympd-script"
  rm -f "$DESTDIR/usr/local/share/man/man1/mympd.1.gz"
  rm -f "$DESTDIR/usr/local/share/man/man1/mympd-config.1.gz"
  rm -f "$DESTDIR/usr/local/share/man/man1/mympd-script.1.gz"
  #MYMPD_INSTALL_PREFIX="/opt/mympd/"
  rm -rf "$DESTDIR/opt/mympd"
  #systemd
  rm -f "$DESTDIR/usr/lib/systemd/system/mympd.service"
  rm -f "$DESTDIR/lib/systemd/system/mympd.service"
  #sysVinit, open-rc
  if [ -z "$DESTDIR" ] && [ -f "/etc/init.d/mympd" ]
  then
    echo "SysVinit / OpenRC-script /etc/init.d/mympd found."
    echo "Make sure it isn't part of any runlevel and delete by yourself"
    echo "or invoke with purge instead of uninstall."
  fi
}

purge() {
  #MYMPD_INSTALL_PREFIX="/usr"
  rm -rf "$DESTDIR/var/lib/mympd"
  rm -f "$DESTDIR/etc/mympd.conf"
  rm -f "$DESTDIR/etc/mympd.conf.dist"
  rm -f "$DESTDIR/etc/init.d/mympd"
  #MYMPD_INSTALL_PREFIX="/usr/local"
  rm -f "$DESTDIR/usr/local/etc/mympd.conf"
  rm -f "$DESTDIR/usr/local/etc/mympd.conf.dist"
  #MYMPD_INSTALL_PREFIX="/opt/mympd/"
  rm -rf "$DESTDIR/var/opt/mympd"
  rm -rf "$DESTDIR/etc/opt/mympd"
  #arch
  rm -rf "$DESTDIR/etc/webapps/mympd"
  #remove user
  getent passwd mympd > /dev/null && userdel mympd
  getent group mympd > /dev/null && groupdel -f mympd
}

translate() {
  cd src/i18n || exit 1
  perl ./tojson.pl pretty > ../../htdocs/js/i18n.js
}

materialicons() {
  TMPDIR=$(mktemp -d)
  cd "$TMPDIR" || exit 1
  if ! wget -q https://raw.githubusercontent.com/google/material-design-icons/master/update/current_versions.json \
	-O current_version.json
  then
    echo "Error downloading json file"
    exit 1
  fi
  EXCLUDE="face_unlock|battery_\\d|battery_charging_\\d|signal_cellular_|signal_wifi_\\d_bar"
  printf "const materialIcons={" > "$STARTPATH/htdocs/js/ligatures.js"
  I=0
  #shellcheck disable=SC2013
  for CAT in $(grep "^\\s" current_version.json | cut -d\" -f2 | cut -d: -f1 | sort -u)
  do
    [ "$I" -gt 0 ] && printf "," >> "$STARTPATH/htdocs/js/ligatures.js"
    printf "\"%s\": [" "$CAT" >> "$STARTPATH/htdocs/js/ligatures.js"
	J=0
	#shellcheck disable=SC2013
	for MI in $(cut -d\" -f2 current_version.json | grep "$CAT::" | cut -d: -f3 | grep -v -P "$EXCLUDE")
	do
	  [ "$J" -gt 0 ] && printf "," >> "$STARTPATH/htdocs/js/ligatures.js"
	  printf "\"%s\"" "$MI" >> "$STARTPATH/htdocs/js/ligatures.js"
	  J=$((J+1))	
	done
	printf "]" >> "$STARTPATH/htdocs/js/ligatures.js"
	I=$((I+1))
  done 
  printf "};\\n"  >> "$STARTPATH/htdocs/js/ligatures.js"
  cd / || exit 1
  rm -fr "$TMPDIR"
}

sbuild_chroots() {
  if [ "$(id -u)" != "0" ]
  then
    echo "Must be run as root: "
    echo "  sudo -E ./build.sh sbuild_chroots"
  	exit 1
  fi
  [ -z "${WORKDIR+x}" ] && WORKDIR="$STARTPATH/builder"
  [ -z "${DISTROS+x}" ] && DISTROS="buster stretch"
  [ -z "${TARGETS+x}" ] && TARGETS="armhf armel"
  [ -z "${DEBIAN_MIRROR+x}" ] && DEBIAN_MIRROR="http://ftp.de.debian.org/debian"

  check_cmd sbuild qemu-debootstrap

  mkdir -p "${WORKDIR}/chroot"
  mkdir -p "${WORKDIR}/cache"

  for DIST in ${DISTROS}
  do
    for ARCH in ${TARGETS}
    do
      CHROOT="${DIST}-${ARCH}"
      echo "Creating chroot for $CHROOT"
      [ -d "${WORKDIR}/chroot/${CHROOT}" ] && echo "chroot ${CHROOT} already exists... skipping." && continue
      qemu-debootstrap --arch="${ARCH}" --variant=buildd --cache-dir="${WORKDIR}/cache" --include=fakeroot,build-essential "${DIST}" "${WORKDIR}/chroot/${CHROOT}/" "${DEBIAN_MIRROR}"

      grep "${CHROOT}" /etc/schroot/schroot.conf || cat << EOF >> /etc/schroot/schroot.conf

[${CHROOT}]
description=Debian ${DIST} ${ARCH}
directory=${WORKDIR}/chroot/${CHROOT}
groups=sbuild-security
EOF
    done
  done
}

sbuild_build() {
  if [ "$(id -u)" != "0" ]
  then
    echo "Must be run as root: "
    echo "  sudo -E ./build.sh sbuild_build"
  	exit 1
  fi
  [ -z "${WORKDIR+x}" ] && WORKDIR="$STARTPATH/builder"
  [ -z "${DISTROS+x}" ] && DISTROS="buster stretch"
  [ -z "${TARGETS+x}" ] && TARGETS="armhf armel"

  check_cmd sbuild qemu-debootstrap

  prepare
  cp -a contrib/packaging/debian .
  export LC_TIME="en_GB.UTF-8"
  tar -czf "../mympd_${VERSION}.orig.tar.gz" -- *
  cd ..
  # Compile for target distro/arch
  for DIST in ${DISTROS}
  do
    for ARCH in ${TARGETS}
    do
      CHROOT="${DIST}-${ARCH}"
      echo "Building ${DIST} for ${ARCH}"
      mkdir -p "${WORKDIR}/builds/${CHROOT}"
      sbuild --arch="${ARCH}" -d unstable --chroot="${CHROOT}" build --build-dir="${WORKDIR}/builds/${CHROOT}"
    done
  done
}

sbuild_cleanup() {
  if [ "$(id -u)" != "0" ]
  then
    echo "Must be run as root: "
    echo "  sudo -E ./build.sh sbuild_cleanup"
  	exit 1
  fi
  [ -z "${WORKDIR+x}" ] && WORKDIR="$STARTPATH/builder"
  rm -rf package "${WORKDIR}"
}

case "$ACTION" in
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
	test)
	  buildtest
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
	pkgbuildx)
	  pkgbuildx
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
	libmympdclient)
	  updatelibmympdclient
	;;
	uninstall)
	  uninstall
	;;
	purge)
	  uninstall
	  purge
	;;
	translate)
	  translate
	;;
	materialicons)
		materialicons
	;;
	createdist)
	  createdistfiles
	;;
	sbuild_chroots)
		sbuild_chroots
	;;
	sbuild_build)
		sbuild_build
	;;
	sbuild_cleanup)
		sbuild_cleanup
	;;
	*)
	  echo "Usage: $0 <option>"
	  echo "Version: ${VERSION}"
	  echo ""
	  echo "Build options:"
	  echo "  release:          build release files in directory release"
	  echo "  install:          installs release files from directory release"
	  echo "                    following environment variables are respected"
	  echo "                      - DESTDIR=\"\""
	  echo "  releaseinstall:   calls release and install afterwards"
	  echo "  debug:            builds debug files in directory debug,"
	  echo "                    linked with libasan3, uses assets in htdocs"
	  echo "  memcheck:         builds debug files in directory debug"
	  echo "                    for use with valgrind, uses assets in htdocs"
	  echo "  check:            runs cppcheck and flawfinder on source files"
	  echo "                    following environment variables are respected"
	  echo "                      - CPPCHECKOPTS=\"--enable=warning\""
	  echo "                      - FLAWFINDEROPTS=\"-m3\""
	  echo "  test:             builds the unit testing files in test/build"
	  echo "  installdeps:      installs build and run dependencies"
	  echo "  translate:        builds the translation file for debug builds"
	  echo "  createdist:       creates the minfied and compressed dist files"
	  echo ""
	  echo "Cleanup options:"
	  echo "  cleanup:          cleanup source tree"
	  echo "  cleanupdist:      cleanup dist directory, forces release to build new assets"
	  echo "  cleanupoldinst:   removes deprecated files"
	  echo "  uninstall:        removes myMPD files, leaves configuration and "
	  echo "                    state files in place"
	  echo "                    following environment variables are respected"
	  echo "                      - DESTDIR=\"\""
	  echo "  purge:            removes all myMPD files, also your init scripts"
	  echo "                    following environment variables are respected"
	  echo "                      - DESTDIR=\"\""
	  echo ""
	  echo "Packaging options:"
	  echo "  pkgalpine:        creates the alpine package"
	  echo "  pkgarch:          creates the arch package"
	  echo "                    following environment variables are respected"
	  echo "                      - SIGN=\"FALSE\""
	  echo "                      - GPGKEYID=\"\""
	  echo "  pkgdebian:        creates the debian package"
	  echo "                    following environment variables are respected"
	  echo "                      - SIGN=\"FALSE\""
	  echo "                      - GPGKEYID=\"\""
	  echo "  pkgdocker:        creates the docker image (debian based)"
      echo "                    following environment variables are respected"
      echo "                      - DOCKERFILE=\"Dockerfile.alpine\""
      echo "  pkgbuildx:        creates a multiarch docker image with buildx"
      echo "                    following environment variables are respected"
      echo "                      - DOCKERFILE=\"Dockerfile.alpine\""
      echo "                      - PLATFORMS=\"linux/amd64,linux/arm64,linux/arm/v7,linux/arm/v6\""
	  echo "  pkgrpm:           creates the rpm package"
	  echo "  pkgosc:           updates the open build service repository"
	  echo "                    following environment variables are respected"
	  echo "                      - OSC_REPO=\"home:jcorporation/myMPD\""
	  echo "  sbuild_chroots:   creates chroots for debian crosscompile"
	  echo "                    must be run as root"
	  echo "                    following environment variables are respected"
      echo "                      - WORKDIR=\"$STARTPATH/builder\""
      echo "                      - DISTROS=\"buster stretch\""
      echo "                      - TARGETS=\"armhf armel\""
      echo "                      - DEBIAN_MIRROR=\"http://ftp.de.debian.org/debian\""
	  echo "  sbuild_build:     builds the packages for targets and distros"
	  echo "                    must be run as root"
	  echo "                    following environment variables are respected"
      echo "                      - WORKDIR=\"$STARTPATH/builder\""
      echo "                      - DISTROS=\"buster stretch\""
      echo "                      - TARGETS=\"armhf armel\""
	  echo "  sbuild_cleanup:   removes the builder and package directory"
	  echo "                    must be run as root"
	  echo ""
	  echo "Misc options:"
	  echo "  setversion:       sets version and date in packaging files from CMakeLists.txt"
	  echo "  addmympduser:     adds mympd group and user"
	  echo "  libmympdclient:   updates libmpdclient"
	  echo ""
	  echo "Environment variables for building"
	  echo "  - MYMPD_INSTALL_PREFIX=\"/usr\""
	  echo "  - ENABLE_SSL=\"ON\""
	  echo "  - ENABLE_LIBID3TAG=\"ON\""
	  echo "  - ENABLE_FLAC=\"ON\""
	  echo "  - ENABLE_LUA=\"ON\""
	  echo "  - MANPAGES=\"ON\""
	  echo ""
	;;
esac
