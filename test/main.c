/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"

#include "dist/utest/utest.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"

#include <sys/stat.h>
#include <unistd.h>

UTEST_STATE();

sds workdir;

int main(int argc, const char *const argv[]) {
    thread_logname = sdsempty();
    set_loglevel(7);
    log_type = LOG_TO_STDOUT;
    workdir = sdsnew("/tmp/mympd-test");

    //utest main
    int rc = utest_main(argc, argv);

    //cleanup
    FREE_SDS(thread_logname);
    sdsfree(workdir);
    return rc;
}
