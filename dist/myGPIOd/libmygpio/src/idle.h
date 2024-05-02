/*
 SPDX-License-Identifier: GPL-3.0-or-later
 libmygpio (c) 2020-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef LIBMYGPIO_SRC_IDLE_H
#define LIBMYGPIO_SRC_IDLE_H

#include "libmygpio/include/libmygpio/libmygpio_idle.h"

/**
 * Struct holding the event information received by mygpio_recv_idle_event.
 */
struct t_mygpio_idle_event {
    unsigned gpio;            //!< GPIO number
    enum mygpio_event event;  //!< the event
    uint64_t timestamp_ms;    //!< timestamp in milliseconds
};

#endif
