/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/scripts/scripts.h"

#include "src/lib/list.h"
#include "src/lib/mem.h"
#include "src/lib/sds_extras.h"

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
 * Frees the t_script_thread_arg struct
 * @param script_thread_arg pointer to the struct to free
 */
void free_t_script_thread_arg(struct t_script_thread_arg *script_thread_arg) {
    FREE_SDS(script_thread_arg->script_name);
    FREE_SDS(script_thread_arg->script_fullpath);
    FREE_SDS(script_thread_arg->script_content);
    FREE_SDS(script_thread_arg->partition);
    list_free(script_thread_arg->arguments);
    FREE_PTR(script_thread_arg);
}
