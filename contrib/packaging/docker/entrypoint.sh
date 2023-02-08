#!/bin/sh
#
# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd
#
export PUID=${PUID:-1000}
export PGID=${PGID:-1000}
echo "Adding mympd user ($PUID) and group ($PGID)"
addgroup -g $PGID mympd 2>/dev/null
adduser -u $PUID -D -H -h /var/lib/mympd -s /sbin/nologin -G mympd -g myMPD mympd 2>/dev/null

echo "Starting myMPD"
exec /usr/bin/mympd
