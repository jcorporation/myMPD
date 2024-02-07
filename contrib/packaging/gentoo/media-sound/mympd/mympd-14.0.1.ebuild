# Copyright 1999-2023 Gentoo Authors
# Distributed under the terms of the GNU General Public License v2

EAPI=8

inherit cmake systemd

MY_PN="myMPD"

DESCRIPTION="myMPD is a standalone and mobile friendly web-based MPD client"
HOMEPAGE="https://jcorporation.github.io/myMPD"
SRC_URI="https://github.com/jcorporation/${MY_PN}/archive/v${PV}.tar.gz -> ${PN}-${PV}.tar.gz"

LICENSE="GPL-3"
SLOT="0"
KEYWORDS="~amd64 ~x86 ~arm ~arm64"
IUSE="+flac +id3tag +lua systemd"

BDEPEND="
    >=dev-build/cmake-3.13
    dev-lang/perl
    app-misc/jq"

RDEPEND="
    acct-group/mympd
    acct-user/mympd
    id3tag? ( media-libs/libid3tag )
    flac? ( media-libs/flac )
    >=dev-libs/openssl-1.1
    lua? ( >=dev-lang/lua-5.3 )
    systemd? ( sys-apps/systemd )
    dev-libs/libpcre2"

QA_PRESTRIPPED="
    usr/bin/mympd
    usr/bin/mympd-script"

S="${WORKDIR}/${MY_PN}-${PV}"

src_compile() {
    default
    MYMPD_ENABLE_LIBID3TAG=$(usex id3tag "ON" "OFF")
    MYMPD_ENABLE_FLAC=$(usex flac "ON" "OFF")
    MYMPD_ENABLE_LUA=$(usex lua "ON" "OFF")
    cmake -B release -DCMAKE_INSTALL_PREFIX:PATH=/usr -DCMAKE_BUILD_TYPE=Release \
        -DMYMPD_ENABLE_LIBID3TAG=$MYMPD_ENABLE_LIBID3TAG \
        -DMYMPD_ENABLE_FLAC=$MYMPD_ENABLE_FLAC -DMYMPD_ENABLE_LUA=$MYMPD_ENABLE_LUA .
    make -C release || die
}

src_install() {
    cd release
    dobin bin/mympd
    dobin bin/mympd-config
    if use lua; then
        dobin bin/mympd-script
    fi
    newinitd "contrib/initscripts/mympd.openrc" "${PN}"
    if use systemd; then
        systemd_newunit contrib/initscripts/mympd.service mympd.service
    fi
    dodoc ${S}/README.md
}

pkg_postinst() {
    elog
    elog "myMPD installed"
    elog
}
