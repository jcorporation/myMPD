

/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Systemd helpers
 */

#ifndef MYMPD_SYSTEMD_H
#define MYMPD_SYSTEMD_H

void systemd_notify_ready(void);
void systemd_notify_stopping(void);

#endif
