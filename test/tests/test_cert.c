/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"

#include "../../dist/sds/sds.h"
#include "../../dist/utest/utest.h"
#include "../../src/lib/cert.h"

#include <sys/stat.h>
#include <unistd.h>

UTEST(cert, test_certificates_check) {
    mkdir("/tmp/ssl", 0770);
    sds workdir = sdsnew("/tmp");
    sds ssl_san = sdsempty();
    certificates_check(workdir, ssl_san);
    sdsfree(workdir);
    sdsfree(ssl_san);
    bool rc = unlink("/tmp/ssl/ca.key");
    ASSERT_EQ(0, rc);
    rc = unlink("/tmp/ssl/ca.pem");
    ASSERT_EQ(0, rc);
    rc = unlink("/tmp/ssl/server.key");
    ASSERT_EQ(0, rc);
    rc = unlink("/tmp/ssl/server.pem");
    ASSERT_EQ(0, rc);
    rmdir("/tmp/ssl");
}
