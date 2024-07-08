/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD trigger API
 */

#ifndef MYMPD_API_TRIGGER_H
#define MYMPD_API_TRIGGER_H

#include "dist/sds/sds.h"
#include "src/lib/list.h"
#include "src/lib/sticker.h"

/**
 * myMPD trigger events. The list is composed of MPD idle events and
 * myMPD specific events.
 */
enum trigger_events {
    TRIGGER_MYMPD_SCROBBLE = -1,       //!< myMPD scrobble event (same event is used for last played sticker / list)
    TRIGGER_MYMPD_START = -2,          //!< myMPD was started (before mpd connection)
    TRIGGER_MYMPD_STOP = -3,           //!< myMPD stops
    TRIGGER_MYMPD_CONNECTED = -4,      //!< myMPD connected to mpd event
    TRIGGER_MYMPD_DISCONNECTED = -5,   //!< myMPD disconnect from mpd event
    TRIGGER_MYMPD_FEEDBACK = -6,       //!< myMPD feedback event (love/hate)
    TRIGGER_MYMPD_SKIPPED = -7,        //!< myMPD song skipped (same event is used for skipped sticker)
    TRIGGER_MYMPD_LYRICS = -8,         //!< myMPD lyrics
    TRIGGER_MYMPD_ALBUMART = -9,       //!< myMPD albumart
    TRIGGER_MYMPD_TAGART = -10,        //!< myMPD tagart
    TRIGGER_MYMPD_JUKEBOX = -11,       //!< myMPD jukebox
    TRIGGER_MPD_DATABASE = 0x1,        //!< mpd database has changed
    TRIGGER_MPD_STORED_PLAYLIST = 0x2, //!< mpd playlist idle event
    TRIGGER_MPD_QUEUE = 0x4,           //!< mpd queue idle event
    TRIGGER_MPD_PLAYER = 0x8,          //!< mpd player idle event
    TRIGGER_MPD_MIXER = 0x10,          //!< mpd mixer idle event (volume)
    TRIGGER_MPD_OUTPUT = 0x20,         //!< mpd output idle event
    TRIGGER_MPD_OPTIONS = 0x40,        //!< mpd options idle event
    TRIGGER_MPD_UPDATE = 0x80,         //!< mpd database idle event (started or finished)
    TRIGGER_MPD_STICKER = 0x100,       //!< mpd sticker idle event
    TRIGGER_MPD_SUBSCRIPTION = 0x200,  //!< mpd subscription idle event
    TRIGGER_MPD_MESSAGE = 0x400,       //!< mpd message idle event
    TRIGGER_MPD_PARTITION = 0x800,     //!< mpd partition idle event
    TRIGGER_MPD_NEIGHBOR = 0x1000,     //!< mpd neighbor idle event
    TRIGGER_MPD_MOUNT = 0x2000         //!< mpd mount idle event
};

/**
 * Holds the scripts and its arguments for a trigger
 */
struct t_trigger_data {
    sds script;               //!< script to execute
    struct t_list arguments;  //!< arguments for the script to execute
};

bool mympd_api_trigger_save(struct t_list *trigger_list, sds name, int trigger_id, int event, sds partition,
        struct t_trigger_data *trigger_data, sds *error);
sds mympd_api_trigger_list(struct t_list *trigger_list, sds buffer, unsigned request_id, const char *partition);
sds mympd_api_trigger_get(struct t_list *trigger_list, sds buffer, unsigned request_id, unsigned trigger_id);
bool mympd_api_trigger_file_read(struct t_list *trigger_list, sds workdir);
bool mympd_api_trigger_file_save(struct t_list *trigger_list, sds workdir);
void mympd_api_trigger_list_clear(struct t_list *trigger_list);
int mympd_api_trigger_execute(struct t_list *trigger_list, enum trigger_events event,
        const char *partition, struct t_list *arguments);
int mympd_api_trigger_execute_http(struct t_list *trigger_list, enum trigger_events event,
        const char *partition, unsigned long conn_id, unsigned request_id,
        struct t_list *arguments);
int mympd_api_trigger_execute_feedback(struct t_list *trigger_list, sds uri,
        enum feedback_type type, int value, const char *partition);
bool mympd_api_trigger_delete(struct t_list *trigger_list, unsigned idx, sds *error);
const char *mympd_api_event_name(int event);
sds mympd_api_trigger_print_event_list(sds buffer);
struct t_trigger_data *trigger_data_new(void);
void mympd_api_trigger_data_free(struct t_trigger_data *trigger_data);

#endif
