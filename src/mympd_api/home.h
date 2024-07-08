/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD home icons API
 */

#ifndef MYMPD_API_HOME_H
#define MYMPD_API_HOME_H

#include "dist/sds/sds.h"
#include "src/lib/list.h"

#include <stdbool.h>

bool mympd_api_home_icon_move(struct t_list *home_list, unsigned from, unsigned to);
bool mympd_api_home_icon_delete(struct t_list *home_list, unsigned pos);
bool mympd_api_home_icon_save(struct t_list *home_list, bool replace, unsigned oldpos,
    sds name, sds ligature, sds bgcolor, sds color, sds image, sds cmd, struct t_list *option_list);
sds mympd_api_home_icon_list(struct t_list *home_list, sds buffer, unsigned request_id);
sds mympd_api_home_icon_get(struct t_list *home_list, sds buffer, unsigned request_id, unsigned pos);
bool mympd_api_home_file_read(struct t_list *home_list, sds workdir);
bool mympd_api_home_file_save(struct t_list *home_list, sds workdir);
#endif
