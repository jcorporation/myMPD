/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_UTILITY_H
#define MYMPD_UTILITY_H

#include <stdbool.h>

#include "../../dist/sds/sds.h"

enum testdir_status {
    DIR_EXISTS = 0,
    DIR_CREATED = 1,
    DIR_CREATE_FAILED = 2,
    DIR_NOT_EXISTS = 3
};

void ws_notify(sds message);
bool is_virtual_cuedir(sds music_directory, sds filename);
int testdir(const char *name, const char *dirname, bool create);
void my_usleep(time_t usec);

//measure time
#define MEASURE_START clock_t measure_start = clock();
#define MEASURE_END clock_t measure_end = clock();
#define MEASURE_PRINT(X) MYMPD_LOG_DEBUG("Execution time for %s: %lf", X, ((double) (measure_end - measure_start)) / CLOCKS_PER_SEC);

#endif
