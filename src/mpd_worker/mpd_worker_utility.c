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
#include "config_defs.h"
#include "../tiny_queue.h"
#include "../api.h"
#include "../global.h"
#include "../utility.h"
#include "../log.h"
#include "../mpd_shared/mpd_shared_typedefs.h"
#include "../mpd_shared/mpd_shared_tags.h"
#include "../mpd_shared/mpd_shared_features.h"
#include "../mpd_shared.h"
#include "mpd_worker_utility.h"

void default_mpd_worker_state(t_mpd_worker_state *mpd_worker_state) {
    mpd_worker_state->smartpls_sort = sdsempty();
    mpd_worker_state->smartpls_prefix = sdsempty();
    mpd_worker_state->generate_pls_tags = sdsempty();
    reset_t_tags(&mpd_worker_state->generate_pls_tag_types);
    //mpd state
    mpd_worker_state->mpd_state = (t_mpd_state *)malloc(sizeof(t_mpd_state));
    mpd_shared_default_mpd_state(mpd_worker_state->mpd_state);
}

void free_mpd_worker_state(t_mpd_worker_state *mpd_worker_state) {
    sdsfree(mpd_worker_state->smartpls_sort);
    sdsfree(mpd_worker_state->smartpls_prefix);
    sdsfree(mpd_worker_state->generate_pls_tags);
    //mpd state
    mpd_shared_free_mpd_state(mpd_worker_state->mpd_state);
    free(mpd_worker_state);
    mpd_worker_state = NULL;
}

void mpd_worker_features(t_mpd_worker_state *mpd_worker_state) {
    mpd_worker_state->mpd_state->feat_mpd_searchwindow = mpd_shared_feat_mpd_searchwindow(mpd_worker_state->mpd_state);
    mpd_worker_state->mpd_state->feat_advsearch = mpd_shared_feat_advsearch(mpd_worker_state->mpd_state);

    reset_t_tags(&mpd_worker_state->generate_pls_tag_types);

    mpd_shared_feat_tags(mpd_worker_state->mpd_state);

    if (mpd_worker_state->mpd_state->feat_tags == true) {
        check_tags(mpd_worker_state->generate_pls_tags, "generate pls tags", &mpd_worker_state->generate_pls_tag_types, mpd_worker_state->mpd_state->mympd_tag_types);
    }
}
