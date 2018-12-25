# Maintainer: Juergen Mang <mail [at] jcgames [dot] de>
# Website: https://github.com/jcorporation/myMPD
# PKGBUILD Based on https://github.com/CultofRobots/archphile-custom/tree/master/mympd

pkgname=mympd
_pkgname=myMPD
<<<<<<< HEAD
pkgver=4.8.0
=======
pkgver=5.0.0
>>>>>>> cd588ab72e10ee934a8ac58f4add1dc9268ed79f
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
