# Copyright 1999-2018 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2

# Copyright 2020 Gentoo Authors
# Distributed under the terms of the GNU General Public License v2

EAPI=6

inherit eutils user cmake-utils

MY_PN="myMPD"

DESCRIPTION="myMPD is a standalone and mobile friendly web mpdclient"
HOMEPAGE="https://github.com/jcorporation/myMPD"
SRC_URI="https://github.com/jcorporation/${MY_PN}/archive/v${PV}.tar.gz"

LICENSE="GPL-2"
SLOT="0"
KEYWORDS="~amd64 ~x86 ~arm ~arm64"
IUSE=""

DEPEND=""
RDEPEND="${DEPEND}"
BDEPEND=""

S="${WORKDIR}/${MY_PN}-${PV}"

pkg_setup() {
    enewgroup mympd
    enewuser mympd -1 -1 -1 audio
}

src_compile() {
    default
    ./build.sh release
}

src_install() {
    cd release
    dobin mympd
    dobin cli_tools/mympd-config
    newinitd "${FILESDIR}/${PN}.init.d" "${PN}"
    insinto /etc
    newins "${FILESDIR}/${PN}.conf" "${PN}.conf"
}
