/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_UTILITY_H
#define MYMPD_UTILITY_H

#include <stdbool.h>

#include "../../dist/src/sds/sds.h"

int testdir(const char *name, const char *dirname, bool create);
void streamuri_to_filename(sds s);
bool strtobool(const char *value);
sds get_extension_from_filename(const char *filename);
int strip_extension(char *s);
void strip_slash(sds s);
void my_usleep(time_t usec);
unsigned long substractUnsigned(unsigned long num1, unsigned long num2);
char *basename_uri(char *uri);
int unsigned_to_int(unsigned x);

//measure time
#define MEASURE_START clock_t measure_start = clock();
#define MEASURE_END clock_t measure_end = clock();
#define MEASURE_PRINT(X) MYMPD_LOG_DEBUG("Execution time for %s: %lf", X, ((double) (measure_end - measure_start)) / CLOCKS_PER_SEC);

#endif
