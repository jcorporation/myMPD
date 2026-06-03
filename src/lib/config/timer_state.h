/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Central myMPD state for the mympd_api thread
 */

#ifndef MYMPD_TIMER_STATE_H
#define MYMPD_TIMER_STATE_H

#include "dist/sds/sds.h"
#include "src/lib/list/list.h"

/**
 * Optional timer definition from GUI
 */
struct t_timer_definition {
    sds name;                         //!< name of the timer
    sds partition;                    //!< mpd partition
    bool enabled;                     //!< enabled flag
    int start_hour;                   //!< start hour
    int start_minute;                 //!< start minute
    sds action;                       //!< timer action, e.g. script, play
    sds subaction;                    //!< timer subaction, e.g. script to execute
    unsigned volume;                  //!< volume to set
    sds playlist;                     //!< playlist to load for play timer
    sds preset;                       //!< preset to load for play timer
    bool weekdays[7];                 //!< array of weekdays for timer execution
    struct t_list arguments;          //!< argumentlist for script timers
};

/**
 * Struct for timers containing a t_list with t_timer_nodes
 */
struct t_timer_list {
    unsigned last_id;                   //!< highest timer id in the list
    int active;                         //!< number of enabled timers
    struct t_list list;                 //!< timer definition
    bool *repopulate_pfds;              //!< Pointer to repopulate state in mympd_state struct
};

#endif
