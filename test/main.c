/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"

#include "../src/lib/sds_extras.h"

#include "tests/test_list.h"
#include "tests/test_queue.h"
#include "tests/test_sds.h"

_Thread_local sds thread_logname;

int main(void) {
    thread_logname = sdsempty();

    test_queue();
    test_list();
    test_sds();
    
    sdsfree(thread_logname);
    return 0;
}
