/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "utility.h"

#include "api.h"
#include "log.h"
#include "sds_extras.h"

#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

void ws_notify(sds message) {
    MYMPD_LOG_DEBUG("Push websocket notify to queue: \"%s\"", message);
    struct t_work_result *response = create_result_new(0, 0, INTERNAL_API_WEBSERVER_NOTIFY);
    response->data = sds_replace(response->data, message);
    mympd_queue_push(web_server_queue, response, 0);
}

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

bool is_virtual_cuedir(sds music_directory, sds filename) {
    sds full_path = sdscatfmt(sdsempty(), "%s/%s", music_directory, filename);
    bool is_file = false;
    struct stat stat_buf;
    if (stat(full_path, &stat_buf) == 0) {
        if (S_ISREG(stat_buf.st_mode)) {
            MYMPD_LOG_DEBUG("Path \"%s\" is a virtual cuesheet directory", filename);
            is_file = true;
        }
    }
    sdsfree(full_path);
    return is_file;
}

bool is_streamuri(const char *uri) {
    if (uri != NULL && strstr(uri, "://") != NULL) {
        return true;
    }
    return false;
}
