/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/passwd.h"

#include "src/lib/log.h"
#include "src/lib/mem.h"

#include <unistd.h>

/**
 * Gets the passwd entry for the supplied user.
 * @param pwd struct passwd to save the result
 * @param username the username
 * @return pointer to passwd struct
 */
struct passwd *get_passwd_entry(struct passwd *pwd, const char *username) {
    long initlen = sysconf(_SC_GETPW_R_SIZE_MAX);
    size_t len = initlen == -1
        ? 1024
        : (size_t) initlen;
    struct passwd *pwd_ptr;
    char *buffer = malloc_assert(len);
    int rc;
    if ((rc = getpwnam_r(username, pwd, buffer, len, &pwd_ptr)) != 0) {
        MYMPD_LOG_ERROR(NULL, "User \"%s\" does not exist (%d)", username, rc);
        FREE_PTR(buffer);
        return NULL;
    }
    FREE_PTR(buffer);
    return pwd_ptr;
}
