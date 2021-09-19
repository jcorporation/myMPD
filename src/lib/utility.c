/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "utility.h"

#include "log.h"
#include "sds_extras.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

int testdir(const char *name, const char *dirname, bool create) {
    DIR* dir = opendir(dirname);
    if (dir != NULL) {
        closedir(dir);
        MYMPD_LOG_NOTICE("%s: \"%s\"", name, dirname);
        //directory exists
        return DIR_EXISTS;
    }

    if (create == true) {
        errno = 0;
        if (mkdir(dirname, 0700) != 0) {
            MYMPD_LOG_ERROR("%s: creating \"%s\" failed", name, dirname);
            MYMPD_LOG_ERRNO(errno);
            //directory does not exist and creating it failed
            return DIR_CREATE_FAILED;
        }
        MYMPD_LOG_NOTICE("%s: \"%s\" created", name, dirname);
        //directory successfully created
        return DIR_CREATED;
    }

    MYMPD_LOG_ERROR("%s: \"%s\" does not exist", name, dirname);
    //directory does not exist
    return DIR_NOT_EXISTS;
}

void my_usleep(time_t usec) {
    struct timespec ts = {
        .tv_sec = (usec / 1000) / 1000,
        .tv_nsec = (usec % 1000000000L) * 1000
    };
    nanosleep(&ts, NULL);
}

unsigned long substractUnsigned(unsigned long num1, unsigned long num2) {
    if (num1 > num2) {
        return num1 - num2;
    }
    return 0;
}
