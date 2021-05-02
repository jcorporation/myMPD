/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __BROWSE_H__
#define __BROWSE_H__
sds mpd_client_put_fingerprint(struct t_mympd_state *mympd_state, sds buffer, sds method,
                               long request_id, const char *uri);
sds mpd_client_put_songdetails(struct t_mympd_state *mympd_state, sds buffer, sds method,
                               long request_id, const char *uri);
sds mpd_client_put_filesystem(struct t_mympd_state *mympd_state, sds buffer, 
                              sds method, long request_id, const char *path, const unsigned int offset, 
                              const unsigned int limit, const char *searchstr,
                              const struct t_tags *tagcols);
sds mpd_client_put_songs_in_album(struct t_mympd_state *mympd_state, sds buffer, sds method,
                                  long request_id, const char *album, const char *search, 
                                  const char *tag, const struct t_tags *tagcols);
sds mpd_client_put_firstsong_in_albums(struct t_mympd_state *mympd_state, sds buffer, sds method,
                                       long request_id, const char *expression, const char *filter,
                                       const char *sort, bool sortdesc, const unsigned int offset,
                                       unsigned int limit);
sds mpd_client_put_db_tag2(struct t_mympd_state *mympd_state, sds buffer, 
                           sds method, long request_id, const char *searchstr, const char *filter,
                           const char *sort, bool sortdesc, const unsigned int offset, 
                           const unsigned int limit, const char *tag);
#endif
