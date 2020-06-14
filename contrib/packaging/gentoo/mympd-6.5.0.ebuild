# Copyright 1999-2020 Gentoo Authors
# Distributed under the terms of the GNU General Public License v2

EAPI=7

inherit eutils user cmake-utils systemd

MY_PN="myMPD"

DESCRIPTION="myMPD is a standalone and mobile friendly web mpdclient"
HOMEPAGE="https://github.com/jcorporation/myMPD"
SRC_URI="https://github.com/jcorporation/${MY_PN}/archive/v${PV}.tar.gz -> ${PN}-${PV}.tar.gz"

LICENSE="GPL-2"
SLOT="0"
KEYWORDS="~amd64 ~x86 ~arm ~arm64"
IUSE="+flac +id3tag +ssl +java +lua systemd"

BDEPEND="
        dev-util/cmake
        java? ( virtual/jre )
        dev-lang/perl
"
RDEPEND="
        media-sound/mpd[flac?,id3tag?]
        ssl? ( dev-libs/openssl )
        lua? ( dev-lang/lua )
        systemd? ( sys-apps/systemd )
"

S="${WORKDIR}/${MY_PN}-${PV}"

pkg_setup() {
    enewgroup mympd
    enewuser mympd -1 -1 -1 audio
}

src_compile() {
    default
    ENABLE_SSL=$(usex ssl "ON" "OFF")
    ENABLE_LIBID3TAG=$(usex id3tag "ON" "OFF")
    ENABLE_FLAC=$(usex flac "ON" "OFF")
    ENABLE_FLAC=$(usex lua "ON" "OFF")
    ./build.sh release
}

src_install() {
    cd release
    dobin mympd
    dobin cli_tools/mympd-config
    newinitd "contrib/initscripts/mympd.openrc" "${PN}"
    if use systemd; then
        systemd_newunit contrib/initscripts/mympd.service mympd.service
    fi
    ${D}/usr/bin/mympd-config --mympdconf ${D}/etc/mympd.conf
    dodoc ${S}/README.md
}

pkg_postinst() {
    elog
    elog "Adapt the configuration file /etc/mympd.conf to your needs or use the"
    elog "\`mympd-config\` tool to generate automatically a valid mympd.conf"
    elog
}
