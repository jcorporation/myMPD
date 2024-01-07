/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "dist/sds/sds.h"
#include "dist/utest/utest.h"
#include "src/lib/cert.h"

#include <sys/stat.h>
#include <unistd.h>

UTEST(cert, test_certificates_check) {
    init_testenv();

    sds ssl_san = sdsempty();
    certificates_check(workdir, ssl_san);
    sdsfree(ssl_san);
    bool rc = unlink("/tmp/mympd-test/ssl/ca.key");
    ASSERT_EQ(0, rc);
    rc = unlink("/tmp/mympd-test/ssl/ca.pem");
    ASSERT_EQ(0, rc);
    rc = unlink("/tmp/mympd-test/ssl/server.key");
    ASSERT_EQ(0, rc);
    rc = unlink("/tmp/mympd-test/ssl/server.pem");
    ASSERT_EQ(0, rc);

    clean_testenv();
}
