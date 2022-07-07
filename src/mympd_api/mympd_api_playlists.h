/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_PLAYLISTS_H
#define MYMPD_API_PLAYLISTS_H

#include "../lib/mympd_state.h"
#include "../mpd_client/mpd_client_playlists.h"

sds mympd_api_playlist_list(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id,
                             const long offset, const long limit, sds searchstr, enum playlist_types type);
sds mympd_api_playlist_content_list(struct t_mympd_state *mympd_state, sds buffer, sds method,
                                 long request_id, sds plist, const long offset,
                                 const long limit, sds searchstr,
                                 const struct t_tags *tagcols);
sds mympd_api_playlist_delete(struct t_mympd_state *mympd_state, sds buffer, sds method,
                               long request_id, const char *playlist, bool smartpls_only);
sds mympd_api_playlist_rename(struct t_mympd_state *mympd_state, sds buffer, sds method,
                               long request_id, const char *old_playlist, const char *new_playlist);
sds mympd_api_smartpls_get(struct t_config *config, sds buffer, sds method, long request_id,
                            const char *playlist);
sds mympd_api_playlist_delete_all(struct t_mympd_state *mympd_state, sds buffer, sds method,
                                  long request_id, const char *type);
void mympd_api_smartpls_update(const char *playlist);
void mympd_api_smartpls_update_all(void);
bool mympd_api_smartpls_default(struct t_config *config);
#endif
