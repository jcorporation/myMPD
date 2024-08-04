/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief String validation functions
 */

#ifndef MYMPD_VALIDATE_H
#define MYMPD_VALIDATE_H

#include "dist/sds/sds.h"

#include <stdbool.h>

/**
 * Definition for the validation callback used by the jsonrpc functions
 */
typedef bool (*validate_callback) (sds);

bool validate_json_object(sds data);
bool validate_json_array(sds data);

bool vcb_isalnum(sds data);
bool vcb_isdigit(sds data);
bool vcb_isprint(sds data);
bool vcb_ishexcolor(sds data);
bool vcb_isname(sds data);
bool vcb_istext(sds data);
bool vcb_isfilename_silent(sds data);
bool vcb_isfilename(sds data);
bool check_dir_traversal(const char *str);
bool vcb_isfilepath(sds data);
bool vcb_ispathfilename(sds data);
bool vcb_isuri(sds data);
bool vcb_isstreamuri(sds data);
bool vcb_isfield(sds data);
bool vcb_istaglist(sds data);
bool vcb_ismpdtag(sds data);
bool vcb_ismpdtag_or_any(sds data);
bool vcb_ismpdstickertype(sds data);
bool vcb_ismpdsort(sds data);
bool vcb_iswebradiosort(sds data);
bool vcb_issearchexpression(sds data);

bool vcb_isstickersort(sds data);
bool vcb_isstickerop(sds data);

bool vcb_ismpd_sticker_sort(sds data);

#endif
