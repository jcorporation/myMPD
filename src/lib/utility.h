/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_UTILITY_H
#define MYMPD_UTILITY_H

#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#include "../../dist/sds/sds.h"

enum testdir_status {
    DIR_EXISTS = 0,
    DIR_CREATED = 1,
    DIR_CREATE_FAILED = 2,
    DIR_NOT_EXISTS = 3
};

enum try_rm_file_status {
    RM_FILE_OK = 0,
    RM_FILE_ENOENT = 1,
    RM_FILE_ERROR = 2
};

bool is_streamuri(const char *uri);
bool is_virtual_cuedir(sds music_directory, sds filename);

const char *getenv_check(const char *env_var, size_t max_len);
void ws_notify(sds message);
int testdir(const char *name, const char *dirname, bool create);
void my_msleep(long msec);

sds *split_coverimage_names(sds coverimage_name, int *count);
const char *get_extension_from_filename(const char *filename);

FILE *open_tmp_file(sds filepath);
bool rename_tmp_file(FILE *fp, sds tmp_file, sds filepath, bool write_rc);
bool write_data_to_file(sds filepath, const char *data, size_t data_len);
bool rm_file(sds filepath);
int try_rm_file(sds filepath);

sds get_mympd_host(sds mpd_host, sds http_host);

//measure time
#define MEASURE_INIT struct timespec tic, toc;
#define MEASURE_START clock_gettime(CLOCK_MONOTONIC, &tic);
#define MEASURE_END clock_gettime(CLOCK_MONOTONIC, &toc);
#define MEASURE_PRINT(X) MYMPD_LOG_DEBUG("Execution time for %s: %lld ms", X, ((long long)(toc.tv_sec) * 1000 + toc.tv_nsec / 1000000) - ((long long)(tic.tv_sec) * 1000 + tic.tv_nsec / 1000000));

#endif
