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

bool is_streamuri(const char *uri);
bool is_virtual_cuedir(sds music_directory, sds filename);
const char *get_extension_from_filename(const char *filename);
void basename_uri(sds s);
void strip_file_extension(sds s);
sds replace_file_extension(sds s, const char *ext);
void strip_slash(sds s);
void sanitize_filename(sds s);

const char *getenv_check(const char *env_var, size_t max_len);
void my_msleep(long msec);

sds get_mympd_host(sds mpd_host, sds http_host);

//measure time
#define MEASURE_INIT struct timespec tic, toc;
#define MEASURE_START clock_gettime(CLOCK_MONOTONIC, &tic);
#define MEASURE_END clock_gettime(CLOCK_MONOTONIC, &toc);
#define MEASURE_PRINT(X) MYMPD_LOG_DEBUG("Execution time for %s: %lld ms", X, ((long long)(toc.tv_sec) * 1000 + toc.tv_nsec / 1000000) - ((long long)(tic.tv_sec) * 1000 + tic.tv_nsec / 1000000));

#endif
