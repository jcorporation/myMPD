/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_VALIDATE_H
#define MYMPD_VALIDATE_H

#include "../../dist/src/sds/sds.h"

#include <stdbool.h>

typedef bool (*validate_callback) (sds);

bool validate_string(const char *data);
bool validate_string_not_empty(const char *data);
bool validate_string_not_dir(const char *data);
bool validate_songuri(const char *data);
bool validate_uri(const char *data);
bool is_streamuri(const char *uri);

bool validate_json(sds data);
bool validate_json_array(sds data);

bool vcb_isalnum(sds data);
bool vcb_isprint(sds data);
bool vcb_ishexcolor(sds data);
bool vcb_isname(sds data);
bool vcb_istext(sds data);
bool vcb_isfilename(sds data);
bool vcb_isfilepath(sds data);

#endif
