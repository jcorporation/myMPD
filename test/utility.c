/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "utility.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

void init_testenv(void) {
    mkdir("/tmp/mympd-test", 0770);
    mkdir("/tmp/mympd-test/state", 0770);
    mkdir("/tmp/mympd-test/state/default", 0770);
    mkdir("/tmp/mympd-test/webradios", 0770);
    unsetenv("TESTVAR");
}

void clean_testenv(void) {
    if (system("rm -rf /tmp/mympd-test") != 0) {
        printf("Failure cleaning /tmp/mympd-test\n");
    }
    unsetenv("TESTVAR");
}
