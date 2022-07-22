/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_SMARTPLS_H
#define MYMPD_SMARTPLS_H

#include "../../dist/sds/sds.h"

#include <stdbool.h>

bool smartpls_save_sticker(sds workdir, sds playlist, sds sticker, int max_entries, int min_value, sds sort);
bool smartpls_save_newest(sds workdir, sds playlist, int timerange, sds sort);
bool smartpls_save_search(sds workdir, sds playlist, sds expression, sds sort);

void smartpls_update(const char *playlist);
void smartpls_update_all(void);
bool smartpls_default(sds workdir);

bool is_smartpls(sds workdir, const char *playlist);
time_t smartpls_get_mtime(sds workdir, const char *playlist);
#endif