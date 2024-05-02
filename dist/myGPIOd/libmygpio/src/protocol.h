/*
 SPDX-License-Identifier: GPL-3.0-or-later
 libmygpio (c) 2020-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef LIBMYGPIO_SRC_PROTOCOL_H
#define LIBMYGPIO_SRC_PROTOCOL_H

#include "libmygpio/src/connection.h"

#include <stdbool.h>

bool libmygpio_send_line(struct t_mygpio_connection *connection, const char *fmt, ...);
bool libmygpio_recv_version(struct t_mygpio_connection *connection);
bool libmygpio_recv_response_status(struct t_mygpio_connection *connection);

#endif
