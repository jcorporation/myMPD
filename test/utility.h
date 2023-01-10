/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef TEST_UTILITY_H
#define TEST_UTILITY_H

#include "../src/lib/sds_extras.h"

extern sds workdir;

struct t_input_result {
    const char *input;
    const char *result;
};

#endif
