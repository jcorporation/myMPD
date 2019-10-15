/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __PLUGINS_H__
#define __PLUGINS_H__
bool init_plugins(struct t_config *config);
void close_plugins(struct t_config *config);
void *handle_plugins_coverextract;
bool (*plugin_coverextract)(const char *, const char *, char *, const int, char *, const int, const bool);
#endif
