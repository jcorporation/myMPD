/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"

#include "../dist/utest/utest.h"
#include "../src/lib/log.h"
#include "../src/lib/sds_extras.h"

#include <sys/stat.h>
#include <unistd.h>

_Thread_local sds thread_logname;

//message queues
struct t_mympd_queue *web_server_queue;
struct t_mympd_queue *mympd_api_queue;
struct t_mympd_queue *mympd_script_queue;

UTEST_STATE();

sds workdir;

int main(int argc, const char *const argv[]) {
    thread_logname = sdsempty();
    set_loglevel(7);
    workdir = sdsnew("/tmp/mympd-test");

    //create dirs for tests
    mkdir("/tmp/mympd-test", 0770);
    mkdir("/tmp/mympd-test/state", 0770);
    mkdir("/tmp/mympd-test/webradios", 0770);

    //utest main
    int rc = utest_main(argc, argv);

    //cleanup
    FREE_SDS(thread_logname);
    rmdir("/tmp/mympd-test/ssl");
    rmdir("/tmp/mympd-test/state");
    rmdir("/tmp/mympd-test");
    sdsfree(workdir);
    return rc;
}
