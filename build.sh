#!/bin/sh
#
#SPDX-License-Identifier: GPL-3.0-or-later
#myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
#https://github.com/jcorporation/mympd

#exit on error
set -e

#exit on undefined variable
set -u

#print out commands
[ -z "${DEBUG+x}" ] || set -x

#get action
if [ -z "${1+x}" ]
then
  ACTION=""
else
  ACTION="$1"
fi

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

if [ -z "${EMBEDDED_ASSETS+x}" ]
then
  if [ "$ACTION" = "release" ]
  then
    export EMBEDDED_ASSETS="ON"
  else
    export EMBEDDED_ASSETS="OFF"
  fi
fi

if [ -z "${ENABLE_LIBASAN+x}" ]
then
  if [ "$ACTION" = "memcheck" ]
  then
    export ENABLE_LIBASAN="ON"
  else
    export ENABLE_LIBASAN="OFF"
  fi
fi

if [ -z "${ENABLE_IPV6+x}" ]
then
  export ENABLE_IPV6="ON"
fi

if [ -z "${EXTRA_CMAKE_OPTIONS+x}" ]
then
  export EXTRA_CMAKE_OPTIONS=""
fi

#colorful warnings and errors
echo_error() {
  printf "\e[0;31mERROR: "
  #shellcheck disable=SC2068
  echo $@
  printf "\e[m"
}

echo_warn() {
  printf "\e[1;33mWARN: "
  #shellcheck disable=SC2068
  echo $@
  printf "\e[m"
}

#clang tidy options
CLANG_TIDY_CHECKS="*"
CLANG_TIDY_CHECKS="$CLANG_TIDY_CHECKS,-altera-id-dependent-backward-branch"
CLANG_TIDY_CHECKS="$CLANG_TIDY_CHECKS,-altera-unroll-loops"
CLANG_TIDY_CHECKS="$CLANG_TIDY_CHECKS,-altera-struct-pack-align,-clang-analyzer-optin.performance.Padding"
CLANG_TIDY_CHECKS="$CLANG_TIDY_CHECKS,-bugprone-easily-swappable-parameters"
CLANG_TIDY_CHECKS="$CLANG_TIDY_CHECKS,-bugprone-macro-parentheses"
CLANG_TIDY_CHECKS="$CLANG_TIDY_CHECKS,-bugprone-reserved-identifier,-cert-dcl37-c,-cert-dcl51-cpp"
CLANG_TIDY_CHECKS="$CLANG_TIDY_CHECKS,-bugprone-signal-handler,-cert-sig30-c"
CLANG_TIDY_CHECKS="$CLANG_TIDY_CHECKS,-bugprone-integer-division"
CLANG_TIDY_CHECKS="$CLANG_TIDY_CHECKS,-clang-diagnostic-invalid-command-line-argument"
CLANG_TIDY_CHECKS="$CLANG_TIDY_CHECKS,-concurrency-mt-unsafe"
CLANG_TIDY_CHECKS="$CLANG_TIDY_CHECKS,-cppcoreguidelines*"
CLANG_TIDY_CHECKS="$CLANG_TIDY_CHECKS,-hicpp-*"
CLANG_TIDY_CHECKS="$CLANG_TIDY_CHECKS,-llvm-header-guard"
CLANG_TIDY_CHECKS="$CLANG_TIDY_CHECKS,-llvm-include-order"
CLANG_TIDY_CHECKS="$CLANG_TIDY_CHECKS,-llvmlibc-restrict-system-libc-headers"
CLANG_TIDY_CHECKS="$CLANG_TIDY_CHECKS,-misc-misplaced-const"
CLANG_TIDY_CHECKS="$CLANG_TIDY_CHECKS,-readability-function-cognitive-complexity,-google-readability-function-size,-readability-function-size"
CLANG_TIDY_CHECKS="$CLANG_TIDY_CHECKS,-readability-magic-numbers"
CLANG_TIDY_CHECKS="$CLANG_TIDY_CHECKS,-readability-avoid-const-params-in-decls"
CLANG_TIDY_CHECKS="$CLANG_TIDY_CHECKS,-readability-non-const-parameter"
CLANG_TIDY_CHECKS="$CLANG_TIDY_CHECKS,-readability-isolate-declaration"
CLANG_TIDY_CHECKS="$CLANG_TIDY_CHECKS,-readability-identifier-length"

#save startpath
STARTPATH=$(pwd)

#set umask
umask 0022

#get myMPD version
VERSION=$(grep CPACK_PACKAGE_VERSION_ CMakeLists.txt | cut -d\" -f2 | tr '\n' '.' | sed 's/\.$//')
COPYRIGHT="myMPD ${VERSION} | (c) 2018-2022 Juergen Mang <mail@jcgames.de> | SPDX-License-Identifier: GPL-3.0-or-later | https://github.com/jcorporation/mympd"

#check for command
check_cmd() {
  for DEPENDENCY in "$@"
  do
    if ! check_cmd_silent "$@"
    then
      echo_error "${DEPENDENCY} not found"
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

if [ "$ACTION" != "installdeps" ] && [ "$ACTION" != "" ]
then
  check_cmd gzip perl
  ZIP="gzip -n -f -v -9"
  ZIPCAT="gzip -n -v -9 -c"
fi

setversion() {
  TS=$(stat -c%Y CMakeLists.txt)
  export LC_TIME="en_GB.UTF-8"
  DATE_F1=$(date --date=@"${TS}" +"%a %b %d %Y")
  DATE_F2=$(date --date=@"${TS}" +"%a, %d %b %Y %H:%m:%S %z")
  DATE_F3=$(date --date=@"${TS}" +"%d %b %Y")
  echo "Setting version to ${VERSION} and date to ${DATE_F2}"

  for F in htdocs/sw.js contrib/packaging/alpine/APKBUILD contrib/packaging/arch/PKGBUILD \
  		contrib/packaging/rpm/mympd.spec contrib/packaging/debian/changelog \
  		contrib/packaging/openwrt/Makefile contrib/man/mympd.1 contrib/man/mympd-script.1
  do
  	echo "$F"
  	sed -e "s/__VERSION__/${VERSION}/g" -e "s/__DATE_F1__/$DATE_F1/g" -e "s/__DATE_F2__/$DATE_F2/g" \
  	  	-e "s/__DATE_F3__/$DATE_F3/g" "$F.in" > "$F"
  done

  #gentoo ebuild must be moved only
  if [ ! -f "contrib/packaging/gentoo/media-sound/mympd/mympd-${VERSION}.ebuild" ]
  then
  	mv -f contrib/packaging/gentoo/media-sound/mympd/mympd-*.ebuild \
      "contrib/packaging/gentoo/media-sound/mympd/mympd-${VERSION}.ebuild"
  fi

  echo "const myMPDversion = '${VERSION}';" > htdocs/js/version.js
  printf "%s" "${VERSION}" > docs/_includes/version
}

minify() {
  TYPE="$1"
  SRC="$2"
  DST="$3"

  #We remove only line-breaks, comments, blank lines and trim whitespaces
  echo "Minifying $SRC"
  if [ "$TYPE" = "html" ]
  then
    #shellcheck disable=SC2016
    if ! perl -pe 's/^<!--debug-->.*\n//gm; s/<!--release\s+(.+)-->/$1/g; s/<!--(.+)-->//g; s/^\s*//gm; s/\s*$//gm' "$SRC" > "${DST}.tmp"
    then
      rm -f "${DST}.tmp"
      echo_error "Error minifying $SRC"
      exit 1
    fi
  elif [ "$TYPE" = "js" ]
  then
    #shellcheck disable=SC2016
    if ! perl -pe 's/^\s*//gm; s/^\/\/.+$//g; s/^logDebug\(.*$//g; s/\/\*debug\*\/.*$//g; s/\s*$//gm;' "$SRC" > "${DST}.tmp"
    then
      rm -f "${DST}.tmp"
      echo_error "Error minifying $SRC"
      exit 1
    fi
  elif [ "$TYPE" = "css" ]
  then
    #shellcheck disable=SC2016
    if ! perl -pe 's/^\s*//gm; s/\s*$//gm; s/: /:/g;' "$SRC" | perl -pe 's/\/\*[^*]+\*\///g;' > "${DST}.tmp"
    then
      rm -f "${DST}.tmp"
      echo_error "Error minifying $SRC"
      exit 1
    fi
  fi

  #successfull minified file
  echo "" >> "${DST}.tmp"
  mv "${DST}.tmp" "$DST"
  return 0
}

createassets() {
  [ -z "${MYMPD_BUILDDIR+x}" ] && MYMPD_BUILDDIR="release"

  echo "Creating assets in $MYMPD_BUILDDIR"
  #Recreate asset directories
  rm -fr "$MYMPD_BUILDDIR/htdocs"
  install -d "$MYMPD_BUILDDIR/htdocs/js"
  install -d "$MYMPD_BUILDDIR/htdocs/css"
  install -d "$MYMPD_BUILDDIR/htdocs/assets"

  #Create translation phrases file
  createi18n "../../$MYMPD_BUILDDIR/htdocs/js/i18n.min.js" "" 2>/dev/null
  transstatus $MYMPD_BUILDDIR/htdocs/js/i18n.min.js

  echo "Minifying javascript"
  JSSRCFILES=""
  #shellcheck disable=SC2013
  for F in $(grep -E '<!--debug-->\s+<script' htdocs/index.html | cut -d\" -f2)
  do
    [ "$F" = "js/bootstrap-native.js" ] && continue;
    [ "$F" = "js/i18n.js" ] && continue;
    [ "$F" = "js/long-press-event.js" ] && continue;
    JSSRCFILES="$JSSRCFILES htdocs/$F"
    if tail -1 "htdocs/$F" | perl -npe 'exit 1 if m/\n/; exit 0'
    then
      echo_error "$F don't end with newline character"
      exit 1
    fi
  done
  echo "Creating mympd.js"
  #shellcheck disable=SC2086
  #shellcheck disable=SC2002
  cat $JSSRCFILES | grep -v "\"use strict\";" > "$MYMPD_BUILDDIR/htdocs/js/mympd.js"
  minify js htdocs/sw.js "$MYMPD_BUILDDIR/htdocs/sw.min.js"
  minify js "$MYMPD_BUILDDIR/htdocs/js/mympd.js" "$MYMPD_BUILDDIR/htdocs/js/mympd.min.js"

  echo "Combining and compressing javascript"
  echo "//${COPYRIGHT}" > "$MYMPD_BUILDDIR/htdocs/js/copyright.min.js"
  JSFILES="dist/bootstrap-native/bootstrap-native.min.js dist/long-press-event/long-press-event.min.js"
  JSFILES="$JSFILES $MYMPD_BUILDDIR/htdocs/js/*.min.js"
  for F in $JSFILES
  do
    if tail -1 "$F" | perl -npe 'exit 1 if m/\n/; exit 0'
    then
      echo_error "$F don't end with newline character"
      exit 1
    fi
  done
  echo "\"use strict\";" > "$MYMPD_BUILDDIR/htdocs/js/combined.js"
  #shellcheck disable=SC2086
  #shellcheck disable=SC2002
  cat $JSFILES >> "$MYMPD_BUILDDIR/htdocs/js/combined.js"
  $ZIP "$MYMPD_BUILDDIR/htdocs/js/combined.js"

  #serviceworker
  $ZIPCAT "$MYMPD_BUILDDIR/htdocs/sw.min.js" > "$MYMPD_BUILDDIR/htdocs/sw.js.gz"

  echo "Minifying stylesheets"
  for F in htdocs/css/*.css
  do
	  [ "$F" = "htdocs/css/bootstrap.css" ] && continue;
    DST=$(basename "$F" .css)
    minify css "$F" "$MYMPD_BUILDDIR/htdocs/css/${DST}.min.css"
  done

  echo "Combining and compressing stylesheets"
  echo "/* ${COPYRIGHT} */" > "$MYMPD_BUILDDIR/htdocs/css/copyright.min.css"
  CSSFILES="dist/bootstrap/compiled/custom.css $MYMPD_BUILDDIR/htdocs/css/*.min.css"
  #shellcheck disable=SC2086
  cat $CSSFILES > "$MYMPD_BUILDDIR/htdocs/css/combined.css"
  $ZIP "$MYMPD_BUILDDIR/htdocs/css/combined.css"

  echo "Compressing fonts"
  FONTFILES="dist/material-icons/MaterialIcons-Regular.woff2"
  for FONT in $FONTFILES
  do
    DST=$(basename "${FONT}")
    $ZIPCAT "$FONT" > "$MYMPD_BUILDDIR/htdocs/assets/${DST}.gz"
  done

  echo "Minifying and compressing html"
  minify html htdocs/index.html "$MYMPD_BUILDDIR/htdocs/index.html"
  $ZIPCAT "$MYMPD_BUILDDIR/htdocs/index.html" > "$MYMPD_BUILDDIR/htdocs/index.html.gz"

  echo "Creating other compressed assets"
  ASSETS="htdocs/mympd.webmanifest htdocs/assets/*.svg"
  for ASSET in $ASSETS
  do
    $ZIPCAT "$ASSET" > "$MYMPD_BUILDDIR/${ASSET}.gz"
  done
  return 0
}

buildrelease() {
  check_docs
  check_includes
  createassets
  EMBEDDED_ASSETS="ON"

  echo "Compiling myMPD"
  install -d release
  cd release || exit 1
  #force rebuild of web_server with embedded assets
  rm -vf CMakeFiles/mympd.dir/src/web_server/web_server_utility.c.o
  #set INSTALL_PREFIX and build myMPD
  export INSTALL_PREFIX="${MYMPD_INSTALL_PREFIX:-/usr}"
  #shellcheck disable=SC2086
  cmake -DCMAKE_INSTALL_PREFIX:PATH="$INSTALL_PREFIX" -DCMAKE_BUILD_TYPE=RELEASE \
  	-DENABLE_SSL="$ENABLE_SSL" -DENABLE_LIBID3TAG="$ENABLE_LIBID3TAG" \
  	-DENABLE_FLAC="$ENABLE_FLAC" -DENABLE_LUA="$ENABLE_LUA" \
    -DEMBEDDED_ASSETS="$EMBEDDED_ASSETS" -DENABLE_LIBASAN="$ENABLE_LIBASAN" \
    -DENABLE_IPV6="$ENABLE_IPV6" $EXTRA_CMAKE_OPTIONS ..
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
      echo_error "Can not add group mympd"
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
      echo_error "Can not add user mympd"
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
}

builddebug() {
  install -d debug/htdocs/js
  createi18n ../../debug/htdocs/js/i18n.js pretty 2>/dev/null
  transstatus debug/htdocs/js/i18n.js
  check_docs
  check_includes

  if [ "$EMBEDDED_ASSETS" = "OFF" ]
  then
    echo "Copy dist assets"
    cp "$PWD/dist/bootstrap/compiled/custom.css" "$PWD/htdocs/css/bootstrap.css"
    cp "$PWD/dist/bootstrap-native/bootstrap-native.js" "$PWD/htdocs/js/bootstrap-native.js"
    cp "$PWD/dist/long-press-event/long-press-event.js" "$PWD/htdocs/js/long-press-event.js"
    cp "$PWD/dist/material-icons/MaterialIcons-Regular.woff2" "$PWD/htdocs/assets/MaterialIcons-Regular.woff2"
    cp "$PWD/debug/htdocs/js/i18n.js" "$PWD/htdocs/js/i18n.js"
  else
    MYMPD_BUILDDIR="debug"
    createassets
  fi

  echo "Compiling myMPD"
  cd debug || exit 1
  #shellcheck disable=SC2086
  cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr -DCMAKE_BUILD_TYPE=DEBUG \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  	-DENABLE_SSL="$ENABLE_SSL" -DENABLE_LIBID3TAG="$ENABLE_LIBID3TAG" \
    -DENABLE_FLAC="$ENABLE_FLAC" -DENABLE_LUA="$ENABLE_LUA" \
    -DEMBEDDED_ASSETS="$EMBEDDED_ASSETS" -DENABLE_LIBASAN="$ENABLE_LIBASAN" \
    -DENABLE_IPV6="$ENABLE_IPV6" $EXTRA_CMAKE_OPTIONS ..
  make VERBOSE=1
  echo "Linking compilation database"
  sed -e 's/\\t/ /g' -e 's/-Wformat-truncation//g' -e 's/-Wformat-overflow=2//g' -e 's/-fsanitize=bounds-strict//g' \
    -e 's/-static-libasan//g' -e 's/-Wno-stringop-overread//g' -e 's/-fstack-clash-protection//g' \
    compile_commands.json > ../src/compile_commands.json
}

buildtest() {
  install -d test/build
  cd test/build || exit 1
  #shellcheck disable=SC2086
  cmake -DCMAKE_BUILD_TYPE=DEBUG $EXTRA_CMAKE_OPTIONS ..
  make VERBOSE=1
  ./test
}

cleanup() {
  [ -z "${MYMPD_BUILDDIR+x}" ] && MYMPD_BUILDDIR="release"
  #build directories
  rm -rf "$MYMPD_BUILDDIR"
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

check_docs() {
  rc=0
  grep -v '//' src/lib/api.h | grep 'X(MYMPD' | cut -d\( -f2 | cut -d\) -f1 | \
  while IFS= read -r METHOD
  do
    if ! grep -q "$METHOD" htdocs/js/apidoc.js
    then
      echo_warn "API $METHOD not documented"
      rc=1
    fi
  done
  O=$(md5sum htdocs/js/apidoc.js | awk '{print $1}')
  C=$(md5sum docs/assets/apidoc.js | awk '{print $1}')
  if [ "$O" != "$C" ]
  then
  	echo_warn "apidoc.js in docs differs"
    cp htdocs/js/apidoc.js docs/assets/apidoc.js
    rc=1
  fi
  return "$rc"
}

check_includes() {
  rc=0
  find src/ -name \*.c | while IFS= read -r FILE
  do
    if ! grep -m1 "#include" "$FILE" | grep -q "mympd_config_defs.h"
    then
      echo_warn "First include is not mympd_config_defs.h: $FILE"
      rc=1
    fi
    SRCDIR=$(dirname "$FILE")

    grep "#include \"" "$FILE" | grep -v "mympd_config_defs.h" | cut -d\" -f2 | \
    while IFS= read -r INCLUDE
    do
      if ! realpath "$SRCDIR/$INCLUDE" > /dev/null 2>&1
      then
        echo_error "Wrong include path in $FILE for $INCLUDE"
        rc=1
      fi
    done
    return "$rc"
  done
}

check_file() {
  FILE=$1
  if check_cmd cppcheck
  then
    echo "Running cppcheck"
    [ -z "${CPPCHECKOPTS+z}" ] && CPPCHECKOPTS="-q --force --enable=warning"
    #shellcheck disable=SC2086
    cppcheck $CPPCHECKOPTS "$FILE"
  else
    echo_warn "cppcheck not found"
  fi

  if check_cmd flawfinder
  then
    echo "Running flawfinder"
    [ -z "${FLAWFINDEROPTS+z}" ] && FLAWFINDEROPTS="-m3 --quiet --dataonly"
    #shellcheck disable=SC2086
    flawfinder $FLAWFINDEROPTS "$FILE"
  else
    echo_warn "flawfinder not found"
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
    clang-tidy --checks="$CLANG_TIDY_CHECKS" \
    	"$FILE" > ../clang-tidy.out 2>/dev/null
    grep -v -E "(/usr/include/|memset|memcpy|\^)" ../clang-tidy.out
  else
    echo_warn "clang-tidy not found"
  fi
}

check() {
  if check_cmd cppcheck
  then
    echo "Running cppcheck"
    [ -z "${CPPCHECKOPTS+z}" ] && CPPCHECKOPTS="-q --force --enable=warning"
    find ./src/ -name \*.c | while read -r FILE
    do
      [ "$FILE" = "./src/mympd_api/mympd_api_scripts_lualibs.c" ] && continue
      [ "$FILE" = "./src/web_server/web_server_embedded_files.c" ] && continue
      #shellcheck disable=SC2086
      if ! cppcheck $CPPCHECKOPTS --error-exitcode=1 "$FILE"
      then
        return 1
      fi
    done
    find ./src/ -name \*.h | while read -r FILE
    do
      #shellcheck disable=SC2086
      if ! cppcheck $CPPCHECKOPTS --error-exitcode=1 "$FILE"
      then
        return 1
      fi
    done
  else
    echo_warn "cppcheck not found"
    return 1
  fi

  if check_cmd flawfinder
  then
    echo "Running flawfinder"
    [ -z "${FLAWFINDEROPTS+z}" ] && FLAWFINDEROPTS="-m3 --quiet --dataonly"
    #shellcheck disable=SC2086
    if ! flawfinder $FLAWFINDEROPTS --error-level=3 src
    then
      return 1
    fi
    #shellcheck disable=SC2086
    if ! flawfinder $FLAWFINDEROPTS --error-level=3 cli_tools
    then
      return 1
    fi
  else
    echo_warn "flawfinder not found"
    return 1
  fi

  if [ ! -f src/compile_commands.json ]
  then
    echo "src/compile_commands.json not found"
    echo "run: ./build.sh debug"
    return 1
  fi

  if check_cmd clang-tidy
  then
    echo "Running clang-tidy, output goes to clang-tidy.out"
    rm -f clang-tidy.out
    cd src || exit 1
    find ./ -name '*.c' -exec clang-tidy \
    	--checks="$CLANG_TIDY_CHECKS" {} \; >> ../clang-tidy.out 2>/dev/null
    ERRORS=$(grep -v -E "(/usr/include/|memset|memcpy|\^)" ../clang-tidy.out)
    if [ -n "$ERRORS" ]
    then
      echo "$ERRORS"
      return 1
    fi
    cd .. || return 1
  else
    echo_warn "clang-tidy not found"
    return 1
  fi

  if ! check_docs
  then
    return 1
  fi
  if ! check_includes
  then
    return 1
  fi
  return 0
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
    echo_warn "Package would not be signed"
  fi
  #shellcheck disable=SC2086
  dpkg-buildpackage -rfakeroot $SIGNOPT

  #get created package name
  PACKAGE=$(ls ../mympd_"${VERSION}"-1_*.deb)
  if [ "$PACKAGE" = "" ]
  then
    echo_error "Can't find package"
  fi

  if check_cmd lintian
  then
    echo "Checking package with lintian"
    lintian "$PACKAGE"
  else
    echo_warn "lintian not found, can't check package"
  fi
}

pkgdocker() {
  check_cmd docker
  [ -z "${DOCKERFILE+x}" ] && DOCKERFILE="Dockerfile.alpine"
  prepare
  docker build --rm -t mympd -f "contrib/packaging/docker/$DOCKERFILE" .
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
    echo_warn "rpmlint not found, can't check package"
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
    [ -z "${GPGKEYID+x}" ] || KEYARG="--key $GPGKEYID"
    #shellcheck disable=SC2086
    makepkg --sign $KEYARG mympd-*.pkg.tar.xz
  fi
  if check_cmd namcap
  then
    echo "Checking package with namcap"
    namcap PKGBUILD
    namcap mympd-*.pkg.tar.xz
  else
    echo_warn "namcap not found, can't check package"
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
    cp "../package/mympd_${VERSION}-1.debian.tar.xz" "$OSC_REPO/"
  fi
  cp ../contrib/packaging/rpm/mympd.spec "$OSC_REPO/"
  cp ../contrib/packaging/arch/PKGBUILD "$OSC_REPO/"
  cp ../contrib/packaging/arch/archlinux.install "$OSC_REPO/"

  cd "$OSC_REPO" || exit 1
  osc addremove
  osc st
  osc vc -m "Update"
  osc commit -m "Update"
}

installdeps() {
  echo "Platform: $(uname -m)"
  if [ -f /etc/debian_version ]
  then
    #we install lua5.3 but lua 5.4 works also
    apt-get update
    apt-get install -y --no-install-recommends \
	    gcc cmake perl libssl-dev libid3tag0-dev libflac-dev \
	    build-essential liblua5.3-dev pkg-config libpcre2-dev jq gzip
  elif [ -f /etc/arch-release ]
  then
    #arch
    pacman -S gcc cmake perl openssl libid3tag flac lua pkgconf pcre2 jq gzip
  elif [ -f /etc/alpine-release ]
  then
    #alpine
    apk add cmake perl openssl-dev libid3tag-dev flac-dev lua5.4-dev \
    	alpine-sdk linux-headers pkgconf pcre2-dev jq gzip
  elif [ -f /etc/SuSE-release ]
  then
    #suse
    zypper install gcc cmake pkgconfig perl openssl-devel libid3tag-devel flac-devel \
	    lua-devel unzip pcre2-devel jq gzip
  elif [ -f /etc/redhat-release ]
  then
    #fedora
    yum install gcc cmake pkgconfig perl openssl-devel libid3tag-devel flac-devel \
	    lua-devel unzip pcre2-devel jq gzip
  else
    echo_warn "Unsupported distribution detected."
    echo "You should manually install:"
    echo "  - gcc"
    echo "  - cmake"
    echo "  - perl"
    echo "  - jq"
    echo "  - openssl (devel)"
    echo "  - flac (devel)"
    echo "  - libid3tag (devel)"
    echo "  - liblua5.3 or liblua5.4 (devel)"
    echo "  - libpcre2 (devel)"
  fi
}

updatelibmympdclient() {
  check_cmd git meson

  cd dist/libmpdclient || exit 1
  STARTDIR=$(pwd)

  TMPDIR=$(mktemp -d)
  cd "$TMPDIR" || exit 1
  git clone --depth=1 -b libmympdclient https://github.com/jcorporation/libmpdclient.git
  cd libmpdclient || exit 1
  meson . output -Dbuffer_size=8192

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

updatebootstrapnative() {
  check_cmd git npm
  cd dist/bootstrap-native || exit 1
  STARTDIR=$(pwd)

  TMPDIR=$(mktemp -d)
  cd "$TMPDIR" || exit 1
  git clone --depth=1 -b master https://github.com/thednp/bootstrap.native
  cd bootstrap.native
  npm install @rollup/plugin-buble
  cp "$STARTDIR/mympd-config.js" src/
  cp "$STARTDIR/mympd-init.js" src/util/
  npm run custom INPUTFILE:src/mympd-config.js,OUTPUTFILE:dist/bootstrap-mympd.js,MIN:false,FORMAT:umd
  npm run custom INPUTFILE:src/mympd-config.js,OUTPUTFILE:dist/bootstrap-mympd.min.js,MIN:true,FORMAT:umd

  cp dist/bootstrap-mympd.js "$STARTDIR/bootstrap-native.js"
  cp dist/bootstrap-mympd.min.js "$STARTDIR/bootstrap-native.min.js"

  cd "$STARTDIR" || exit 1
  rm -rf "$TMPDIR"

  if [ -d ../../debug ]
  then
  	cp bootstrap-native.js ../../htdocs/js/
  fi
}

updatebootstrap() {
  check_cmd npm
  cd dist/bootstrap || exit 1
  [ -z "${BOOTSTRAP_VERSION+x}" ] && BOOTSTRAP_VERSION=""
  npm install "$BOOTSTRAP_VERSION"
  npm run build
  sed -i '$ d' compiled/custom.css
  rm compiled/custom.css.map
  if [ -d ../../debug ]
  then
  	cp -v compiled/custom.css ../../htdocs/css/bootstrap.css
  fi
}

#Also deletes stale installations in other locations.
#
uninstall() {
  #cmake does not provide an uninstall target, instead its manifest is of use at least for
  #the binaries
  if [ -f release/install_manifest.txt ]
  then
    xargs rm < release/install_manifest.txt
  fi
  [ -z "${DESTDIR+x}" ] && DESTDIR=""
  #MYMPD_INSTALL_PREFIX="/usr"
  rm -f "$DESTDIR/usr/bin/mympd"
  rm -f "$DESTDIR/usr/bin/mympd-script"
  rm -rf "$DESTDIR/usr/share/doc/mympd"
  rm -f "$DESTDIR/usr/share/man/man1/mympd.1.gz"
  rm -f "$DESTDIR/usr/share/man/man1/mympd-script.1.gz"
  #MYMPD_INSTALL_PREFIX="/usr/local"
  rm -f "$DESTDIR/usr/local/bin/mympd"
  rm -f "$DESTDIR/usr/local/bin/mympd-script"
  rm -rf "$DESTDIR/usr/local/share/doc/mympd"
  rm -f "$DESTDIR/usr/local/share/man/man1/mympd.1.gz"
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
  rm -rf "$DESTDIR/var/cache/mympd"
  rm -f "$DESTDIR/etc/init.d/mympd"
  #MYMPD_INSTALL_PREFIX="/opt/mympd/"
  rm -rf "$DESTDIR/var/opt/mympd"
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
      echo_error "Can not remove user mympd"
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
      echo_error "Can not remove user mympd"
      return 1
    fi
  fi
}

createi18n() {
  DST=$1
  PRETTY=$2
  cd src/i18n || exit 1
  echo "Creating i18n json"
  if ! perl ./tojson.pl "$PRETTY" > "$DST"
  then
    echo "Error creating translation files"
    exit 1
  fi
  cd ../.. || exit 1
}

transstatus() {
  if check_cmd_silent jq
  then
    TFILE=$1
    MISSING=$(grep missingPhrases "$TFILE" | sed -e 's/.*missingPhrases=//' -e 's/;//' | jq '.' -r -M)
    if [ "$MISSING" = "{}" ]
    then
      echo "All translation phrased found"
    else
      echo_warn "Missing translation phrases ($TFILE):"
      echo "$MISSING"
    fi
  else
    echo_warn "jq not found - can not print translation statistics"
  fi
}

materialicons() {
  check_cmd jq
  check_cmd wget

  TMPDIR=$(mktemp -d)
  cd "$TMPDIR" || exit 1
  FONT_URI=$(wget -q "https://fonts.googleapis.com/css2?family=Material+Icons" -O - | \
    grep url | cut -d\( -f2 | cut -d\) -f1)
  if ! wget -q "$FONT_URI" -O MaterialIcons-Regular.woff2
  then
    echo_error "Error downloading font file"
    exit 1
  fi
  METADATA_URI="https://fonts.google.com/metadata/icons"
  if ! wget -q "$METADATA_URI" -O metadata.json
  then
    echo_error "Error downloading metadata"
    exit 1
  fi
  sed -i '1d' metadata.json
  printf "const materialIcons={" > "ligatures.js"
  I=0
  for CAT in $(jq -r ".icons[].categories | .[]" < metadata.json | sort -u)
  do
    [ "$I" -gt 0 ] && printf "," >> "ligatures.js"
    printf "\"%s\":[" "$CAT" >> "ligatures.js"
    J=0
    for ICON in $(jq -r ".icons[] | select(.categories[]==\"$CAT\") | .name" < metadata.json)
    do
      [ "$J" -gt 0 ] && printf "," >> "ligatures.js"
      printf "\"%s\"" "$ICON" >> "ligatures.js"
      J=$((J+1))
    done
    printf "]" >> "ligatures.js"
    I=$((I+1))
  done
  echo "};"  >> "ligatures.js"
  cp ligatures.js "$STARTPATH/htdocs/js/"
  cp MaterialIcons-Regular.woff2 "$STARTPATH/dist/material-icons/"
  cd "$STARTPATH"
  rm -fr "$TMPDIR"
}

sbuild_chroots() {
  if [ "$(id -u)" != "0" ]
  then
    echo "Must be run as root:"
    echo "  sudo -E ./build.sh sbuild_chroots"
  	exit 1
  fi
  [ -z "${WORKDIR+x}" ] && WORKDIR="$STARTPATH/builder"
  [ -z "${DISTROS+x}" ] && DISTROS="bullseye buster"
  [ -z "${TARGETS+x}" ] && TARGETS="armhf armel"
  [ -z "${DEBIAN_MIRROR+x}" ] && DEBIAN_MIRROR="http://ftp.de.debian.org/debian"
  [ -z "${DEBOOTSTRAP+x}" ] && DEBOOTSTRAP="debootstrap"

  check_cmd sbuild "$DEBOOTSTRAP"

  mkdir -p "${WORKDIR}/chroot"
  mkdir -p "${WORKDIR}/cache"

  for DIST in ${DISTROS}
  do
    for ARCH in ${TARGETS}
    do
      CHROOT="${DIST}-${ARCH}"
      echo "Creating chroot for $CHROOT"
      if [ -d "${WORKDIR}/chroot/${CHROOT}" ]
      then
        echo "chroot ${CHROOT} already exists... skipping."
        continue
      fi
      $DEBOOTSTRAP --arch="${ARCH}" --variant=buildd --cache-dir="${WORKDIR}/cache" \
        --include=fakeroot,build-essential "${DIST}" "${WORKDIR}/chroot/${CHROOT}/" "${DEBIAN_MIRROR}"

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
    echo "Must be run as root:"
    echo "  sudo -E ./build.sh sbuild_build"
  	exit 1
  fi
  [ -z "${WORKDIR+x}" ] && WORKDIR="$STARTPATH/builder"
  [ -z "${DISTROS+x}" ] && DISTROS="bullseye buster"
  [ -z "${TARGETS+x}" ] && TARGETS="armhf armel"

  check_cmd sbuild

  prepare
  cp -a contrib/packaging/debian .
  export LC_TIME="en_GB.UTF-8"
  tar -czf "../mympd_${VERSION}.orig.tar.gz" -- *
  cd ..
  #Compile for target distro/arch
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
    echo "Must be run as root:"
    echo "  sudo -E ./build.sh sbuild_cleanup"
  	exit 1
  fi
  [ -z "${WORKDIR+x}" ] && WORKDIR="$STARTPATH/builder"
  rm -rf "${WORKDIR}"
}

run_eslint() {
  if ! check_cmd npx
  then
    return 1
  fi
  createassets
  rc=0
  echo ""
  for F in htdocs/sw.js release/htdocs/js/mympd.js
  do
    echo "Linting $F"
    if ! npx eslint $F
    then
      rc=1
    fi
  done
  for F in release/htdocs/sw.min.js release/htdocs/js/mympd.min.js release/htdocs/js/i18n.min.js
  do
    echo "Linting $F"
    if ! npx eslint -c .eslintrc-min.json $F
    then
      rc=1
    fi
  done
  echo "Check for forbidden js functions"
  FORBIDDEN_CMDS="innerHTML outerHTML insertAdjacentHTML innerText"
  for F in $FORBIDDEN_CMDS
  do
  	if grep -q "$F" release/htdocs/js/mympd.min.js
  	then
  		echo_error "Found $F usage"
      rc=1
  	fi
  done
  return "$rc"
}

run_stylelint() {
  if ! check_cmd npx
  then
    return 1
  fi
  rc=0
  for F in mympd.css theme-light.css
  do
    echo "Linting $F"
    if ! npx stylelint "htdocs/css/$F"
    then
      rc=1
    fi
  done
  return "$rc"
}

run_htmlhint() {
  if ! check_cmd npx
  then
    return 1
  fi
  echo "Linting htdocs/index.html"
  if ! npx htmlhint htdocs/index.html
  then
    return 1
  fi
  return 0
}

luascript_index() {
  rm -f "docs/scripting/scripts/index.json"
  exec 3<> "docs/scripting/scripts/index.json"
  printf "{\"scripts\":[" >&3
  I=0
  for F in docs/scripting/scripts/*.lua
  do
    [ "$I" -gt 0 ] &&  printf "," >&3
    SCRIPTNAME=$(basename "$F")
    printf "\"%s\"" "$SCRIPTNAME" >&3
    I=$((I+1))
  done
  printf "]}\n" >&3
  exec 3>&-
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
	  builddebug
	;;
	memcheck)
	  builddebug
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
	  if ! check
    then
      exit 1
    fi
	;;
  check_file)
    if [ -z "${2+x}" ]
    then
      echo "Usage: $0 $1 <filename>"
      exit 1
    fi
    check_file "$2"
	;;
	check_docs)
	  check_docs
	;;
	check_includes)
	  check_includes
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
  bootstrapnative)
    updatebootstrapnative
  ;;
  bootstrap)
    updatebootstrap
  ;;
	uninstall)
	  uninstall
	;;
	purge)
	  uninstall
	  purge
	;;
	translate)
	  createi18n ../../htdocs/js/i18n.js pretty
	;;
	transstatus)
    createi18n ../../htdocs/js/i18n.js pretty 2>/dev/null
	  transstatus htdocs/js/i18n.js
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
	;;
  lint)
    if ! run_htmlhint
    then
      exit 1
    fi
    pwd
    if ! run_eslint
    then
      exit 1
    fi
    pwd
    if ! run_stylelint
    then
      exit 1
    fi
  ;;
	eslint)
    run_eslint
	;;
	stylelint)
	  run_stylelint
	;;
	htmlhint)
	  run_htmlhint
	;;
  luascript_index)
    luascript_index
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
    echo "                    serves assets from htdocs"
    echo "  memcheck:         builds debug files in directory debug"
    echo "                    linked with libasan3 and not embedding assets"
    echo "  test:             builds the unit testing files in test/build"
    echo "  installdeps:      installs build and run dependencies"
    echo "  createassets:     creates the minfied and compressed dist files"
    echo "                    following environment variables are respected"
    echo "                      - MYMPD_BUILDDIR=\"release\""
    echo ""
    echo "Translation options:"
    echo "  translate:        builds the translation file for debug builds"
    echo "  transstatus:      shows the translation status"
    echo ""
    echo "Check options:"
    echo "  check:            runs cppcheck, flawfinder and clang-tidy on source files"
    echo "                    following environment variables are respected"
    echo "                      - CPPCHECKOPTS=\"-q --force --enable=warning\""
    echo "                      - FLAWFINDEROPTS=\"-m3 --quiet --dataonly\""
    echo "  check_file:       same as check, but for one file, second arg must be the file"
    echo "  check_docs        checks the documentation for missing API methods"
    echo "  check_includes:   checks for valid include paths"
    echo "  lint:             runs eslint, stylelint and htmlhint"
    echo "  eslint:           combines javascript files and runs eslint"
    echo "  stylelint:        runs stylelint (lints css files)"
    echo "  htmlhint:         runs htmlhint (lints html files)"
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
    echo "                      - DEBOOTSTRAP=\"debootstrap"
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
    echo "  addmympduser:     adds mympd group and user"
    echo "  luascript_index:  creates the json index of lua scripts"
    echo ""
    echo "Source update options:"
    echo "  bootstrap:        updates bootstrap"
    echo "  bootstrapnative:  updates bootstrap.native"
    echo "  libmympdclient:   updates libmympdclient (fork of libmpdclient)"
    echo "  materialicons:    updates the materialicons json"
    echo "  setversion:       sets version and date in packaging files from CMakeLists.txt"
    echo ""
    echo "Environment variables (with defaults) for building"
    echo "  - EMBEDDED_ASSETS=\"ON\""
    echo "  - ENABLE_FLAC=\"ON\""
    echo "  - ENABLE_IPV6=\"ON\""
    echo "  - ENABLE_LIBASAN=\"OFF\""
    echo "  - ENABLE_LIBID3TAG=\"ON\""
    echo "  - ENABLE_LUA=\"ON\""
    echo "  - ENABLE_SSL=\"ON\""
    echo "  - EXTRA_CMAKE_OPTIONS=\"\""
    echo "  - MANPAGES=\"ON\""
    echo "  - MYMPD_INSTALL_PREFIX=\"/usr\""
    echo ""
    exit 1
	;;
esac

exit 0
