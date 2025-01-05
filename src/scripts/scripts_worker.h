/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Script worker
 */

#ifndef MYMPD_SCRIPTS_WORKER_H
#define MYMPD_SCRIPTS_WORKER_H

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

void *script_run(void *script_thread_arg);

#endif
