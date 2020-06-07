/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MPD_CLIENT_QUEUE_H__
#define __MPD_CLIENT_QUEUE_H__
sds mpd_client_get_queue_state(t_mpd_client_state *mpd_client_state, sds buffer);
sds mpd_client_put_queue_state(struct mpd_status *status, sds buffer);
sds mpd_client_put_queue(t_mpd_client_state *mpd_client_state, sds buffer, sds method, int request_id,
                         const unsigned int offset, const t_tags *tagcols);
sds mpd_client_crop_queue(t_mpd_client_state *mpd_client_state, sds buffer, sds method, int request_id, bool or_clear);
sds mpd_client_search_queue(t_mpd_client_state *mpd_client_state, sds buffer, sds method, int request_id,
                            const char *mpdtagtype, const unsigned int offset, 
                            const char *searchstr, const t_tags *tagcols);
bool mpd_client_queue_replace_with_song(t_mpd_client_state *mpd_client_state, const char *uri);
bool mpd_client_queue_replace_with_playlist(t_mpd_client_state *mpd_client_state, const char *plist);
#endif
