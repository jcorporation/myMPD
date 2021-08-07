/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __STATE_FILES_H
#define __STATE_FILES_H

#include <stdbool.h>

#include "../dist/src/sds/sds.h"

sds state_file_rw_string_sds(const char *workdir, const char *dir, const char *name, sds old_value, bool warn);
sds state_file_rw_string(const char *workdir, const char *dir, const char *name, const char *def_value, bool warn);
bool state_file_rw_bool(const char *workdir, const char *dir, const char *name, const bool def_value, bool warn);
int state_file_rw_int(const char *workdir, const char *dir, const char *name, const int def_value, bool warn);
unsigned state_file_rw_uint(const char *workdir, const char *dir, const char *name, const unsigned def_value, bool warn);
bool state_file_write(const char *workdir, const char *dir, const char *name, const char *value);
sds camel_to_snake(const char *text, size_t len);
#endif
