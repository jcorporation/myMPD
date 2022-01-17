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

sds state_file_rw_string_sds(const char *workdir, const char *dir, const char *name, sds old_value, validate_callback vcb, bool warn);
sds state_file_rw_string(const char *workdir, const char *dir, const char *name, const char *def_value, validate_callback vcb, bool warn);
bool state_file_rw_bool(const char *workdir, const char *dir, const char *name, const bool def_value, bool warn);
int state_file_rw_int(const char *workdir, const char *dir, const char *name, const int def_value, const int min, const int max, bool warn);
unsigned state_file_rw_uint(const char *workdir, const char *dir, const char *name, const unsigned def_value, const unsigned min, const unsigned max, bool warn);
long state_file_rw_long(const char *workdir, const char *dir, const char *name, const long def_value, const long min, const long max, bool warn);
bool state_file_write(const char *workdir, const char *dir, const char *name, const char *value);
sds camel_to_snake(sds text);
#endif
