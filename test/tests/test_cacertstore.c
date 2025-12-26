/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "src/lib/config/cacertstore.h"

UTEST(cacertstore, test_find_ca_cert_store) {
    const char *cabundle = find_ca_cert_store(false);
    ASSERT_TRUE(cabundle != NULL);
}
