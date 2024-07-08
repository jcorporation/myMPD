/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Utility functions
 */

#ifndef MYMPD_LIB_UTILITY_H
#define MYMPD_LIB_UTILITY_H

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
void sanitize_filename2(sds filename);

void my_msleep(int msec);

sds resolv_mympd_uri(sds uri, sds mpd_host, struct t_config *config);
bool get_ipv6_support(void);

/**
 * Initialize time measurement
 */
#define MEASURE_INIT struct timespec tic;\
    struct timespec toc;

/**
 * Start measurement
 */
#define MEASURE_START clock_gettime(CLOCK_MONOTONIC, &tic);

/**
 * Stop measurement
 */
#define MEASURE_END clock_gettime(CLOCK_MONOTONIC, &toc);

/**
 * Print measurement result
 */
#define MEASURE_PRINT(PART, X) MYMPD_LOG_DEBUG(PART, "Execution time for %s: %" PRId64 " ms", X, \
    (int64_t)((toc.tv_sec * 1000 + toc.tv_nsec / 1000000) - (tic.tv_sec * 1000 + tic.tv_nsec / 1000000)));

#endif
