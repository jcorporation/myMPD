// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

/**
 * @mainpage
 *
 * This is a client library for the [Music Player
 * Daemon](https://www.musicpd.org/), written in C.
 *
 * You can choose one of several APIs, depending on your requirements:
 *
 * - struct mpd_async: a very low-level asynchronous API which knows
 *   the protocol syntax, but no specific commands
 *
 * - struct mpd_connection: a basic synchronous API which knows all
 *   MPD commands and parses all responses
 *
 * \author Max Kellermann (max.kellermann@gmail.com)
 */

#ifndef MPD_CLIENT_H
#define MPD_CLIENT_H

// IWYU pragma: begin_exports

#include "audio_format.h"
#include "albumart.h"
#include "binary.h"
#include "capabilities.h"
#include "connection.h"
#include "database.h"
#include "directory.h"
#include "entity.h"
#include "fingerprint.h"
#include "idle.h"
#include "list.h"
#include "message.h"
#include "mixer.h"
#include "mount.h"
#include "neighbor.h"
#include "output.h"
#include "pair.h"
#include "partition.h"
#include "password.h"
#include "player.h"
#include "playlist.h"
#include "queue.h"
#include "readpicture.h"
#include "recv.h"
#include "replay_gain.h"
#include "response.h"
#include "search.h"
#include "send.h"
#include "settings.h"
#include "song.h"
#include "stats.h"
#include "status.h"
#include "sticker.h"

/* this is a generated header and may be installed in a different
   filesystem tree, therefore we can't use just "version.h" */
#include <mpd/version.h>

// IWYU pragma: end_exports

#endif
