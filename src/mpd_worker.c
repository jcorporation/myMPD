/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h>

#include <mpd/client.h>

#include "../dist/src/sds/sds.h"
#include "../dist/src/rax/rax.h"
#include "sds_extras.h"
#include "log.h"
#include "list.h"
#include "mympd_config_defs.h"
#include "mympd_state.h"
#include "tiny_queue.h"
#include "api.h"
#include "global.h"
#include "utility.h"
#include "mympd_state.h"
#include "mpd_shared.h"
#include "mpd_shared/mpd_shared_tags.h"
#include "mpd_shared/mpd_shared_sticker.h"
#include "mpd_worker/mpd_worker_utility.h"
#include "mpd_worker/mpd_worker_api.h"
#include "mpd_worker/mpd_worker_smartpls.h"
#include "mpd_worker/mpd_worker_cache.h"
#include "mpd_worker.h"

//private definitions
static void *mpd_worker_run(void *arg);
static bool mpd_worker_connect(struct t_mpd_worker_state *mpd_worker_state);

//public functions
bool mpd_worker_start(struct t_mympd_state *mympd_state, t_work_request *request) {
    MYMPD_LOG_NOTICE("Starting mpd_worker thread for %s", request->method);
    pthread_t mpd_worker_thread;
    pthread_attr_t attr;
    if (pthread_attr_init(&attr) != 0) {
        MYMPD_LOG_ERROR("Can not init mpd_worker thread attribute");
        return false;
    }
    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0) {
        MYMPD_LOG_ERROR("Can not set mpd_worker thread to detached");
        return false;
    }
    //create mpd worker state from mympd_state
    struct t_mpd_worker_state *mpd_worker_state = (struct t_mpd_worker_state *)malloc(sizeof(struct t_mpd_worker_state));
    assert(mpd_worker_state);
    mpd_worker_state->request = request;
    mpd_worker_state->smartpls = mympd_state->smartpls;
    mpd_worker_state->smartpls_sort = sdsdup(mympd_state->smartpls_sort);
    mpd_worker_state->smartpls_prefix = sdsdup(mympd_state->smartpls_prefix);
    mpd_worker_state->config = mympd_state->config;
    reset_t_tags(&mpd_worker_state->generate_pls_tag_types);
    check_tags(mympd_state->generate_pls_tags, "generate pls tags", &mpd_worker_state->generate_pls_tag_types, mympd_state->mpd_state->mympd_tag_types);

    //mpd state
    mpd_worker_state->mpd_state = (struct t_mpd_state *)malloc(sizeof(struct t_mpd_state));
    assert(mpd_worker_state->mpd_state);
    mpd_shared_default_mpd_state(mpd_worker_state->mpd_state);
    mpd_worker_state->mpd_state->mpd_host = sdsreplace(mpd_worker_state->mpd_state->mpd_host, mympd_state->mpd_state->mpd_host);
    mpd_worker_state->mpd_state->mpd_port = mympd_state->mpd_state->mpd_port;
    mpd_worker_state->mpd_state->mpd_pass = sdsreplace(mpd_worker_state->mpd_state->mpd_pass, mympd_state->mpd_state->mpd_pass);
    mpd_worker_state->mpd_state->feat_tags = mympd_state->mpd_state->feat_tags;
    mpd_worker_state->mpd_state->feat_stickers = mympd_state->mpd_state->feat_stickers;
    mpd_worker_state->mpd_state->feat_playlists = mympd_state->mpd_state->feat_playlists;
    
    if (pthread_create(&mpd_worker_thread, &attr, mpd_worker_run, mpd_worker_state) != 0) {
        MYMPD_LOG_ERROR("Can not create mpd_worker thread");
        free_mpd_worker_state(mpd_worker_state);
        return false;
    }
    pthread_setname_np(mpd_worker_thread, "mpd_worker");
    worker_threads++;
    return true;
}

static void *mpd_worker_run(void *arg) {
    thread_logname = sdsreplace(thread_logname, "mpdworker");
    struct t_mpd_worker_state *mpd_worker_state = (struct t_mpd_worker_state *) arg;

    if (mpd_worker_connect(mpd_worker_state) == true) {
        mpd_worker_api(mpd_worker_state);
        mpd_shared_mpd_disconnect(mpd_worker_state->mpd_state);
    }
    MYMPD_LOG_NOTICE("Stopping mpd_worker thread");
    sdsfree(thread_logname);
    free_mpd_worker_state(mpd_worker_state);
    worker_threads--;
    return NULL;
}

static bool mpd_worker_connect(struct t_mpd_worker_state *mpd_worker_state) {
    if (strncmp(mpd_worker_state->mpd_state->mpd_host, "/", 1) == 0) {
        MYMPD_LOG_NOTICE("MPD worker connecting to socket %s", mpd_worker_state->mpd_state->mpd_host);
    }
    else {
        MYMPD_LOG_NOTICE("MPD worker connecting to %s:%d", mpd_worker_state->mpd_state->mpd_host, mpd_worker_state->mpd_state->mpd_port);
    }
    mpd_worker_state->mpd_state->conn = mpd_connection_new(mpd_worker_state->mpd_state->mpd_host, mpd_worker_state->mpd_state->mpd_port, mpd_worker_state->mpd_state->timeout);
    if (mpd_worker_state->mpd_state->conn == NULL) {
        MYMPD_LOG_ERROR("MPD worker connection to failed: out-of-memory");
        mpd_worker_state->mpd_state->conn_state = MPD_FAILURE;
        mpd_connection_free(mpd_worker_state->mpd_state->conn);
        return false;
    }
    if (mpd_connection_get_error(mpd_worker_state->mpd_state->conn) != MPD_ERROR_SUCCESS) {
        MYMPD_LOG_ERROR("MPD worker connection: %s", mpd_connection_get_error_message(mpd_worker_state->mpd_state->conn));
        mpd_worker_state->mpd_state->conn_state = MPD_FAILURE;
        return false;
    }
    if (sdslen(mpd_worker_state->mpd_state->mpd_pass) > 0 && !mpd_run_password(mpd_worker_state->mpd_state->conn, mpd_worker_state->mpd_state->mpd_pass)) {
        MYMPD_LOG_ERROR("MPD worker connection: %s", mpd_connection_get_error_message(mpd_worker_state->mpd_state->conn));
        mpd_worker_state->mpd_state->conn_state = MPD_FAILURE;
        return false;
    }

    MYMPD_LOG_NOTICE("MPD worker connected");
    mpd_connection_set_timeout(mpd_worker_state->mpd_state->conn, mpd_worker_state->mpd_state->timeout);
    mpd_worker_state->mpd_state->conn_state = MPD_CONNECTED;
    return true;
}
