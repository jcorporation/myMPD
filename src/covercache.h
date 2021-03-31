/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __COVERCACHE_H__
#define __COVERCACHE_H__
bool write_covercache_file(t_config *config, const char *uri, const char *mime_type, sds binary);
int clear_covercache(t_config *config, int keepdays);
#endif
