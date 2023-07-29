# Copyright 1999-2023 Gentoo Authors
# Distributed under the terms of the GNU General Public License v2

EAPI=8

inherit acct-user

DESCRIPTION="User for media-sound/mympd"
ACCT_USER_ID=-1
ACCT_USER_GROUPS=( mympd audio )

acct-user_add_deps
