/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __VALIDATE_H__
#define __VALIDATE_H__

#include <stdbool.h>

bool validate_string(const char *data);
bool validate_string_not_empty(const char *data);
bool validate_string_not_dir(const char *data);
bool validate_songuri(const char *data);
bool validate_uri(const char *data);
bool is_streamuri(const char *uri);

#endif
