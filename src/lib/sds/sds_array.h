/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Array functions for sds strings
 */

#ifndef MYMPD_SDS_ARRAY_H
#define MYMPD_SDS_ARRAY_H

#include "dist/sds/sds.h"

#include <stdbool.h>

struct t_sds_array {
    unsigned length;
    unsigned capacity;
    sds *items;
};

enum {
    SDS_ARRAY_START_CAPACITY = 16
};

struct t_sds_array *sds_array_new(void);
void sds_array_init(struct t_sds_array *array);
void sds_array_clear(struct t_sds_array *array);
void sds_array_free(struct t_sds_array *array);
bool sds_array_push(struct t_sds_array *array, sds s);
bool sds_array_shuffle(struct t_sds_array *array);

#endif
