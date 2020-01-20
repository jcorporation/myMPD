/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MYMPD_API_BOOKMARKS_H
#define __MYMPD_API_BOOKMARKS_H
bool mympd_api_bookmark_update(t_config *config, const int id, const char *name,
                               const char *uri, const char *type);
sds mympd_api_bookmark_list(t_config *config, sds buffer, sds method, int request_id, 
                            unsigned int offset);
bool mympd_api_bookmark_clear(t_config *config);
#endif
