/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Smart playlist implementation
 */

#ifndef MYMPD_SMARTPLS_H
#define MYMPD_SMARTPLS_H

#include "dist/sds/sds.h"

#include <stdbool.h>

bool smartpls_save_sticker(sds workdir, const char *playlist, const char *sticker,
        const char *value, const char *op, const char *sort, bool sortdesc, int max_entries);
bool smartpls_save_newest(sds workdir, const char *playlist, unsigned timerange,
        const char *sort, bool sortdesc, int max_entries);
bool smartpls_save_search(sds workdir, const char *playlist, const char *expression,
        const char *sort, bool sortdesc, int max_entries);

bool smartpls_update(const char *playlist, unsigned long conn_id, unsigned int request_id);
bool smartpls_update_all(void);

bool is_smartpls(sds workdir, const char *playlist);
time_t smartpls_get_mtime(sds workdir, const char *playlist);
#endif
