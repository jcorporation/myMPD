# Maintainer: K. Loz <cultofrobots [at] protonmail [dot] com>
# Based on jcorporation's myMPD mkrelease.sh 
# Website: https://github.com/jcorporation/myMPD
# PKGBUILD Based on mympd-archphile by Mike Andonov <info [at] archphile [dot] org>

pkgname=mympd
_pkgname=myMPD
pkgver=4.2.0
pkgrel=1
pkgdesc="A standalone MPD Web GUI based on YMPD - Default port set to 80"
arch=('x86_64' 'armv7h' 'aarch64')
url="http://github.org/jcorporation/myMPD"
license=('GPL')
depends=('libmpdclient' 'openssl')
makedepends=('cmake' 'git')
optdepends=()
provides=()
conflicts=()
replaces=()
install=contrib/archlinux.install
source=("https://github.com/jcorporation/${_pkgname}/archive/v${pkgver}.tar.gz")
sha256sums=('SKIP')

prepare() {
  export java=$(which java 2> /dev/null)

  if [ -f ${srcdir}/${_pkgname}-${pkgver}/dist/buildtools/closure-compiler.jar ] && [ "$java" != "$NULL" ]
  then
    echo "Minifying javascript"
    [ ${srcdir}/${_pkgname}-${pkgver}/htdocs/js/player.js -nt ${srcdir}/${_pkgname}-${pkgver}/dist/htdocs/js/player.min.js ] && \
      java -jar ${srcdir}/${_pkgname}-${pkgver}/dist/buildtools/closure-compiler.jar ${srcdir}/${_pkgname}-${pkgver}/htdocs/js/player.js > ${srcdir}/${_pkgname}-${pkgver}/dist/htdocs/js/player.min.js
    [ ${srcdir}/${_pkgname}-${pkgver}/htdocs/js/mympd.js -nt  ${srcdir}/${_pkgname}-${pkgver}/dist/htdocs/js/mympd.min.js ] && \
      java -jar ${srcdir}/${_pkgname}-${pkgver}/dist/buildtools/closure-compiler.jar ${srcdir}/${_pkgname}-${pkgver}/htdocs/js/mympd.js > ${srcdir}/${_pkgname}-${pkgver}/dist/htdocs/js/mympd.min.js
    [ ${srcdir}/${_pkgname}-${pkgver}/htdocs/sw.js -nt ${srcdir}/${_pkgname}-${pkgver}/dist/htdocs/sw.min.js ] && \
      java -jar ${srcdir}/${_pkgname}-${pkgver}/dist/buildtools/closure-compiler.jar ${srcdir}/${_pkgname}-${pkgver}/htdocs/sw.js > ${srcdir}/${_pkgname}-${pkgver}/dist/htdocs/sw.min.js
  else
    echo "${srcdir}/${_pkgname}-${pkgver}/dist/buildtools/closure-compiler.jar not found, using non-minified files"
    [ ${srcdir}/${_pkgname}-${pkgver}/htdocs/js/player.js -nt ${srcdir}/${_pkgname}-${pkgver}/dist/htdocs/js/player.min.js ] && \
      cp ${srcdir}/${_pkgname}-${pkgver}/htdocs/js/player.js ${srcdir}/${_pkgname}-${pkgver}/dist/htdocs/js/player.min.js
    [ ${srcdir}/${_pkgname}-${pkgver}/htdocs/js/mympd.js -nt  ${srcdir}/${_pkgname}-${pkgver}/dist/htdocs/js/mympd.min.js ] && \
      cp ${srcdir}/${_pkgname}-${pkgver}/htdocs/js/mympd.js  ${srcdir}/${_pkgname}-${pkgver}/dist/htdocs/js/mympd.min.js
    [ ${srcdir}/${_pkgname}-${pkgver}/htdocs/sw.js -nt ${srcdir}/${_pkgname}-${pkgver}/dist/htdocs/sw.min.js ] && \
      cp ${srcdir}/${_pkgname}-${pkgver}/htdocs/sw.js ${srcdir}/${_pkgname}-${pkgver}/dist/htdocs/sw.min.js    
  fi

  if [ -f ${srcdir}/${_pkgname}-${pkgver}/dist/buildtools/closure-stylesheets.jar ] && [ "$java" != "$NULL" ]
  then
    echo "Minifying stylesheets"
    [ ${srcdir}/${_pkgname}-${pkgver}/htdocs/css/mympd.css -nt ${srcdir}/${_pkgname}-${pkgver}/dist/htdocs/css/mympd.min.css ] && \
      java -jar ${srcdir}/${_pkgname}-${pkgver}/dist/buildtools/closure-stylesheets.jar --allow-unrecognized-properties ${srcdir}/${_pkgname}-${pkgver}/htdocs/css/mympd.css > ${srcdir}/${_pkgname}-${pkgver}/dist/htdocs/css/mympd.min.css
  else
    echo "${srcdir}/${_pkgname}-${pkgver}/dist/buildtools/closure-stylesheets.jar not found, using non-minified files"
    [ ${srcdir}/${_pkgname}-${pkgver}/htdocs/css/mympd.css -nt ${srcdir}/${_pkgname}-${pkgver}/dist/htdocs/css/mympd.min.css ] && \
      cp ${srcdir}/${_pkgname}-${pkgver}/htdocs/css/mympd.css ${srcdir}/${_pkgname}-${pkgver}/dist/htdocs/css/mympd.min.css    
  fi

  echo "Replacing javascript and stylesheets with minified files"
  sed -e 's/mympd\.css/mympd\.min\.css/' -e 's/mympd\.js/mympd\.min\.js/' ${srcdir}/${_pkgname}-${pkgver}/htdocs/index.html > ${srcdir}/${_pkgname}-${pkgver}/dist/htdocs/index.html
  sed -e 's/mympd\.css/mympd\.min\.css/' -e 's/player\.js/player\.min\.js/' ${srcdir}/${_pkgname}-${pkgver}/htdocs/player.html > ${srcdir}/${_pkgname}-${pkgver}/dist/htdocs/player.html
  sed -i -e 's/mympd\.css/mympd\.min\.css/' -e 's/mympd\.js/mympd\.min\.js/' -e 's/player\.js/player\.min\.js/' ${srcdir}/${_pkgname}-${pkgver}/dist/htdocs/sw.min.js
  sed -i -e 's/\/sw\.js/\/sw\.min\.js/' ${srcdir}/${_pkgname}-${pkgver}/dist/htdocs/js/mympd.min.js
  echo "Minifying html"
  perl -i -pe 's/^\s*//gm; s/\s*$//gm' ${srcdir}/${_pkgname}-${pkgver}/dist/htdocs/index.html
  perl -i -pe 's/^\s*//gm; s/\s*$//gm' ${srcdir}/${_pkgname}-${pkgver}/dist/htdocs/player.html
}

build() {
  cd "${srcdir}/${_pkgname}-${pkgver}"
  
  [ -d release ] || mkdir release
  cd release
  cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr -DCMAKE_BUILD_TYPE=RELEASE ..
  make
}

package() {
  cd "${srcdir}/${_pkgname}-${pkgver}/release"
  make DESTDIR="$pkgdir/" install

  install -Dm644  "${srcdir}/${_pkgname}-${pkgver}/contrib/mympd.service" "$pkgdir/usr/lib/systemd/system/mympd.service"
  /usr/share/mympd/crcert.sh
}

