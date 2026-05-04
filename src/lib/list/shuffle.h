/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Linked list merge shuffle implementation
 */

#ifndef MYMPD_LIST_SHUFFLE_H
#define MYMPD_LIST_SHUFFLE_H

#include <stdbool.h>

struct t_list;

bool list_shuffle(struct t_list *l);

#endif
