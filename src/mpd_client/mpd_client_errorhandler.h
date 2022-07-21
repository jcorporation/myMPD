/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_MPD_CLIENT_ERRORHANDLER_H
#define MYMPD_MPD_CLIENT_ERRORHANDLER_H

#include "../dist/sds/sds.h"
#include "../lib/api.h"
#include "../lib/mympd_state.h"

bool mympd_check_error_and_recover(struct t_mpd_state *mpd_state);
bool mympd_check_rc_error_and_recover(struct t_mpd_state *mpd_state, bool rc, const char *command);

bool mympd_check_error_and_recover_respond(struct t_mpd_state *mpd_state, sds *buffer,
        enum mympd_cmd_ids cmd_id, long request_id);
bool mympd_check_rc_error_and_recover_respond(struct t_mpd_state *mpd_state, sds *buffer,
        enum mympd_cmd_ids cmd_id, long request_id, bool rc, const char *command);

bool mympd_check_error_and_recover_notify(struct t_mpd_state *mpd_state, sds *buffer);
bool mympd_check_rc_error_and_recover_notify(struct t_mpd_state *mpd_state, sds *buffer, bool rc,
        const char *command);

sds mympd_respond_with_error_or_ok(struct t_mpd_state *mpd_state, sds buffer, enum mympd_cmd_ids cmd_id,
        long request_id, bool rc, const char *command, bool *result);
sds mympd_respond_with_command_error(sds buffer, enum mympd_cmd_ids cmd_id, long request_id, const char *command);

#endif
