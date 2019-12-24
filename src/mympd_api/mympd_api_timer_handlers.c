/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdbool.h>
#include <stdlib.h>

#include "../dist/src/sds/sds.h"
#include "../log.h"
#include "../list.h"
#include "config_defs.h"
#include "../maintenance.h"
#include "mympd_api_utility.h"
#include "mympd_api_timer_handlers.h"

void timer_handler_covercache(void *user_data) {
    LOG_DEBUG("Start timer_handler_covercache");
    t_config *config = (t_config *) user_data;
    clear_covercache(config, -1);
}
