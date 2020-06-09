/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MPD_SHARED_H__
#define __MPD_SHARED_H__

void mpd_shared_free_mpd_state(t_mpd_state *mpd_state);
void mpd_shared_default_mpd_state(t_mpd_state *mpd_state);
void mpd_shared_mpd_disconnect(t_mpd_state *mpd_state);
bool check_rc_error_and_recover(t_mpd_state *mpd_state, sds *buffer,
                                sds method, int request_id, bool notify, bool rc, const char *command);
bool check_error_and_recover2(t_mpd_state *mpd_state, sds *buffer, sds method, int request_id, bool notify);
sds check_error_and_recover(t_mpd_state *mpd_state, sds buffer, sds method, int request_id);
sds check_error_and_recover_notify(t_mpd_state *mpd_state, sds buffer);
sds respond_with_command_error(sds buffer, sds method, int request_id, const char *command);
sds respond_with_mpd_error_or_ok(t_mpd_state *mpd_state, sds buffer, sds method, int request_id, bool rc, const char *command);
bool mpd_shared_feat_mpd_searchwindow(t_mpd_state *mpd_state);
bool mpd_shared_feat_tags(t_mpd_state *mpd_state);
bool mpd_shared_feat_advsearch(t_mpd_state *mpd_state);
#endif
