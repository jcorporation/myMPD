/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_RAX_EXTRAS_H
#define MYMPD_RAX_EXTRAS_H

#include "dist/rax/rax.h"
#include "dist/sds/sds.h"

typedef void (*rax_free_data_callback) (void *data);

void rax_insert_no_dup(rax *r, sds key, void *data);
void rax_free_data(rax *r, rax_free_data_callback free_cb);
void rax_free_sds_data(rax *r);

#endif
