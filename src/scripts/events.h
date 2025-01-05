/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Events helper functions
 */

#ifndef MYMPD_SCRIPTS_EVENTS_H
#define MYMPD_SCRIPTS_EVENTS_H

#include "src/lib/list.h"

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

/**
 * Data for INTERNAL_API_SCRIPT_EXECUTE
 */
struct t_script_execute_data {
    sds scriptname;                        //!< Script name
    enum script_start_events script_event; //!< Script start event
    struct t_list *arguments;              //!< List of script arguments
};

const char *script_start_event_name(enum script_start_events start_event);
enum script_start_events script_start_event_parse(const char *str);

struct t_script_execute_data *script_execute_data_new(const char *scriptname, enum script_start_events script_event);
void script_execute_data_free(struct t_script_execute_data *data);

#endif
