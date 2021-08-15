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
        return 0;
    }

    if (create == true) {
        errno = 0;
        if (mkdir(dirname, 0700) != 0) {
            MYMPD_LOG_ERROR("%s: creating \"%s\" failed", name, dirname);
            MYMPD_LOG_ERRNO(errno);
            //directory does not exist and creating it failed
            return 2;
        }
        MYMPD_LOG_NOTICE("%s: \"%s\" created", name, dirname);
        //directory successfully created
        return 1;
    }

    MYMPD_LOG_ERROR("%s: \"%s\" does not exist", name, dirname);
    //directory does not exist
    return 3;
}

void strip_slash(sds s) {
    ssize_t len = (ssize_t)sdslen(s);
    if (len > 1 && s[len - 1] == '/') {
        sdsrange(s, 0, len - 2);
    }
}

int strip_extension(char *s) {
    for (size_t i = strlen(s) - 1 ; i > 0; i--) {
        if (s[i] == '.') {
            s[i] = '\0';
            return (int)i;
        }
        if (s[i] == '/') {
            return -1;
        }
    }
    return -1;
}

int replacechar(char *str, const char orig, const char rep) {
    char *ix = str;
    int n = 0;
    while ((ix = strchr(ix, orig)) != NULL) {
        *ix++ = rep;
        n++;
    }
    return n;
}

bool strtobool(const char *value) {
    return strncmp(value, "true", 4) == 0 ? true : false;
}

char *strtolower(char *s) {
    for (char *p = s; *p; p++) {
        *p = tolower((unsigned char) *p);
    }
    return s;
}

int uri_to_filename(char *str) {
    int n = replacechar(str, '/', '_');
    n+= replacechar(str, '.', '_');
    n+= replacechar(str, ':', '_');
    return n;
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

char *basename_uri(char *uri) {
    //filename
    if (strstr(uri, "://") == NULL) {
        char *b = basename(uri);
        return b;
    }
    //uri, remove query and hash
    char *b = uri;
    for (size_t i = 0;  i < strlen(b); i++) {
        if (b[i] == '#' || b[i] == '?') {
            b[i] = '\0';
            return b;
        }
    }
    return b;
}

//converts unsigned to int and prevents wrap arround
int unsigned_to_int(unsigned x) {
    return x < INT_MAX ? (int) x : INT_MAX;
}
