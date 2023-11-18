/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_OUTPUTS_H
#define MYMPD_API_OUTPUTS_H

#include "src/lib/mympd_state.h"

bool mympd_api_output_toggle(struct t_partition_state *partition_state, unsigned output_id, unsigned state, sds *error);
sds mympd_api_output_get(struct t_partition_state *partition_state, sds buffer, unsigned request_id, sds output_name);
sds mympd_api_output_list(struct t_partition_state *partition_state, sds buffer, unsigned request_id);
bool mympd_api_output_attributes_set(struct t_partition_state *partition_state,
        unsigned output_id, struct t_list *attributes, sds *error);
#endif
