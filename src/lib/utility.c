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
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

const char *getenv_check(const char *env_var, size_t max_len) {
    const char *env_value = getenv(env_var); /* Flawfinder: ignore */
    if (env_value == NULL) {
        MYMPD_LOG_DEBUG("Environment variable \"%s\" not set", env_var);
        return NULL;
    }
    if (env_value[0] == '\0') {
        MYMPD_LOG_DEBUG("Environment variable \"%s\" is empty", env_var);
        return NULL;
    }
    if (strlen(env_value) > max_len) {
        MYMPD_LOG_WARN("Environment variable \"%s\" is too long", env_var);
        return NULL;
    }
    MYMPD_LOG_INFO("Got environment variable \"%s\" with value \"%s\"", env_var, env_value);
    return env_value;
}

sds *split_coverimage_names(sds coverimage_name, int *count) {
    *count = 0;
    sds *coverimage_names = sdssplitlen(coverimage_name, (ssize_t)sdslen(coverimage_name), ",", 1, count);
    for (int j = 0; j < *count; j++) {
        sdstrim(coverimage_names[j], " ");
    }
    return coverimage_names;
}

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
        if (mkdir(dirname, 0770) != 0) {
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

void my_msleep(long msec) {
    struct timespec ts = {
        .tv_sec = (time_t)(msec / 1000),
        .tv_nsec = (msec % 1000L) * 1000000L
    };
    nanosleep(&ts, NULL);
}

//mpd uses the cue filename as path
//we simply check if the filename is a file or not
bool is_virtual_cuedir(sds music_directory, sds filename) {
    sds full_path = sdscatfmt(sdsempty(), "%S/%S", music_directory, filename);
    bool is_cue_file = false;
    struct stat stat_buf;
    if (stat(full_path, &stat_buf) == 0) {
        if (S_ISREG(stat_buf.st_mode)) {
            MYMPD_LOG_DEBUG("Path \"%s\" is a virtual cuesheet directory", filename);
            is_cue_file = true;
        }
    }
    else {
        MYMPD_LOG_ERROR("Error accessing \"%s\"", full_path);
    }
    FREE_SDS(full_path);
    return is_cue_file;
}

bool is_streamuri(const char *uri) {
    if (uri != NULL &&
        strstr(uri, "://") != NULL)
    {
        return true;
    }
    return false;
}

//opens tmp file for write
FILE *open_tmp_file(sds filepath) {
    errno = 0;
    int fd = mkstemp(filepath);
    if (fd < 0) {
        MYMPD_LOG_ERROR("Can not open file \"%s\" for write", filepath);
        MYMPD_LOG_ERRNO(errno);
        return NULL;
    }
    errno = 0;
    FILE *fp = fdopen(fd, "w");
    if (fp == NULL) {
        MYMPD_LOG_ERROR("Can not open file \"%s\" for write", filepath);
        MYMPD_LOG_ERRNO(errno);
    }
    return fp;
}

//closes the tmp file and moves it to its destination name
bool rename_tmp_file(FILE *fp, sds tmp_file, sds filepath, bool write_rc) {
    if (fclose(fp) != 0 ||
        write_rc == false)
    {
        MYMPD_LOG_ERROR("Error writing data to file \"%s\"", tmp_file);
        errno = 0;
        if (unlink(tmp_file) != 0) {
            MYMPD_LOG_ERROR("Error removing file \"%s\"", tmp_file);
            MYMPD_LOG_ERRNO(errno);
        }
        FREE_SDS(tmp_file);
        return false;
    }
    errno = 0;
    if (rename(tmp_file, filepath) == -1) {
        MYMPD_LOG_ERROR("Rename file from \"%s\" to \"%s\" failed", tmp_file, filepath);
        MYMPD_LOG_ERRNO(errno);
        errno = 0;
        if (unlink(tmp_file) != 0) {
            MYMPD_LOG_ERROR("Error removing file \"%s\"", tmp_file);
            MYMPD_LOG_ERRNO(errno);
        }
        return false;
    }
    return true;
}

bool write_data_to_file(sds filepath, const char *data, size_t data_len) {
    sds tmp_file = sdscatfmt(sdsempty(), "%S.XXXXXX", filepath);
    FILE *fp = open_tmp_file(tmp_file);
    if (fp == NULL) {
        FREE_SDS(tmp_file);
        return false;
    }
    size_t written = fwrite(data, 1, data_len, fp);
    bool write_rc = written == data_len ? true : false;
    bool rc = rename_tmp_file(fp, tmp_file, filepath, write_rc);
    FREE_SDS(tmp_file);
    return rc;
}

const char *get_extension_from_filename(const char *filename) {
    if (filename == NULL) {
        return NULL;
    }
    char *ext = strrchr(filename, '.');
    if (ext != NULL) {
        //skip dot
        ext++;
        if (ext[0] == '\0') {
            return NULL;
        }
    }
    return ext;
}
