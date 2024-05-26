/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_SCRIPTS_EVENTS_H
#define MYMPD_SCRIPTS_EVENTS_H

/**
 * Script start events
 */
enum script_start_events {
    SCRIPT_START_UNKNOWN = -1,
    SCRIPT_START_TIMER,
    SCRIPT_START_TRIGGER,
    SCRIPT_START_USER,
    SCRIPT_START_HTTP,
    SCRIPT_START_EXTERN
};

const char *script_start_event_name(enum script_start_events start_event);
enum script_start_events script_start_event_parse(const char *str);

#endif
