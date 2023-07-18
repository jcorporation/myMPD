/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_TRIGGER_H
#define MYMPD_API_TRIGGER_H

#include "dist/sds/sds.h"
#include "src/lib/list.h"

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

struct t_trigger_data {
    sds script;
    struct t_list arguments;
};

bool mympd_api_trigger_save(struct t_list *trigger_list, sds name, int trigger_id, int event, sds partition,
        struct t_trigger_data *trigger_data, sds *error);
sds mympd_api_trigger_list(struct t_list *trigger_list, sds buffer, long request_id, const char *partition);
sds mympd_api_trigger_get(struct t_list *trigger_list, sds buffer, long request_id, long id);
bool mympd_api_trigger_file_read(struct t_list *trigger_list, sds workdir);
bool mympd_api_trigger_file_save(struct t_list *trigger_list, sds workdir);
void mympd_api_trigger_list_clear(struct t_list *trigger_list);
void mympd_api_trigger_execute(struct t_list *trigger_list, enum trigger_events event, const char *partition);
void mympd_api_trigger_execute_feedback(struct t_list *trigger_list, sds uri, int vote, const char *partition);
bool mympd_api_trigger_delete(struct t_list *trigger_list, long idx, sds *error);
const char *mympd_api_event_name(long event);
sds mympd_api_trigger_print_event_list(sds buffer);
struct t_trigger_data *trigger_data_new(void);
void mympd_api_trigger_data_free(struct t_trigger_data *trigger_data);

#endif
