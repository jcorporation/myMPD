/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

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
#include "../sds_extras.h"
#include "../../dist/src/frozen/frozen.h"
#include "../list.h"
#include "../mpd_shared.h"
#include "config_defs.h"
#include "../tiny_queue.h"
#include "../api.h"
#include "../global.h"
#include "../utility.h"
#include "../log.h"
#include "mpd_worker_utility.h"

void default_mpd_worker_state(t_mpd_worker_state *mpd_worker_state) {
    //mpd state
    mpd_worker_state->mpd_state = (t_mpd_state *)malloc(sizeof(t_mpd_state));
    mpd_shared_default_mpd_state(mpd_worker_state->mpd_state);
}

void free_mpd_worker_state(t_mpd_worker_state *mpd_worker_state) {
    //mpd state
    mpd_shared_free_mpd_state(mpd_worker_state->mpd_state);
    free(mpd_worker_state);
    mpd_worker_state = NULL;
}
