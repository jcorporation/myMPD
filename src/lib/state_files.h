/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_STATE_FILES_H
#define MYMPD_STATE_FILES_H

#include "../../dist/sds/sds.h"
#include "validate.h"

#include <stdbool.h>

bool check_partition_state_dir(sds workdir, sds partition);
sds state_file_rw_string_sds(sds workdir, const char *dir, const char *name, sds def_value, validate_callback vcb, bool warn);
sds state_file_rw_string(sds workdir, const char *dir, const char *name, const char *def_value, validate_callback vcb, bool warn);
bool state_file_rw_bool(sds workdir, const char *dir, const char *name, bool def_value, bool warn);
int state_file_rw_int(sds workdir, const char *dir, const char *name, int def_value, int min, int max, bool warn);
unsigned state_file_rw_uint(sds workdir, const char *dir, const char *name, unsigned def_value, unsigned min, unsigned max, bool warn);
long state_file_rw_long(sds workdir, const char *dir, const char *name, long def_value, long min, long max, bool warn);
bool state_file_write(sds workdir, const char *subdir, const char *name, const char *value);
sds camel_to_snake(sds text);
#endif
