/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_LIB_SCRIPTS_H
#define MYMPD_LIB_SCRIPTS_H

#include "dist/sds/sds.h"

#include <stdbool.h>

enum script_start_events {
    SCRIPT_START_UNKNOWN = -1,
    SCRIPT_START_TIMER,
    SCRIPT_START_TRIGGER,
    SCRIPT_START_USER,
    SCRIPT_START_HTTP,
    SCRIPT_START_EXTERN
};

/**
 * Struct for passing values to the script execute function
 */
struct t_script_thread_arg {
    sds lualibs;                           //!< comma separated string of lua libs to load
    bool localscript;                      //!< true = read script from filesystem, false = use script_content
    sds script_fullpath;                   //!< full uri of the script
    sds script_name;                       //!< name of the script
    sds script_content;                    //!< script content if localscript = false
    sds partition;                         //!< execute the script in this partition
    struct t_list *arguments;              //!< argumentlist
    enum script_start_events start_event;  //!< script start event
    unsigned long conn_id;                 //!< mongoose connection id
    unsigned request_id;                   //!< jsonrpc request id
};

const char *script_start_event_name(enum script_start_events start_event);
enum script_start_events script_start_event_parse(const char *str);
void free_t_script_thread_arg(struct t_script_thread_arg *script_thread_arg);

#endif
