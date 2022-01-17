/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_MPD_SHARED_H
#define MYMPD_MPD_SHARED_H

#include "../dist/sds/sds.h"

#include "lib/mympd_state.h"

void mpd_shared_free_mpd_state(struct t_mpd_state *mpd_state);
void mpd_shared_default_mpd_state(struct t_mpd_state *mpd_state);
void mpd_shared_mpd_disconnect(struct t_mpd_state *mpd_state);
bool check_rc_error_and_recover(struct t_mpd_state *mpd_state, sds *buffer,
                                sds method, long request_id, bool notify, bool rc, const char *command);
bool check_error_and_recover2(struct t_mpd_state *mpd_state, sds *buffer, sds method, long request_id,
                              bool notify);
sds check_error_and_recover(struct t_mpd_state *mpd_state, sds buffer, sds method, long request_id);
sds check_error_and_recover_notify(struct t_mpd_state *mpd_state, sds buffer);
sds respond_with_command_error(sds buffer, sds method, long request_id, const char *command);
sds respond_with_mpd_error_or_ok(struct t_mpd_state *mpd_state, sds buffer, sds method,
                                 long request_id, bool rc, const char *command, bool *result);
bool mpd_shared_set_keepalive(struct t_mpd_state *mpd_state);
#endif
