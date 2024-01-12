/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "src/lib/random.h"

UTEST(random, test_random_char) {
    char buffer[25];
    randstring(buffer, 25);
    ASSERT_STRNE(buffer, "");
    printf("Random chars: %s", buffer);
}

UTEST(random, test_random_xsmall) {
    printf("Random number: ");
    for (int i = 0; i < 100; i++) {
        unsigned r = randrange(0, 10);
        printf("%u ", r);
        ASSERT_LT(r, 10U);
    }
    printf("\n");
}

UTEST(random, test_random_small) {
    printf("Random number: ");
    for (int i = 0; i < 100; i++) {
        unsigned r = randrange(0, 100);
        printf("%u ", r);
        ASSERT_LT(r, 100U);
    }
    printf("\n");
}

UTEST(random, test_random_small2) {
    int s = 0;
    int m = 0;
    int l = 0;
    int x = 0;
    for (int i = 0; i < 1000; i++) {
        unsigned r = randrange(20, 40);
        ASSERT_GE(r, 20U);
        ASSERT_LT(r, 40U);
        if (r < 25) { s++; }
        else if (r < 30) { m++; }
        else if (r < 35) { l++; }
        else { x++; }
    }
    printf("Random number distribution: %d %d %d %d\n", s, m, l, x);
    ASSERT_GE(s, 200);
    ASSERT_GE(m, 200);
    ASSERT_GE(l, 200);
    ASSERT_GE(x, 200);
    ASSERT_LE(s, 300);
    ASSERT_LE(m, 300);
    ASSERT_LE(l, 300);
    ASSERT_LE(x, 300);
}

UTEST(random, test_random_medium) {
    printf("Random number: ");
    for (int i = 0; i < 100; i++) {
        unsigned r = randrange(1000, 9000);
        printf("%u ", r);
        ASSERT_GE(r, 1000U);
        ASSERT_LT(r, 9000U);
    }
    printf("\n");
}

UTEST(random, test_random_larg) {
    int s = 0;
    int m = 0;
    int l = 0;
    int x = 0;
    for (int i = 0; i < 100000; i++) {
        unsigned r = randrange(0, 9999999);
        ASSERT_LE(r, 9999999U);
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

UTEST(random, test_random_max) {
    int s = 0;
    int m = 0;
    int l = 0;
    int x = 0;
    unsigned max = 0;
    unsigned upper = UINT_MAX;
    for (int i = 0; i < 100000; i++) {
        unsigned r = randrange(0, upper);
        if (r > max) {
            max = r;
        }
        ASSERT_LT(r, upper);
        if (r < upper / 4) { s++; }
        else if (r < upper / 2) { m++; }
        else if (r < (upper / 4) * 3) { l++; }
        else { x++; }
    }
    printf("Random number distribution: %d %d %d %d\n", s, m, l, x);
    printf("Max value: %u / %u\n", max, upper);
    ASSERT_LE(s, 30000);
    ASSERT_LE(m, 30000);
    ASSERT_LE(l, 30000);
    ASSERT_LE(x, 30000);
    ASSERT_GT(s, 20000);
    ASSERT_GT(m, 20000);
    ASSERT_GT(l, 20000);
    ASSERT_GT(x, 20000);
}
