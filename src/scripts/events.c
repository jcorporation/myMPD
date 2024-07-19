/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Events helper functions
 */

#include "compile_time.h"
#include "src/lib/mem.h"
#include "src/lib/sds_extras.h"
#include "src/scripts/events.h"

#include <string.h>

/**
 * Returns the name for the script start event
 * @param start_event start event enum
 * @return start event name or empty if unknown
 */
const char *script_start_event_name(enum script_start_events start_event) {
    switch (start_event) {
        case SCRIPT_START_EXTERN:  return "extern";
        case SCRIPT_START_HTTP:    return "http";
        case SCRIPT_START_TIMER:   return "timer";
        case SCRIPT_START_TRIGGER: return "trigger";
        case SCRIPT_START_USER:    return "user";
        case SCRIPT_START_UNKNOWN: return "";
    }
    return "";
}

/**
 * Parses the name for the script start event
 * @param str string to parse
 * @return script_start_event enum
 */
enum script_start_events script_start_event_parse(const char *str) {
    if (strcmp(str, "http") == 0) { return SCRIPT_START_HTTP; }
    if (strcmp(str, "timer") == 0) { return SCRIPT_START_TIMER; }
    if (strcmp(str, "trigger") == 0) { return SCRIPT_START_TRIGGER; }
    if (strcmp(str, "user") == 0) { return SCRIPT_START_USER; }
    if (strcmp(str, "extern") == 0) { return SCRIPT_START_EXTERN; }
    return SCRIPT_START_UNKNOWN;
}

/**
 * Creates the script_execute_data struct
 * @param scriptname script name
 * @param script_event script start event
 * @return newly allocated struct
 */
struct t_script_execute_data *script_execute_data_new(const char *scriptname, enum script_start_events script_event) {
    struct t_script_execute_data *data = malloc_assert(sizeof(struct t_script_execute_data));
    data->scriptname = sdsnew(scriptname);
    data->script_event = script_event;
    data->arguments = NULL;
    return data;
}

/**
 * Frees the script_execute_data struct
 * @param data script_execute_data struct
 */
void script_execute_data_free(struct t_script_execute_data *data) {
    list_free(data->arguments);
    FREE_SDS(data->scriptname);
    FREE_PTR(data);
}
