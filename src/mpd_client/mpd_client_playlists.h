/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MPD_CLIENT_PLAYLISTS_H__
#define __MPD_CLIENT_PLAYLISTS_H__
bool smartpls_default(struct t_config *config);
sds mpd_client_put_playlists(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id,
                             const unsigned int offset, const unsigned int limit, const char *searchstr);
sds mpd_client_put_playlist_list(struct t_mympd_state *mympd_state, sds buffer, sds method, 
                                 long request_id, const char *uri, const unsigned int offset, 
                                 const unsigned int limit, const char *searchstr,
                                 const struct t_tags *tagcols);
sds mpd_client_playlist_delete(struct t_mympd_state *mympd_state, sds buffer, sds method,
                               long request_id, const char *playlist);
sds mpd_client_playlist_rename(struct t_mympd_state *mympd_state, sds buffer, sds method,
                               long request_id, const char *old_playlist, const char *new_playlist);
sds mpd_client_smartpls_put(struct t_config *config, sds buffer, sds method, long request_id,
                            const char *playlist);
sds mpd_client_playlist_delete_all(struct t_mympd_state *mympd_state, sds buffer, sds method, 
                                   long request_id, const char *type);
void mpd_client_smartpls_update(const char *playlist);
void mpd_client_smartpls_update_all(void);
#endif
