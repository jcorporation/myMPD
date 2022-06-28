/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_UTILITY_H
#define MYMPD_UTILITY_H

#include <stdbool.h>
#include <time.h>

#include "../../dist/sds/sds.h"

enum testdir_status {
    DIR_EXISTS = 0,
    DIR_CREATED = 1,
    DIR_CREATE_FAILED = 2,
    DIR_NOT_EXISTS = 3
};

const char *getenv_check(const char *env_var, size_t max_len);
void ws_notify(sds message);
bool is_virtual_cuedir(sds music_directory, sds filename);
int testdir(const char *name, const char *dirname, bool create);
void my_msleep(long msec);
bool is_streamuri(const char *uri);
bool write_data_to_file(sds filepath, const char *data, size_t data_len);
sds *split_coverimage_names(sds coverimage_name, sds *coverimage_names, int *count);
const char *get_extension_from_filename(const char *filename);

//measure time
#define MEASURE_INIT struct timespec tic, toc;
#define MEASURE_START clock_gettime(CLOCK_MONOTONIC, &tic);
#define MEASURE_END clock_gettime(CLOCK_MONOTONIC, &toc);
#define MEASURE_PRINT(X) MYMPD_LOG_DEBUG("Execution time for %s: %lld ms", X, ((long long)(toc.tv_sec) * 1000 + toc.tv_nsec / 1000000) - ((long long)(tic.tv_sec) * 1000 + tic.tv_nsec / 1000000));

#endif
