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

UTEST_STATE();

int main(int argc, const char *const argv[]) {
    thread_logname = sdsempty();
    //set_loglevel(5);
    int rc = utest_main(argc, argv);
    FREE_SDS(thread_logname);
    return rc;
}
