/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"

#include "dist/utest/utest.h"
#include "dist/tinymt/tinymt32.h"
#include "src/lib/random.h"

UTEST(random, test_random) {
    //initialize random number generator
    tinymt32_init(&tinymt, (unsigned)time(NULL));
    printf("Random number: ");
    for (int i = 0; i < 100; i++) {
        long r = randrange(0, 100);
        printf("%ld ", r);
        ASSERT_GE(r, 0);
        ASSERT_LE(r, 100);
    }
    printf("\n");
}
