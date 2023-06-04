/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "src/lib/random.h"

UTEST(random, test_random_small) {
    //initialize random number generator
    printf("Random number: ");
    for (int i = 0; i < 100; i++) {
        long r = randrange(0, 100);
        printf("%ld ", r);
        ASSERT_GE(r, 0);
        ASSERT_LE(r, 100);
    }
    printf("\n");
}

UTEST(random, test_random_large) {
    //initialize random number generator
    printf("Random number: ");
    for (int i = 0; i < 100; i++) {
        long r = randrange(1000, 100000);
        printf("%ld ", r);
        ASSERT_GE(r, 1000);
        ASSERT_LE(r, 100000);
    }
    printf("\n");
}
