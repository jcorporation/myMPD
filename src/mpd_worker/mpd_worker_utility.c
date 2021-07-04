/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <assert.h>
#include <stdlib.h>
#include <libgen.h>
#include <pthread.h>
#include <string.h>
#include <inttypes.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../dist/src/rax/rax.h"
#include "../list.h"
#include "mympd_config_defs.h"
#include "../tiny_queue.h"
#include "../api.h"
#include "../global.h"
#include "../utility.h"
#include "../log.h"
#include "../mympd_state.h"
#include "../mpd_shared.h"
#include "mpd_worker_utility.h"

void free_mpd_worker_state(struct t_mpd_worker_state *mpd_worker_state) {
    sdsfree(mpd_worker_state->smartpls_sort);
    sdsfree(mpd_worker_state->smartpls_prefix);
    //mpd state
    mpd_shared_free_mpd_state(mpd_worker_state->mpd_state);
    free(mpd_worker_state);
}
