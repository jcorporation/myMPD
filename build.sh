#!/bin/sh
#
#SPDX-License-Identifier: GPL-3.0-or-later
#myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
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

#save script path and change to it
STARTPATH=$(dirname "$(realpath "$0")")
cd "$STARTPATH" || exit 1

#set umask
umask 0022

#get myMPD version
VERSION=$(grep "  VERSION" CMakeLists.txt | sed 's/  VERSION //')
COPYRIGHT="myMPD ${VERSION} | (c) 2018-2024 Juergen Mang <mail@jcgames.de> | SPDX-License-Identifier: GPL-3.0-or-later | https://github.com/jcorporation/mympd"

MYMPD_MINIFY_JS="1"
if [ -f .git/HEAD ] && ! grep -q "master" .git/HEAD
then
  MYMPD_MINIFY_JS="0"
fi

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

  for F in contrib/packaging/alpine/APKBUILD contrib/packaging/arch/PKGBUILD \
      contrib/packaging/rpm/mympd.spec contrib/packaging/debian/changelog \
      contrib/packaging/openwrt/Makefile contrib/man/mympd.1 contrib/man/mympd-config.1 \
      contrib/man/mympd-script.1 contrib/packaging/freebsd/multimedia/mympd/Makefile
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

  printf "%s" "${VERSION}" > docs/_includes/version

  BUILD=$(git log --format="%H" -n 1)
  echo "const myMPDversion = '${VERSION}';" > htdocs/js/version.js
  echo "const myMPDbuild = '${BUILD}';" >> htdocs/js/version.js
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
    if [ "$MYMPD_MINIFY_JS" = "0" ]
    then
      cp "$SRC" "${DST}.tmp"
    else
      if ! perl -pe 's/^\s*//gm; s/^\s*\/?\*.*$//g; s/^\/\/.+$//g; s/^logDebug\(.*$//g; s/\/\*debug\*\/.*$//g; s/\s*$//gm;' "$SRC" > "${DST}.tmp"
      then
        rm -f "${DST}.tmp"
        echo_error "Error minifying $SRC"
        exit 1
      fi
    fi
  elif [ "$TYPE" = "json" ]
  then
    #shellcheck disable=SC2016
    if ! jq -r tostring "$SRC" | tr -d '\n' > "${DST}.tmp"
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
  check_cmd jq

  [ -z "${MYMPD_BUILDDIR+x}" ] && MYMPD_BUILDDIR="release"

  echo "Creating assets in $MYMPD_BUILDDIR"
  #Recreate asset directories
  rm -fr "$MYMPD_BUILDDIR/htdocs"
  install -d "$MYMPD_BUILDDIR/htdocs/js"
  install -d "$MYMPD_BUILDDIR/htdocs/css"
  install -d "$MYMPD_BUILDDIR/htdocs/assets/i18n"

  #Create translation phrases file
  check_phrases
  createi18n 2>/dev/null
  minify js "$MYMPD_BUILDDIR/htdocs/js/i18n.js" "$MYMPD_BUILDDIR/htdocs/js/i18n.min.js"

  #Create defines
  create_js_defines

  echo "Minifying javascript"
  JSSRCFILES=""
  #shellcheck disable=SC2013
  for F in $(grep -E '<!--debug-->\s+<script' htdocs/index.html | cut -d\" -f2)
  do
    [ "$F" = "js/bootstrap-native.js" ] && continue
    [ "$F" = "js/i18n.js" ] && continue
    [ "$F" = "js/long-press-event.js" ] && continue
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
  if [ "$MYMPD_MINIFY_JS" = "0" ]
  then
    JSFILES="dist/bootstrap-native/bootstrap-native.js dist/long-press-event/long-press-event.js"
  else
    JSFILES="dist/bootstrap-native/bootstrap-native.min.js dist/long-press-event/long-press-event.min.js"
  fi
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
    [ "$F" = "htdocs/css/bootstrap.css" ] && continue
    DST=$(basename "$F" .css)
    minify css "$F" "$MYMPD_BUILDDIR/htdocs/css/${DST}.min.css"
  done

  echo "Combining and compressing stylesheets"
  echo "/* ${COPYRIGHT} */" > "$MYMPD_BUILDDIR/htdocs/css/copyright.min.css"
  CSSFILES="dist/bootstrap/compiled/custom.css $MYMPD_BUILDDIR/htdocs/css/*.min.css"
  #shellcheck disable=SC2086
  cat $CSSFILES > "$MYMPD_BUILDDIR/htdocs/css/combined.css"
  $ZIP "$MYMPD_BUILDDIR/htdocs/css/combined.css"

  echo "Compressing i18n json"
  jq -r "select(.missingPhrases < 100) | keys[]" "$STARTPATH/src/i18n/json/i18n.json" | grep -v "default" | \
    while read -r CODE
    do
      minify json "$STARTPATH/src/i18n/json/${CODE}.json" "$MYMPD_BUILDDIR/htdocs/assets/i18n/${CODE}.min.json"
      $ZIPCAT "$MYMPD_BUILDDIR/htdocs/assets/i18n/${CODE}.min.json" > "$MYMPD_BUILDDIR/htdocs/assets/i18n/${CODE}.json.gz"
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

  echo "Copy images"
  cp -v htdocs/assets/*.png "$MYMPD_BUILDDIR/htdocs/assets/"

  echo "Copy webfont"
  cp -v dist/material-icons/MaterialIcons-Regular.woff2 "$MYMPD_BUILDDIR/htdocs/assets/"
  $ZIPCAT dist/material-icons/ligatures.json > "$MYMPD_BUILDDIR/htdocs/assets/ligatures.json.gz"

  lualibs

  return 0
}

lualibs() {
  [ -z "${MYMPD_ENABLE_MYGPIOD+x}" ] && MYMPD_ENABLE_MYGPIOD="OFF"
  echo "Copy integrated lua libraries"
  mkdir -p "$MYMPD_BUILDDIR/contrib/lualibs"
  cp -v contrib/lualibs/json.lua "$MYMPD_BUILDDIR/contrib/lualibs/"
  cp -v contrib/lualibs/mympd/00-start.lua "$MYMPD_BUILDDIR/contrib/lualibs/mympd.lua"
  cat contrib/lualibs/mympd/10-mympd.lua >> "$MYMPD_BUILDDIR/contrib/lualibs/mympd.lua"
  cat contrib/lualibs/mympd/20-http_client.lua >> "$MYMPD_BUILDDIR/contrib/lualibs/mympd.lua"
  cat contrib/lualibs/mympd/30-execute.lua >> "$MYMPD_BUILDDIR/contrib/lualibs/mympd.lua"
  [ "$MYMPD_ENABLE_MYGPIOD" = "ON" ] && cat contrib/lualibs/mympd/40-mygpiod.lua >> "$MYMPD_BUILDDIR/contrib/lualibs/mympd.lua"
  cat contrib/lualibs/mympd/99-end.lua >> "$MYMPD_BUILDDIR/contrib/lualibs/mympd.lua"
}

buildrelease() {
  BUILD_TYPE=$1
  echo "Compiling myMPD v${VERSION}"
  cmake -B release \
    -DCMAKE_INSTALL_PREFIX:PATH=/usr \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    .
  make -j4 -C release
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
  if [ ! -f /usr/lib/systemd/system/mympd.service ] &&
     [ ! -f /usr/systemd/system/mympd.service ]
  then
    addmympduser
  else
    echo "Systemd found skipping mympd user creation"
  fi
  echo "myMPD installed"
}

copyassets() {
  echo "Copy dist assets"
  [ -z "${MYMPD_BUILDDIR+x}" ] && MYMPD_BUILDDIR="debug"

  cp -v "$STARTPATH/dist/bootstrap/compiled/custom.css" "$STARTPATH/htdocs/css/bootstrap.css"
  cp -v "$STARTPATH/dist/bootstrap-native/bootstrap-native.js" "$STARTPATH/htdocs/js/bootstrap-native.js"
  cp -v "$STARTPATH/dist/long-press-event/long-press-event.js" "$STARTPATH/htdocs/js/long-press-event.js"
  cp -v "$STARTPATH/dist/material-icons/MaterialIcons-Regular.woff2" "$STARTPATH/htdocs/assets/MaterialIcons-Regular.woff2"
  cp -v "$STARTPATH/dist/material-icons/ligatures.json" "$STARTPATH/htdocs/assets/ligatures.json"

  lualibs

  #Create defines
  create_js_defines

  #translation files
  check_phrases
  createi18n
  cp -v "$MYMPD_BUILDDIR/htdocs/js/i18n.js" "$STARTPATH/htdocs/js/i18n.js"
  rm -fr "$STARTPATH/htdocs/assets/i18n/"
  install -d "$STARTPATH/htdocs/assets/i18n"
  jq -r "select(.missingPhrases < 100) | keys[]" "$STARTPATH/src/i18n/json/i18n.json" | grep -v "default" | \
    while read -r CODE
    do
      minify json "$STARTPATH/src/i18n/json/${CODE}.json" "$STARTPATH/htdocs/assets/i18n/${CODE}.json"
    done
}

builddebug() {
  echo "Compiling myMPD v${VERSION}"
  CMAKE_SANITIZER_OPTIONS=""
  case "$ACTION" in
    asan)  CMAKE_SANITIZER_OPTIONS="-DMYMPD_ENABLE_ASAN=ON" ;;
    tsan)  CMAKE_SANITIZER_OPTIONS="-DMYMPD_ENABLE_TSAN=ON" ;;
    ubsan) CMAKE_SANITIZER_OPTIONS="-DMYMPD_ENABLE_UBSAN=ON" ;;
  esac

  cmake -B debug \
    -DCMAKE_INSTALL_PREFIX:PATH=/usr \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    $CMAKE_SANITIZER_OPTIONS \
    .
  make -j4 -C debug VERBOSE=1
  echo "Linking compilation database"
  sed -e 's/\\t/ /g' -e 's/-Wformat-truncation//g' -e 's/-Wformat-overflow=2//g' -e 's/-fsanitize=bounds-strict//g' \
    -e 's/-Wno-stringop-overread//g' -e 's/-fstack-clash-protection//g' \
    debug/compile_commands.json > src/compile_commands.json
}

buildtest() {
  echo "Compiling and running unit tests"
  cmake -B debug \
    -DCMAKE_INSTALL_PREFIX:PATH=/usr \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DMYMPD_ENABLE_ASAN=ON \
    -DMYMPD_BUILD_TESTING=ON \
    .
  make -j4 -C debug
  make -j4 -C debug test
  echo "Linking compilation database"
  sed -e 's/\\t/ /g' -e 's/-Wformat-truncation//g' -e 's/-Wformat-overflow=2//g' -e 's/-fsanitize=bounds-strict//g' \
    -e 's/-Wno-stringop-overread//g' -e 's/-fstack-clash-protection//g' \
    debug/compile_commands.json > test/compile_commands.json
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
  rm -f htdocs/sw.js
  rm -f htdocs/js/bootstrap-native.js
  rm -f htdocs/js/long-press-event.js
  rm -f htdocs/js/i18n.js
  rm -f htdocs/css/bootstrap.css
  rm -f htdocs/assets/MaterialIcons-Regular.woff2
  rm -f htdocs/assets/ligatures.json
  rm -rf htdocs/assets/i18n

  #generated documentation
  rm -rf docs/_site
  rm -rf docs/.jekyll-cache
  rm -rf docs/doxygen
  rm -rf docs/jsdoc

  #compilation database
  rm -f src/compile_commands.json

  #caches
  rm -fr src/.cache
  rm -fr .cache

  #node modules
  rm -fr dist/bootstrap/node_modules

  #clang tidy
  rm -f clang-tidy.out
}

cleanuposc() {
  rm -rf osc
}

check_docs() {
  rc=0
  METHODS=$(grep -v '//' src/lib/api.h | grep 'X(MYMPD' | cut -d\( -f2 | cut -d\) -f1)
  for METHOD in $METHODS
  do
    if ! grep -q "$METHOD" htdocs/js/apidoc.js
    then
      echo_warn "API $METHOD not documented"
      rc=1
    fi
  done
  return "$rc"
}

check_includes() {
  rc=0
  FILES=$(find src/ -name \*.c)
  for FILE in $FILES
  do
    if ! grep -m1 "#include" "$FILE" | grep -q "compile_time.h"
    then
      echo_warn "First include is not compile_time.h: $FILE"
      rc=1
    fi

    INCLUDES=$(grep "#include \"" "$FILE" | grep -v "compile_time.h" | cut -d\" -f2)
    for INCLUDE in $INCLUDES
    do
      if ! realpath "$INCLUDE" > /dev/null 2>&1
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
    echo "Running clang-tidy"
    rm -f clang-tidy.out
    clang-tidy --config-file="$STARTPATH/.clang-tidy" "$FILE" > ../clang-tidy.out 2>/dev/null
    grep -v -E "(/usr/include/|memset|memcpy|_XOPEN_SOURCE|\^)" ../clang-tidy.out
  else
    echo_warn "clang-tidy not found"
  fi
}

check() {
  check_phrases
  if ! check_docs
  then
    return 1
  fi

  if ! check_includes
  then
    return 1
  fi

  if check_cmd cppcheck
  then
    echo "Running cppcheck"
    [ -z "${CPPCHECKOPTS+z}" ] && CPPCHECKOPTS="-q --force --enable=warning"
    find ./src/ -name \*.c | while read -r FILE
    do
      [ "$FILE" = "./src/mympd_api/scripts_lualibs.c" ] && continue
      [ "$FILE" = "./src/web_server/embedded_files.c" ] && continue
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
    echo "Running clang-tidy"
    rm -f clang-tidy.out
    cd src || exit 1
    find ./ -name '*.c' -exec clang-tidy \
      --config-file="$STARTPATH/.clang-tidy" {} \; >> ../clang-tidy.out 2>/dev/null
    ERRORS=$(grep -v -E "(/usr/include/|memset|memcpy|_XOPEN_SOURCE|\^)" ../clang-tidy.out)
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

  return 0
}

prepare() {
  cleanup
  SRC=$(ls -d "$STARTPATH"/* -1)
  mkdir -p package/build
  cd package/build || exit 1
  for F in $SRC
  do
    [ "$F" = "$STARTPATH/osc" ] && continue
    [ "$F" = "$STARTPATH/builder" ] && continue
    cp -a "$F" .
    rm -fr src/.cache
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
    namcap "mympd-${VERSION}"*.pkg.tar.*
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
    apt-get update
    if ! apt-get install -y --no-install-recommends liblua5.4-dev
    then
      #fallback to lua 5.3 for older debian versions
      apt-get install -y --no-install-recommends liblua5.3-dev
    fi
    apt-get install -y --no-install-recommends \
      gcc cmake perl libssl-dev libid3tag0-dev libflac-dev \
      build-essential pkg-config libpcre2-dev gzip jq whiptail
  elif [ -f /etc/arch-release ]
  then
    #arch
    pacman -Sy gcc base-devel cmake perl openssl libid3tag flac lua pkgconf pcre2 gzip jq libnewt
  elif [ -f /etc/alpine-release ]
  then
    #alpine
    apk add cmake perl openssl-dev libid3tag-dev flac-dev lua5.4-dev \
      alpine-sdk linux-headers pkgconf pcre2-dev gzip jq newt
  elif [ -f /etc/SuSE-release ]
  then
    #suse
    zypper install gcc cmake pkgconfig perl openssl-devel libid3tag-devel flac-devel \
      lua-devel unzip pcre2-devel gzip jq whiptail
  elif [ -f /etc/redhat-release ]
  then
    #fedora
    yum install gcc cmake pkgconfig perl openssl-devel libid3tag-devel flac-devel \
      lua-devel unzip pcre2-devel gzip jq whiptail
  else
    echo_warn "Unsupported distribution detected."
    echo "You should manually install:"
    echo "  - gcc or clang"
    echo "  - cmake"
    echo "  - perl"
    echo "  - gzip"
    echo "  - jq"
    echo "  - whiptail"
    echo "  - openssl (devel)"
    echo "  - flac (devel)"
    echo "  - libid3tag (devel)"
    echo "  - liblua5.4 or liblua5.3 (devel)"
    echo "  - libpcre2 (devel)"
  fi
}

updatelibmympdclient() {
  check_cmd git meson

  cd dist/libmympdclient || exit 1

  TMPDIR=$(mktemp -d)
  cd "$TMPDIR" || exit 1
  git clone --depth=1 -b libmympdclient https://github.com/jcorporation/libmympdclient.git
  cd libmympdclient || exit 1
  meson setup . output -Dbuffer_size=8192

  cd "$STARTPATH/dist/libmympdclient" || exit 1
  install -d src
  install -d include/mpd
  install -d LICENSES

  rsync -av --delete "$TMPDIR/libmympdclient/src/" ./src/
  rsync -av --delete "$TMPDIR/libmympdclient/include/mpd/" ./include/mpd/

  rsync -av "$TMPDIR/libmympdclient/output/include/mpd/version.h" include/mpd/version.h
  rsync -av "$TMPDIR/libmympdclient/output/config.h" include/config.h

  rsync -av "$TMPDIR/libmympdclient/LICENSES/" LICENSES/

  rm -rf "$TMPDIR"
}

updatebootstrapnative() {
  check_cmd git npm
  #clone repository
  TMPDIR=$(mktemp -d)
  cd "$TMPDIR" || exit 1
  git clone --depth=1 git@github.com:thednp/bootstrap.native.git
  cd bootstrap.native
  npm install vite
  #copy custom config
  cp "$STARTPATH/dist/bootstrap-native/mympd-config.ts" src/index.ts
  cp "$STARTPATH/dist/bootstrap-native/mympd-init.ts" src/util/
  #minified build
  npm run build-vite
  grep -v "^//" dist/bootstrap-native.js > "$STARTPATH/dist/bootstrap-native/bootstrap-native.min.js"
  #normal build
  sed -i 's/sourcemap: true,/sourcemap: true,\nminify: false/' vite.config.ts
  npm run build-vite
  grep -v "^//" dist/bootstrap-native.js > "$STARTPATH/dist/bootstrap-native/bootstrap-native.js"
  #cleanup
  cd "$STARTPATH" || exit 1
  rm -rf "$TMPDIR"
  #update debug build
  if [ -d debug ]
  then
    cp dist/bootstrap-native/bootstrap-native.js htdocs/js/
  fi
}

updatebootstrap() {
  check_cmd npm
  cd "$STARTPATH/dist/bootstrap" || exit 1
  [ -z "${BOOTSTRAP_VERSION+x}" ] && BOOTSTRAP_VERSION=""
  npm install "$BOOTSTRAP_VERSION"
  npm run build
  sed -i '$ d' compiled/custom.css
  rm compiled/custom.css.map
  if [ -d "$STARTPATH/debug" ]
  then
    cp -v compiled/custom.css "$STARTPATH/htdocs/css/bootstrap.css"
  fi
  rm -fr  dist/bootstrap/node_modules
}

#Also deletes stale installations in other locations.
#
uninstall() {
  #cmake does not provide an uninstall target, instead its manifest is of use at least for
  #the binaries
  if [ -f "$STARTPATH/release/install_manifest.txt" ]
  then
    xargs rm -f < "$STARTPATH/release/install_manifest.txt"
  fi
  [ -z "${DESTDIR+x}" ] && DESTDIR=""
  #CMAKE_INSTALL_PREFIX="/usr"
  rm -f "$DESTDIR/usr/bin/mympd"
  rm -f "$DESTDIR/usr/bin/mympd-script"
  rm -rf "$DESTDIR/usr/share/doc/mympd"
  rm -f "$DESTDIR/usr/share/man/man1/mympd.1.gz"
  rm -f "$DESTDIR/usr/share/man/man1/mympd-script.1.gz"
  #CMAKE_INSTALL_PREFIX="/usr/local"
  rm -f "$DESTDIR/usr/local/bin/mympd"
  rm -f "$DESTDIR/usr/local/bin/mympd-script"
  rm -rf "$DESTDIR/usr/local/share/doc/mympd"
  rm -f "$DESTDIR/usr/local/share/man/man1/mympd.1.gz"
  rm -f "$DESTDIR/usr/local/share/man/man1/mympd-script.1.gz"
  #CMAKE_INSTALL_PREFIX="/opt/mympd/"
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
  #CMAKE_INSTALL_PREFIX="/usr"
  rm -rf "$DESTDIR/var/lib/mympd"
  rm -rf "$DESTDIR/var/lib/private/mympd"
  rm -rf "$DESTDIR/var/cache/mympd"
  rm -rf "$DESTDIR/var/cache/private/mympd"
  rm -f "$DESTDIR/etc/init.d/mympd"
  rm -rf "$DESTDIR/usr/share/doc/mympd"
  rm -f "$DESTDIR/usr/share/man/man1/mympd.1.gz"
  rm -f "$DESTDIR/usr/share/man/man1/mympd-script.1.gz"
  #CMAKE_INSTALL_PREFIX="/opt/mympd/"
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
    if check_cmd_silent groupdel
    then
      groupdel mympd
    elif check_cmd_silent delgroup
    then
      #alpine
      delgroup mympd
    else
      echo_error "Can not remove group mympd"
      return 1
    fi
  fi
}

check_phrases() {
  check_cmd jq
  echo "Validating translation phrases"
  for F in "src/i18n/json/"*.json
  do
    if ! jq "." "$F" > /dev/null
    then
      echo "Invalid json: $F"
      exit 1
    fi
  done
}

# Create an JavaScript object from compile_time.h.in
# This is used to define global defaults.
create_js_defines() {
  printf "const defaults = " > "$STARTPATH/htdocs/js/defines.js"
  {
    I=0
    printf "{"
    perl -npe 's/\\\n//g; s/^\s+//g' < "$STARTPATH/src/compile_time.h.in" \
      | perl -ne 's/MPD_TAG_ARTIST/"Artist"/g; s/""//g; 
      if (not /\$/ and /^#define ((MYMPD|PARTITION)_\w+)\s+(\S+)/) {
        print "$1|$3\n";
      }
    ' | while IFS="|" read -r KEY VALUE
      do
        [ "$I" -gt 0 ] && echo ","
        if JSON_VALUE=$(printf '{"value": %s}' "$VALUE" | jq -r '.value | fromjson' 2>/dev/null)
        then
          printf '"%s":%s' "$KEY" "$JSON_VALUE"
        else
          printf '"%s":%s' "$KEY" "$VALUE"
        fi
        I=$((I+1))
      done
      printf "}"
  } | jq -r "." >> "$STARTPATH/htdocs/js/defines.js"
  echo ";" >> "$STARTPATH/htdocs/js/defines.js"
}

createi18n() {
  check_cmd perl
  install -d "$MYMPD_BUILDDIR/htdocs/js"
  echo "Creating i18n json"
  if ! perl ./src/i18n/translate.pl
  then
    echo "Error creating translation files"
    exit 1
  fi
  #json to js
  printf "const i18n = " > "$MYMPD_BUILDDIR/htdocs/js/i18n.js"
  BYTES=$(wc -c < src/i18n/json/i18n.json)
  BYTES=$((BYTES-1))
  head -c "$BYTES" "src/i18n/json/i18n.json" >> "$MYMPD_BUILDDIR/htdocs/js/i18n.js"
  echo ";" >> "$MYMPD_BUILDDIR/htdocs/js/i18n.js"
  #Update serviceworker
  TO_CACHE=""
  for CODE in $(jq -r "select(.missingPhrases < 100) | keys[]" "$STARTPATH/src/i18n/json/i18n.json" | grep -v "default")
  do
    TO_CACHE="${TO_CACHE}\nsubdir + '/assets/i18n/${CODE}.json',"
  done
  sed -e "s|__VERSION__|${VERSION}|g" -e "s|__I18NASSETS__|${TO_CACHE}|g" htdocs/sw.js.in > htdocs/sw.js
}

materialicons() {
  check_cmd jq wget woff2_compress

  TMPDIR=$(mktemp -d)
  cd "$TMPDIR" || exit 1
  FONT_URI=$(wget -q "https://fonts.googleapis.com/css2?family=Material+Icons" -O - | \
    grep url | cut -d\( -f2 | cut -d\) -f1)
  if ! wget -q "$FONT_URI" -O MaterialIcons-Regular.woff2
  then
    echo_error "Error downloading font file"
    exit 1
  fi
  if ! woff2_compress MaterialIcons-Regular.woff2
  then
    echo_error "Compression failed"
    exit 1
  fi
  METADATA_URI="https://fonts.google.com/metadata/icons"
  if ! wget -q "$METADATA_URI" -O metadata.json
  then
    echo_error "Error downloading metadata"
    exit 1
  fi
  sed -i '1d' metadata.json
  printf "{" > "ligatures.json"
  I=0
  for CAT in $(jq -r ".icons[].categories | .[]" < metadata.json | sort -u)
  do
    [ "$I" -gt 0 ] && printf "," >> "ligatures.json"
    printf "\"%s\":[" "$CAT" >> "ligatures.json"
    J=0
    for ICON in $(jq -r ".icons[] | select(.categories[]==\"$CAT\") | .name" < metadata.json)
    do
      [ "$J" -gt 0 ] && printf "," >> "ligatures.json"
      printf "\"%s\"" "$ICON" >> "ligatures.json"
      J=$((J+1))
    done
    printf "]" >> "ligatures.json"
    I=$((I+1))
  done
  echo "}"  >> "ligatures.json"
  cp ligatures.json "$STARTPATH/dist/material-icons/"
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

run_tsc() {
  if ! check_cmd npx
  then
    return 1
  fi
  echo "Running typscript compiler for validation"
  if ! npx tsc -p htdocs/js/jsconfig.json
  then
    return 1
  fi
  return 0
}

run_checkjs() {
  echo "Check for defined javascript functions"
  if ! linter/checkjs.pl
  then
    return 1
  fi
  return 0
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
    if ! npx eslint $F
    then
      rc=1
    fi
  done
  echo "Check for forbidden js functions"
  FORBIDDEN_CMDS="innerHTML outerHTML insertAdjacentHTML innerText getElements"
  for F in $FORBIDDEN_CMDS
  do
    if grep -q "$F" release/htdocs/js/mympd.min.js
    then
      echo_error "Found $F usage"
      rc=1
    fi
  done
  echo "Check for subdir usage"
  if grep -q -P "subdir\s*\+\s*\'[^/]" htdocs/js/*.js
  then
    echo_error "Wrong path found"
    rc=1
  fi
  return "$rc"
}

run_stylelint() {
  if ! check_cmd npx
  then
    return 1
  fi
  rc=0
  for F in mympd.css theme-light.css theme-dark.css
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

run_doxygen() {
  if ! check_cmd doxygen
  then
    return 1
  fi
  echo "Running doxygen"
  doxygen
}

run_jsdoc() {
  if ! check_cmd jsdoc
  then
    return 1
  fi
  echo "Running jsdoc"
  jsdoc htdocs/js/ -c jsdoc.json -d docs/jsdoc/
}

create_doc() {
  DOC_DEST=$1
  if ! check_cmd jekyll
  then
    echo "Jekyll not installed, can not create documentation"
    return 1
  fi
  if ! run_doxygen
  then
    echo "Skipped generation of c api documentation"
  fi
  if ! run_jsdoc
  then
    echo "Skipped generation of js api documentation"
  fi
  install -d "$DOC_DEST" || return 1
  jekyll build -s "$STARTPATH/docs" -d "$DOC_DEST"
}

translation_import() {
  #shellcheck disable=SC1091
  . "$STARTPATH/.secrets"
  if [ -z "${POEDITOR_TOKEN+x}" ]
  then
    echo_error "POEDITOR_TOKEN variable not set."
    exit 1
  fi
  LANG_SHORT=$1
  LANG_CODE=$2
  if RESPONSE=$(curl -s -f --show-error -X POST https://api.poeditor.com/v2/projects/export \
    -d api_token="$POEDITOR_TOKEN" \
    -d id="550213" \
    -d language="$LANG_SHORT" \
    -d type="key_value_json" \
    -d order="terms")
  then
    URL=$(echo "$RESPONSE" | jq -r ".result.url")
    if [ "$URL" != "null" ]
    then
      if curl -s -f --show-error "$URL" -o "src/i18n/json/$LANG_CODE.tmp"
      then
        mv "src/i18n/json/$LANG_CODE.tmp" "src/i18n/json/$LANG_CODE.json"
        return 0
      fi
    else
      echo_error "Download failed: $RESPONSE"
      return 1
    fi
  fi
  return 1
}

translation_import_all() {
  #shellcheck disable=SC1091
  . "$STARTPATH/.secrets"
  if [ -z "${POEDITOR_TOKEN+x}" ]
  then
    echo_error "POEDITOR_TOKEN variable not set."
    exit 1
  fi
  while IFS=":" read -r LANG_SHORT LANG_CODE LANG_DESC
  do
    [ "$LANG_CODE" = "en-US" ] && continue
    echo "$LANG_SHORT: $LANG_DESC"
    translation_import "$LANG_SHORT" "$LANG_CODE"
  done < src/i18n/i18n.txt
}

terms_export() {
  #shellcheck disable=SC1091
  . "$STARTPATH/.secrets"
  if [ -z "${POEDITOR_TOKEN+x}" ]
  then
    echo_error "POEDITOR_TOKEN variable not set."
    exit 1
  fi
  curl -s -f --show-error -X POST https://api.poeditor.com/v2/projects/upload \
    -F api_token="$POEDITOR_TOKEN" \
    -F id="550213" \
    -F updating="terms" \
    -F sync_terms="1" \
    -F file=@"src/i18n/json/phrases.json"
  echo ""
}

translation_export() {
  #shellcheck disable=SC1091
  . "$STARTPATH/.secrets"
  if [ -z "${POEDITOR_TOKEN+x}" ]
  then
    echo_error "POEDITOR_TOKEN variable not set."
    exit 1
  fi
  LANG_SHORT=$1
  UPLOAD_FILE=$2
  curl -s -f --show-error -X POST https://api.poeditor.com/v2/projects/upload \
    -F api_token="$POEDITOR_TOKEN" \
    -F id="550213" \
    -F updating="translations" \
    -F language="$LANG_SHORT" \
    -F file=@"$UPLOAD_FILE"
  echo ""
}

case "$ACTION" in
  release|MinSizeRel)
    buildrelease "Release"
  ;;
  RelWithDebInfo)
    buildrelease "RelWithDebInfo"
  ;;
  install)
    installrelease
  ;;
  releaseinstall)
    buildrelease
    installrelease
  ;;
  debug|asan|tsan|ubsan)
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
  terms_export)
    terms_export
    ;;
  trans_export)
    if [ -z "${2+x}" ] || [ -z "${3+x}" ]
    then
      echo "Usage: $0 $1 <code short> <file>"
      echo "  e.g. $0 $1 de src/i18n/json/de-DE.json"
      exit 1
    fi
    translation_export  "$2" "$3"
    ;;
  trans_import)
    if [ -z "${2+x}" ] || [ -z "${3+x}" ]
    then
      echo "Usage: $0 $1 <code short> <lang code full>"
      echo "  e.g. $0 $1 de de-DE"
      exit 1
    fi
    translation_import "$2" "$3"
  ;;
  trans_import_all)
    translation_import_all
  ;;
  translate)
    src/i18n/translate.pl verbose
  ;;
  transstatus)
    src/i18n/translate.pl
  ;;
  materialicons)
    materialicons
  ;;
  createassets)
    createassets
  ;;
  copyassets)
    copyassets
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
    if ! run_eslint
    then
      exit 1
    fi
    if ! run_stylelint
    then
      exit 1
    fi
    if ! run_tsc
    then
      exit 1
    fi
    if ! run_checkjs
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
  api_doc)
    if ! run_doxygen
    then
      echo "Could not create backend api documentation"
      exit 1
    fi
    if ! run_jsdoc
    then
      echo "Could not create frontend api documentation"
      exit 1
    fi
    cp -v htdocs/js/apidoc.js docs/assets/apidoc.js
  ;;
  doc)
    if [ -z "${2+x}" ]
    then
      echo "Usage: $0 $1 <destination folder>"
      exit 1
    fi
    create_doc "$2"
    ;;
  cloc)
    cloc --exclude-dir=dist .
  ;;
  *)
    echo "Usage: $0 <option>"
    echo "Version: ${VERSION}"
    echo ""
    echo "Build options:"
    echo "  release:          build release files in directory release (stripped)"
    echo "  RelWithDebInfo:   build release files in directory release (with debug info)"
    echo "  install:          installs release files from directory release"
    echo "                    following environment variables are respected"
    echo "                      - DESTDIR=\"\""
    echo "  releaseinstall:   calls release and install afterwards"
    echo "  debug:            builds debug files in directory debug,"
    echo "                    serves assets from htdocs"
    echo "  asan|tsan|ubsan:  builds debug files in directory debug"
    echo "                    linked with the sanitizer and serves assets from htdocs"
    echo "  test:             builds and runs the unit tests in directory debug"
    echo "                    linked with libasan3"
    echo "  installdeps:      installs build and runtime dependencies"
    echo "  createassets:     creates the minfied and compressed dist files"
    echo "                    following environment variables are respected"
    echo "                      - MYMPD_BUILDDIR=\"release\""
    echo "  copyassets:       copies the assets from dist to the source tree"
    echo "                    for debug builds without embedded assets"
    echo "                    following environment variables are respected"
    echo "                      - MYMPD_BUILDDIR=\"debug\""
    echo ""
    echo "Translation options:"
    echo "  translate:        builds the translation file for debug builds"
    echo "  transstatus:      shows the translation status"
    echo "  trans_import:     Import a translation from poeditor.com"
    echo "  trans_import_all: Import all translations from poeditor.com"
    echo "  trans_export:     Exports a translation to poeditor.com"
    echo "  terms_export:     Exports the terms to poeditor.com"
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
    echo "  pkgdocker:        creates the docker image (Alpine Linux based)"
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
    echo "                      - DEBOOTSTRAP=\"debootstrap\""
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
    echo "  api_doc:          generates the api documentation"
    echo "  doc:              generates the html documentation"
    echo "  cloc:             runs cloc (count lines of code)"
    echo ""
    echo "Source update options:"
    echo "  bootstrap:        updates bootstrap"
    echo "  bootstrapnative:  updates bootstrap.native"
    echo "  libmympdclient:   updates libmympdclient (fork of libmpdclient)"
    echo "  materialicons:    updates the materialicons json"
    echo "  setversion:       sets version and date in packaging files from CMakeLists.txt"
    echo ""
    exit 1
  ;;
esac

exit 0
