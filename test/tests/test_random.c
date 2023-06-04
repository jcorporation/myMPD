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
    printf("Random number: ");
    for (int i = 0; i < 100; i++) {
        long r = randrange(0, 100);
        printf("%ld ", r);
        ASSERT_GE(r, 0);
        ASSERT_LE(r, 100);
    }
    printf("\n");
}

UTEST(random, test_random_medium) {
    printf("Random number: ");
    for (int i = 0; i < 100; i++) {
        long r = randrange(1000, 9000);
        printf("%ld ", r);
        ASSERT_GE(r, 1000);
        ASSERT_LE(r, 9000);
    }
    printf("\n");
}

UTEST(random, test_random_larg) {
    int s = 0;
    int m = 0;
    int l = 0;
    int x = 0;
    for (int i = 0; i < 100000; i++) {
        long r = randrange(0, 9999999);
        ASSERT_GE(r, 0);
        ASSERT_LE(r, 9999999);
        if (r < 2500000) { s++; }
        else if (r < 5000000) { m++; }
        else if (r < 7500000) { l++; }
        else { x++; }
    }
    printf("Random number distribution: %d %d %d %d\n", s, m, l, x);
    ASSERT_LE(s, 30000);
    ASSERT_LE(m, 30000);
    ASSERT_LE(l, 30000);
    ASSERT_LE(x, 30000);
}
