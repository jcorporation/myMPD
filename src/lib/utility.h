/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_UTILITY_H
#define MYMPD_UTILITY_H

#include "dist/sds/sds.h"
#include "src/lib/config_def.h"

#include <stdbool.h>
#include <time.h>


bool is_streamuri(const char *uri);
bool is_virtual_cuedir(sds music_directory, sds filename);
const char *get_extension_from_filename(const char *filename);
void basename_uri(sds uri);
void strip_file_extension(sds filename);
sds replace_file_extension(sds filename, const char *ext);
void strip_slash(sds dirname);
void sanitize_filename(sds filename);

void my_msleep(long msec);

sds resolv_mympd_uri(sds uri, sds mpd_host, struct t_config *config);

//measure time
#define MEASURE_INIT struct timespec tic;\
    struct timespec toc;
#define MEASURE_START clock_gettime(CLOCK_MONOTONIC, &tic);
#define MEASURE_END clock_gettime(CLOCK_MONOTONIC, &toc);
#define MEASURE_PRINT(X) MYMPD_LOG_DEBUG("Execution time for %s: %lld ms", X, ((long long)(toc.tv_sec) * 1000 + toc.tv_nsec / 1000000) - ((long long)(tic.tv_sec) * 1000 + tic.tv_nsec / 1000000));

#endif
