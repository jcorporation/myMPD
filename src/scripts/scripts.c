/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/scripts/scripts.h"

#include "src/lib/config_def.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/msg_queue.h"
#include "src/lib/sds_extras.h"
#include "src/lib/thread.h"
#include "src/scripts/api_handler.h"
#include "src/scripts/api_scripts.h"
#include "src/scripts/api_vars.h"
#include "src/scripts/util.h"

/**
 * This is the main function for the scripts thread
 * @param arg_config void pointer to t_config struct
 */
void *scripts_loop(void *arg_config) {
    thread_logname = sds_replace(thread_logname, "scripts");
    set_threadname(thread_logname);

    // create initial scripts_state struct and set defaults
    struct t_scripts_state *scripts_state = malloc_assert(sizeof(struct t_scripts_state));
    scripts_state_default(scripts_state, (struct t_config *)arg_config);
    scripts_vars_file_read(&scripts_state->var_list, scripts_state->config->workdir);
    scripts_file_read(scripts_state);

    // thread loop
    while (s_signal_received == 0) {
        struct t_work_request *request = mympd_queue_shift(script_queue, 0, 0);
        if (request != NULL) {
            scripts_api_handler(scripts_state, request);
        }
    }
    MYMPD_LOG_DEBUG(NULL, "Stopping scripts thread");

    // save and free states
    scripts_state_save(scripts_state, true);
    FREE_SDS(thread_logname);
    return NULL;
}
