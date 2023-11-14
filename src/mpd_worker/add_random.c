/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mpd_worker/add_random.h"

#include "dist/sds/sds.h"

#include <stdbool.h>

/**
 * Adds randoms songs or albums to the queue
 * @param mpd_worker_state pointer to mpd_worker_state
 * @param add number of songs/albums to add
 * @param mode 1 = add songs, 2 = add albums
 * @param plist playlist to select songs from
 * @return true on success, else false
 */
bool mpd_worker_add_random_to_queue(struct t_mpd_worker_state *mpd_worker_state, long add, unsigned mode, sds plist) {
    //TODO: implement after album cache is thread save
    (void) mpd_worker_state;
    (void) add;
    (void) mode;
    (void) plist;
    return true;
}
