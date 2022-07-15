/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"

#include "../dist/utest/utest.h"
#include "../src/lib/log.h"
#include "../src/lib/sds_extras.h"

_Thread_local sds thread_logname;

//message queues
struct t_mympd_queue *web_server_queue;
struct t_mympd_queue *mympd_api_queue;
struct t_mympd_queue *mympd_script_queue;

UTEST_STATE();

int main(int argc, const char *const argv[]) {
    thread_logname = sdsempty();
    //set_loglevel(5);
    int rc = utest_main(argc, argv);
    FREE_SDS(thread_logname);
    return rc;
}
