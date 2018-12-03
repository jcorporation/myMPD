# Maintainer: Juergen Mang <mail [at] jcgames [dot] de>
# Website: https://github.com/jcorporation/myMPD
# PKGBUILD Based on https://github.com/CultofRobots/archphile-custom/tree/master/mympd

pkgname=mympd
_pkgname=myMPD
pkgver=4.7.1
pkgrel=1
pkgdesc="myMPD is a standalone and mobile friendly web mpdclient."
arch=('x86_64' 'armv7h' 'aarch64')
url="http://github.org/jcorporation/myMPD"
license=('GPL')
depends=('libmpdclient' 'openssl')
makedepends=('cmake')
optdepends=()
provides=()
conflicts=()
replaces=()
install=contrib/archlinux.install
source=("https://github.com/jcorporation/${_pkgname}/archive/v${pkgver}.tar.gz")
sha256sums=('SKIP')

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
