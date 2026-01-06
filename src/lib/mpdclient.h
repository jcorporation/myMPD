/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Custom libmpdclient include
 */

#ifndef MYMPD_MPDCLIENT_H
#define MYMPD_MPDCLIENT_H

#ifdef MYMPD_EMBEDDED_LIBMPDCLIENT
    //copied from /dist/libmpdclient/include/mpd/client.h
    #include "dist/libmpdclient/include/mpd/audio_format.h"
    #include "dist/libmpdclient/include/mpd/albumart.h"
    #include "dist/libmpdclient/include/mpd/binary.h"
    #include "dist/libmpdclient/include/mpd/capabilities.h"
    #include "dist/libmpdclient/include/mpd/connection.h"
    #include "dist/libmpdclient/include/mpd/database.h"
    #include "dist/libmpdclient/include/mpd/directory.h"
    #include "dist/libmpdclient/include/mpd/entity.h"
    #include "dist/libmpdclient/include/mpd/feature.h"
    #include "dist/libmpdclient/include/mpd/fingerprint.h"
    #include "dist/libmpdclient/include/mpd/idle.h"
    #include "dist/libmpdclient/include/mpd/list.h"
    #include "dist/libmpdclient/include/mpd/message.h"
    #include "dist/libmpdclient/include/mpd/mixer.h"
    #include "dist/libmpdclient/include/mpd/mount.h"
    #include "dist/libmpdclient/include/mpd/neighbor.h"
    #include "dist/libmpdclient/include/mpd/output.h"
    #include "dist/libmpdclient/include/mpd/pair.h"
    #include "dist/libmpdclient/include/mpd/partition.h"
    #include "dist/libmpdclient/include/mpd/password.h"
    #include "dist/libmpdclient/include/mpd/player.h"
    #include "dist/libmpdclient/include/mpd/playlist.h"
    #include "dist/libmpdclient/include/mpd/queue.h"
    #include "dist/libmpdclient/include/mpd/readpicture.h"
    #include "dist/libmpdclient/include/mpd/recv.h"
    #include "dist/libmpdclient/include/mpd/replay_gain.h"
    #include "dist/libmpdclient/include/mpd/response.h"
    #include "dist/libmpdclient/include/mpd/search.h"
    #include "dist/libmpdclient/include/mpd/send.h"
    #include "dist/libmpdclient/include/mpd/settings.h"
    #include "dist/libmpdclient/include/mpd/song.h"
    #include "dist/libmpdclient/include/mpd/stats.h"
    #include "dist/libmpdclient/include/mpd/status.h"
    #include "dist/libmpdclient/include/mpd/sticker.h"
    #include "dist/libmpdclient/include/mpd/stringnormalization.h"
    // Use version include from dist
    #include "dist/libmpdclient/include/mpd/version.h"
#else
    // Use shared libmpdclient
    #include <mpd/client.h>
#endif

#endif
