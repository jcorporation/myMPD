/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __BROWSE_H__
#define __BROWSE_H__
sds mpd_client_put_fingerprint(t_mpd_state *mpd_state, sds buffer, sds method, int request_id,
                               const char *uri);
sds mpd_client_put_songdetails(t_mpd_state *mpd_state, sds buffer, sds method, int request_id, 
                               const char *uri);
sds mpd_client_put_filesystem(t_config *config, t_mpd_state *mpd_state, sds buffer, sds method, int request_id, 
                              const char *path, const unsigned int offset, const char *filter, const t_tags *tagcols);
sds mpd_client_put_db_tag(t_mpd_state *mpd_state, sds buffer, sds method, int request_id,
                          const unsigned int offset, const char *mpdtagtype, const char *mpdsearchtagtype, const char *searchstr, const char *filter);
sds mpd_client_put_songs_in_album(t_mpd_state *mpd_state, sds buffer, sds method, int request_id,
                                  const char *album, const char *search, const char *tag, const t_tags *tagcols);
sds mpd_client_put_firstsong_in_albums(t_config *config, t_mpd_state *mpd_state, sds buffer, sds method, int request_id,
                                       const char *searchstr, const char *tag, const char *sort, bool sortdesc, const unsigned int offset);
#endif
