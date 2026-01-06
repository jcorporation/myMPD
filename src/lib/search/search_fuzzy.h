/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Fuzzy search implementation
 */

#ifndef MYMPD_LIB_SEARCH_FUZZY_H
#define MYMPD_LIB_SEARCH_FUZZY_H

#include <stdbool.h>

bool mympd_search_fuzzy_match(const char *haystack, const char *needle);

#endif
