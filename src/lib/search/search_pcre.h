/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief PCRE search implementation
 */

#ifndef MYMPD_LIB_SEARCH_PCRE_H
#define MYMPD_LIB_SEARCH_PCRE_H

#include "dist/sds/sds.h"

/**
 * PCRE for UTF-8
 */
#define PCRE2_CODE_UNIT_WIDTH 8

#include <pcre2.h>
#include <stdbool.h>

pcre2_code *mympd_search_pcre_compile(sds regex_str);
bool mympd_search_pcre_match(pcre2_code *re_compiled, const char *value);

#endif
