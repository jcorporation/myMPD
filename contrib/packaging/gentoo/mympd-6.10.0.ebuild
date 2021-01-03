# Copyright 1999-2021 Gentoo Authors
# Distributed under the terms of the GNU General Public License v2

EAPI=7

inherit eutils user cmake systemd

MY_PN="myMPD"
S="${WORKDIR}/${MY_PN}-${PV}"

DESCRIPTION="myMPD is a standalone and mobile friendly web-based MPD client"
HOMEPAGE="https://jcorporation.github.io/myMPD"
SRC_URI="https://github.com/jcorporation/${MY_PN}/archive/v${PV}.tar.gz -> ${PN}-${PV}.tar.gz"

LICENSE="GPL-2"
SLOT="0"
KEYWORDS="~amd64 ~x86 ~arm ~arm64"
IUSE="+flac +id3tag +ssl +java +lua systemd"

BDEPEND="
        >=dev-util/cmake-3.4
        java? ( >=virtual/jre-1.7 )
        dev-lang/perl
"

RDEPEND="
        id3tag? (media-libs/libid3tag )
	flac? (media-libs/flac )
        ssl? ( >=dev-libs/openssl-1.1 )
        lua? ( >=dev-lang/lua-5.3 )
        systemd? ( sys-apps/systemd )
"

QA_PRESTRIPPED="
	usr/bin/mympd
	usr/bin/mympd-config
	usr/bin/mympd-script
"

src_compile() {
    default
    export ENABLE_SSL=$(usex ssl "ON" "OFF")
    export ENABLE_LIBID3TAG=$(usex id3tag "ON" "OFF")
    export ENABLE_FLAC=$(usex flac "ON" "OFF")
    export ENABLE_LUA=$(usex lua "ON" "OFF")
    ./build.sh release || die
}

src_install() {
    cd release
    dobin mympd
    dobin cli_tools/mympd-config
    if use lua; then
	dobin cli_tools/mympd-script
    fi
    newinitd "contrib/initscripts/mympd.openrc" "${PN}"
    if use systemd; then
        systemd_newunit contrib/initscripts/mympd.service mympd.service
    fi
    ${D}/usr/bin/mympd-config --mympdconf ${D}/etc/mympd.conf
    dodoc ${S}/README.md
}

pkg_postinst() {
    elog "Checking status of mympd system user and group"
    getent group mympd > /dev/null || enewgroup mympd
    getent passwd mympd > /dev/null || enewuser mympd -1 -1 -1 audio

    elog
    elog "myMPD installed"
    elog "Modify /etc/mympd.conf to suit your needs or use the"
    elog "\`mympd-config\` tool to generate a valid mympd.conf automatically."
    elog
}
