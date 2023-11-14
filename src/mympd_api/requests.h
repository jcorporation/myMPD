/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_REQUESTS_H
#define MYMPD_API_REQUESTS_H

#include "src/mympd_api/trigger.h"

#include <stdbool.h>

bool mympd_api_request_caches_create(void);
bool mympd_api_request_jukebox_restart(const char *partition);
bool mympd_api_request_trigger_event_emit(enum trigger_events event, const char *partition);
bool mympd_api_request_sticker_features(bool feat_sticker, bool feat_sticker_sort_window, bool sticker_int);

#endif
