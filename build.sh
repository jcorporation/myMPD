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
    if ! check_cmd_silent "$@"
    then
      echo "ERROR: ${DEPENDENCY} not found"
      return 1
    fi
  done
  return 0
}

check_cmd_silent() {
  for DEPENDENCY in "$@"
  do
    if ! command -v "${DEPENDENCY}" > /dev/null
    then
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
  check_cmd gzip perl

  GZIP="gzip -n -f -v -9"
  GZIPCAT="gzip -n -v -9 -c"
fi

setversion() {
  check_cmd bzcat
  TS=$(stat -c%Y CMakeLists.txt)
  export LC_TIME="en_GB.UTF-8"
  DATE_F1=$(date --date=@"${TS}" +"%a %b %d %Y")
  DATE_F2=$(date --date=@"${TS}" +"%a, %d %b %Y %H:%m:%S %z")
  DATE_F3=$(date --date=@"${TS}" +"%d %b %Y")
  echo "Setting version to ${VERSION} and date to ${DATE_F2}"

  for F in htdocs/sw.js contrib/packaging/alpine/APKBUILD contrib/packaging/arch/PKGBUILD \
  		   contrib/packaging/rpm/mympd.spec contrib/packaging/debian/changelog contrib/man/mympd.1 \
  		   contrib/man/mympd-config.1 contrib/man/mympd-script.1
  do
  	echo "$F"
  	sed -e "s/__VERSION__/${VERSION}/g" -e "s/__DATE_F1__/$DATE_F1/g" -e "s/__DATE_F2__/$DATE_F2/g" \
  	  	-e "s/__DATE_F3__/$DATE_F3/g" "$F.in" > "$F"
  done
  #compress manpages
  for F in contrib/man/mympd.1 contrib/man/mympd-config.1 contrib/man/mympd-script.1
  do
    $GZIPCAT "$F" > "$F.gz"
  done

  #gentoo ebuild must be moved only
  [ -f "contrib/packaging/gentoo/media-sound/mympd/mympd-${VERSION}.ebuild" ] || \
  	mv -f contrib/packaging/gentoo/media-sound/mympd/mympd-*.ebuild "contrib/packaging/gentoo/media-sound/mympd/mympd-${VERSION}.ebuild"

  echo "var myMPDversion = '${VERSION}';" > htdocs/js/version.js
}

minify() {
  TYPE="$1"
  SRC="$2"
  DST="$3"

  #We remove only line-breaks, comments and truncate spaces of lines
  echo "Minifying $SRC"
  if [ "$TYPE" = "html" ]
  then
    #shellcheck disable=SC2016
    if ! perl -pe 's/^<!--debug-->.*\n//gm; s/<!--release\s+(.+)-->/$1/g; s/<!--(.+)-->//g; s/^\s*//gm; s/\s*$//gm' "$SRC" > "${DST}.tmp"
    then
      rm -f "${DST}.tmp"
      echo "Error minifying $SRC"
      exit 1
    fi
  elif [ "$TYPE" = "js" ]
  then
    #shellcheck disable=SC2016
    if ! perl -pe 's/^\s*//gm; s/^\/\/.+$//g; s/^logDebug\(.*$//g; s/\s*$//gm;' "$SRC" > "${DST}.tmp"
    then
      rm -f "${DST}.tmp"
      echo "Error minifying $SRC"
      exit 1
    fi
  elif [ "$TYPE" = "css" ]
  then
    #shellcheck disable=SC2016
    if ! perl -pe 's/^\s*//gm; s/\s*$//gm; s/: /:/g;' "$SRC" | perl -pe 's/\/\*[^*]+\*\///g;' > "${DST}.tmp"
    then
      rm -f "${DST}.tmp"
      echo "Error minifying $SRC"
      exit 1
    fi
  fi

  #successfull minified file
  echo "" >> "${DST}.tmp"
  mv "${DST}.tmp" "$DST"
  return 0
}

createi18n() {
  DST=$1
  PRETTY=$2
  cd src/i18n || exit 1
  echo "Creating i18n json"
  perl ./tojson.pl "$PRETTY" > "$DST"
  cd ../.. || exit 1
}

createassets() {
  echo "Creating assets"
  install -d release/htdocs/js
  install -d release/htdocs/css
  install -d release/htdocs/assets

  createi18n ../../release/htdocs/js/i18n.min.js ""
  
  echo "Minifying javascript"
  JSSRCFILES=""
  for F in htdocs/js/*.js
  do
    [ "$F" = "htdocs/js/i18n.js" ] && continue
    [ "$F" = "htdocs/js/bootstrap-native.js" ] && continue
    [ "$F" = "htdocs/js/long-press-event.js" ] && continue
    [ -L "$F" ] || JSSRCFILES="$JSSRCFILES $F"
    if tail -1 "$F" | perl -npe 'exit 1 if m/\n/; exit 0'
    then
      echo "ERROR: $F don't end with newline character"
      exit 1
    fi
  done
  echo "Creating mympd.js"
  # shellcheck disable=SC2086
  # shellcheck disable=SC2002
  cat $JSSRCFILES | grep -v "\"use strict\";" > release/htdocs/js/mympd.js
  minify js htdocs/sw.js release/htdocs/sw.min.js
  minify js htdocs/js/keymap.js release/htdocs/js/keymap.min.js
  minify js release/htdocs/js/mympd.js release/htdocs/js/mympd.min.js
  
  echo "Combining and compressing javascript"
  JSFILES="dist/htdocs/js/*.min.js release/htdocs/js/*.min.js"
  for F in $JSFILES
  do
    if tail -1 "$F" | perl -npe 'exit 1 if m/\n/; exit 0'
    then
      echo "ERROR: $F don't end with newline character"
      exit 1
    fi
  done
  echo "\"use strict\";" > release/htdocs/js/combined.js
  # shellcheck disable=SC2086
  # shellcheck disable=SC2002
  cat $JSFILES >> release/htdocs/js/combined.js
  $GZIP release/htdocs/js/combined.js
  
  #serviceworker
  $GZIPCAT release/htdocs/sw.min.js > release/htdocs/sw.js.gz
 
  echo "Minifying stylesheets"
  for F in htdocs/css/*.css
  do
    DST=$(basename "$F" .css)
    [ -L "$F" ] || minify css "$F" "release/htdocs/css/${DST}.min.css"
  done
  
  echo "Combining and compressing stylesheets"
  CSSFILES="dist/htdocs/css/*.min.css release/htdocs/css/*.min.css"
  # shellcheck disable=SC2086
  cat $CSSFILES > release/htdocs/css/combined.css
  $GZIP release/htdocs/css/combined.css
  
  echo "Minifying and compressing html"
  minify html htdocs/index.html release/htdocs/index.html
  $GZIPCAT release/htdocs/index.html > release/htdocs/index.html.gz

  echo "Creating other compressed assets"
  ASSETS="htdocs/mympd.webmanifest htdocs/assets/*.svg"
  for ASSET in $ASSETS
  do
    $GZIPCAT "$ASSET" > "release/${ASSET}.gz"
  done
  return 0
}

buildrelease() {
  createassets

  echo "Compiling myMPD"
  install -d release
  cd release || exit 1
  #force rebuild of web_server with embedded assets
  rm -vf CMakeFiles/mympd.dir/src/web_server/web_server_utility.c.o
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
    if check_cmd_silent groupadd
    then
      groupadd -r mympd
    elif check_cmd_silent addgroup
    then
      #alpine
      addgroup -S mympd
    else
      echo "Can not add group mympd"
      return 1
    fi
  fi

  if ! getent passwd mympd > /dev/null
  then
    if check_cmd_silent useradd
    then
      useradd -r -g mympd -s /bin/false -d /var/lib/mympd mympd
    elif check_cmd_silent adduser
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
  [ -z "${DESTDIR+x}" ] && DESTDIR=""
  make install DESTDIR="$DESTDIR"
  addmympduser
  echo "myMPD installed"
  echo "Modify mympd.conf to suit your needs or use the"
  echo "mympd-config tool to generate a valid mympd.conf automatically."
}

builddebug() {
  MEMCHECK=$1

  echo "Linking dist assets"
  ln -f "$PWD/dist/htdocs/css/bootstrap.css" "$PWD/htdocs/css/bootstrap.css"
  ln -f "$PWD/dist/htdocs/js/bootstrap-native.js" "$PWD/htdocs/js/bootstrap-native.js"
  ln -f "$PWD/dist/htdocs/js/long-press-event.js" "$PWD/htdocs/js/long-press-event.js"
  ln -f "$PWD/dist/htdocs/assets/MaterialIcons-Regular.woff2" "$PWD/htdocs/assets/MaterialIcons-Regular.woff2"

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

cleanup() {
  #build directories
  rm -rf release
  rm -rf debug
  rm -rf package
  rm -rf test/build
  
  #htdocs
  rm -f htdocs/js/bootstrap-native.js
  rm -f htdocs/js/long-press-event.js
  rm -f htdocs/js/i18n.js
  rm -f htdocs/css/bootstrap.css
  rm -f htdocs/assets/MaterialIcons-Regular.woff2

  #compilation database
  rm -f src/compile_commands.json

  #clang tidy
  rm -f clang-tidy.out
}

cleanuposc() {
  rm -rf osc
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
}

pkgdebian() {
  check_cmd dpkg-buildpackage
  prepare
  cp -a contrib/packaging/debian .
  export LC_TIME="en_GB.UTF-8"
  tar -czf "../mympd_${VERSION}.orig.tar.gz" -- *

  SIGNOPT="--no-sign"
  if [ -n "${SIGN+x}" ] && [ "$SIGN" = "TRUE" ]
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
  [ -z "${DOCKERFILE+x}" ] && DOCKERFILE="Dockerfile.alpine"
  prepare
  docker build -t mympd -f "contrib/packaging/docker/$DOCKERFILE" .
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
  [ -z "${DOCKERFILE+x}" ] && DOCKERFILE="Dockerfile.alpine"
  [ -z "${PLATFORMS+x}" ] && PLATFORMS="linux/amd64,linux/arm64,linux/arm/v7,linux/arm/v6"
  prepare
  cp contrib/packaging/docker/"$DOCKERFILE" Dockerfile
  docker run --rm --privileged docker/binfmt:820fdd95a9972a5308930a2bdfb8573dd4447ad3
  if ! docker buildx ls | grep -q mympdbuilder
  then
    docker buildx create --name mympdbuilder
  fi
  docker buildx use mympdbuilder
  docker buildx inspect --bootstrap
  docker buildx build -t mympd --platform "$PLATFORMS" .
}

pkgalpine() {
  if [ -z "${1+x}" ]
  then
    TARONLY=""
  else
    TARONLY=$1
  fi
  check_cmd abuild
  prepare
  tar -czf "mympd_${VERSION}.orig.tar.gz" -- *
  [ "$TARONLY" = "taronly" ] && return 0
  cp contrib/packaging/alpine/* .
  abuild checksum
  abuild -r
}

pkgrpm() {
  if [ -z "${1+x}" ]
  then
    TARONLY=""
  else
    TARONLY=$1
  fi
  check_cmd rpmbuild
  prepare
  SRC=$(ls)
  mkdir "mympd-${VERSION}"
  for F in $SRC
  do
    mv "$F" "mympd-${VERSION}"
  done
  tar -czf "mympd-${VERSION}.tar.gz" "mympd-${VERSION}"
  [ "$TARONLY" = "taronly" ] && return 0
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
  if [ -n "${SIGN+x}" ] && [ "$SIGN" = "TRUE" ]
  then
    KEYARG=""
    [ -z "${GPGKEYID+x}" ] || KEYARG="--key $PGPGKEYID"
    #shellcheck disable=SC2086
    makepkg --sign $KEYARG mympd-*.pkg.tar.xz
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
  if [ -z "${OSC_REPO+x}" ]
  then
    if [ -f .git/HEAD ] && grep -q "master" .git/HEAD
    then
  	  OSC_REPO="home:jcorporation/myMPD"
  	else
  	  OSC_REPO="home:jcorporation/myMPD-devel"
  	fi
  fi
  
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
    apt-get update
    apt-get install -y --no-install-recommends \
	gcc cmake perl libssl-dev libid3tag0-dev libflac-dev \
	build-essential liblua5.3-dev pkg-config libpcre3-dev
  elif [ -f /etc/arch-release ]
  then
    #arch
    pacman -S gcc cmake perl openssl libid3tag flac lua pkgconf pcre
  elif [ -f /etc/alpine-release ]
  then
    #alpine
    apk add cmake perl openssl-dev libid3tag-dev flac-dev lua5.3-dev \
    	alpine-sdk linux-headers pkgconf pcre-dev
  elif [ -f /etc/SuSE-release ]
  then
    #suse
    zypper install gcc cmake pkgconfig perl openssl-devel libid3tag-devel flac-devel \
	lua-devel unzip pcre-devel
  elif [ -f /etc/redhat-release ]
  then  
    #fedora 	
    yum install gcc cmake pkgconfig perl openssl-devel libid3tag-devel flac-devel \
	lua-devel unzip pcre-devel
  else 
    echo "Unsupported distribution detected."
    echo "You should manually install:"
    echo "  - gcc"
    echo "  - cmake"
    echo "  - perl"
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
  [ -z "${DESTDIR+x}" ] && DESTDIR=""
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
  [ -z "${DESTDIR+x}" ] && DESTDIR=""
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
  if getent passwd mympd > /dev/null
  then
    if check_cmd_silent userdel
    then
      userdel mympd
    elif check_cmd_silent deluser
    then
      #alpine
      deluser mympd
    else
      echo "Can not del user mygpiod"
      return 1
    fi
  fi
  #remove group
  if getent group mympd > /dev/null
  then
    if check_cmd_silent userdel
    then
      userdel mympd
    elif check_cmd_silent deluser
    then
      deluser mympd
    else
      echo "Can not del user mympd"
      return 1
    fi
  fi
}

transstatus() {
  TRANSOUT=$(./build.sh translate 2>&1)
  for F in src/i18n/*-*.txt
  do
    G=$(basename "$F" .txt)
    printf "%s: " "$G"
    echo "$TRANSOUT" | grep -c "$G not found"
  done
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
  #cleanup package dir
  rm -rf "$STARTPATH/package"
}

sbuild_cleanup() {
  if [ "$(id -u)" != "0" ]
  then
    echo "Must be run as root: "
    echo "  sudo -E ./build.sh sbuild_cleanup"
  	exit 1
  fi
  [ -z "${WORKDIR+x}" ] && WORKDIR="$STARTPATH/builder"
  rm -rf "${WORKDIR}"
}

run_eslint() {
  check_cmd eslint
  createassets
  for F in sw.js js/mympd.js
  do
    echo "Linting $F"
    eslint "htdocs/$F"
  done
}

run_stylelint() {
  check_cmd npx
  for F in mympd.css theme-light.css theme-dark.css
  do
    echo "Linting $F"
    npx stylelint "htdocs/css/$F"
  done
}

case "$ACTION" in
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
	transstatus)
	  transstatus
	;;
	materialicons)
		materialicons
	;;
	createassets)
	  createassets
	;;
	sbuild_chroots)
	  sbuild_chroots
	;;
	sbuild_build)
	  sbuild_build
	;;
	sbuild_cleanup)
	  sbuild_cleanup
	  exit 0
	;;
	eslint)
	  run_eslint
	;;
	stylelint)
	  run_stylelint
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
	  echo "  test:             builds the unit testing files in test/build"
	  echo "  installdeps:      installs build and run dependencies"
	  echo "  createassets:     creates the minfied and compressed dist files"
      echo ""
	  echo "Translation options:"
	  echo "  translate:        builds the translation file for debug builds"
	  echo "  transstatus:      shows the translation status"
	  echo ""
      echo "Test options:"
	  echo "  check:            runs cppcheck and flawfinder on source files"
	  echo "                    following environment variables are respected"
	  echo "                      - CPPCHECKOPTS=\"--enable=warning\""
	  echo "                      - FLAWFINDEROPTS=\"-m3\""
	  echo "  eslint:           combines javascript files and runs eslint"
	  echo "  stylelint:        runs stylelint"
	  echo ""
	  echo "Cleanup options:"
	  echo "  cleanup:          cleanup source tree"
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
	  echo "                    following environment variables are respected"
      echo "                      - WORKDIR=\"$STARTPATH/builder\""
	  echo ""
	  echo "Misc options:"
	  echo "  setversion:       sets version and date in packaging files from CMakeLists.txt"
	  echo "  addmympduser:     adds mympd group and user"
	  echo "  libmympdclient:   updates libmympdclient (fork of libmpdclient)"
	  echo ""
	  echo "Environment variables for building"
	  echo "  - MYMPD_INSTALL_PREFIX=\"/usr\""
	  echo "  - ENABLE_SSL=\"ON\""
	  echo "  - ENABLE_LIBID3TAG=\"ON\""
	  echo "  - ENABLE_FLAC=\"ON\""
	  echo "  - ENABLE_LUA=\"ON\""
	  echo "  - MANPAGES=\"ON\""
	  echo ""
	  exit 1
	;;
esac

exit 0
