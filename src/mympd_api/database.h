/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_DATABASE_H
#define MYMPD_API_DATABASE_H

#include "dist/sds/sds.h"
#include "src/lib/api.h"
#include "src/lib/mympd_state.h"

#include <stdbool.h>

sds mympd_api_database_update(struct t_partition_state *partition_state, sds buffer, enum mympd_cmd_ids cmd_id, long request_id, sds path);

#endif
