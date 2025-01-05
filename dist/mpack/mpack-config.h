/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#define MPACK_READER  0
#define MPACK_EXPECT  0
#define MPACK_DOUBLE  0
#define MPACK_FLOAT   0

#include "src/lib/mem.h"

#define MPACK_MALLOC malloc_assert
#define MPACK_FREE free

#ifdef MYMPD_DEBUG
    #define MPACK_DEBUG 1
#else
    #define MPACK_DEBUG 0
#endif
