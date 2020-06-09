/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MPD_CLIENT_PLAYLISTS_H__
#define __MPD_CLIENT_PLAYLISTS_H__
sds mpd_client_put_playlists(t_config *config, t_mpd_client_state *mpd_client_state, sds buffer, sds method, int request_id,
                             const unsigned int offset, const char *filter, bool paginated);
sds mpd_client_put_playlist_list(t_config *config, t_mpd_client_state *mpd_client_state, sds buffer, sds method, int request_id,
                                 const char *uri, const unsigned int offset, const char *filter, const t_tags *tagcols);
sds mpd_client_playlist_delete(t_config *config, t_mpd_client_state *mpd_client_state, sds buffer, sds method, int request_id,
                               const char *playlist);
sds mpd_client_playlist_rename(t_config *config, t_mpd_client_state *mpd_client_state, sds buffer, sds method, int request_id,
                                const char *old_playlist, const char *new_playlist);
sds mpd_client_smartpls_put(t_config *config, sds buffer, sds method, int request_id,
                            const char *playlist);
sds mpd_client_playlist_delete_all(t_config *config, t_mpd_client_state *mpd_client_state, sds buffer, sds method, int request_id,
                                   const char *type);
void mpd_client_smartpls_update(const char *playlist);
void mpd_client_smartpls_update_all(void);
#endif
