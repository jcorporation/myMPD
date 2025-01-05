/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "src/lib/passwd.h"

#include <pwd.h>

UTEST(passwd, test_get_passwd_entry) {
    struct passwd pwd;
    struct passwd *pwd_ptr = get_passwd_entry(&pwd, "root");
    ASSERT_TRUE(pwd_ptr != NULL);
}
