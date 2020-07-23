/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MPD_CLIENT_TRIGGER_H
#define MPD_CLIENT_TRIGGER_H
sds trigger_list(t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id);
bool triggerfile_read(t_config *config, t_mpd_client_state *mpd_client_state);
bool triggerfile_save(t_config *config, t_mpd_client_state *mpd_client_state);
void free_trigerlist_arguments(t_mpd_client_state *mpd_client_state);
void trigger_execute(t_mpd_client_state *mpd_client_state, enum trigger_events event);
bool delete_trigger(t_mpd_client_state *mpd_client_state, unsigned idx);
#endif
