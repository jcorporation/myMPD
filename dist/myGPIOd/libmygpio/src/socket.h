/*
 SPDX-License-Identifier: GPL-3.0-or-later
 libmygpio (c) 2020-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef LIBMYGPIO_SRC_SOCKET_H
#define LIBMYGPIO_SRC_SOCKET_H

#include "libmygpio/src/buffer.h"

#include <stdbool.h>

int libmygpio_socket_connect(const char *socket_path);
void libmygpio_socket_close(int fd);
bool libmygpio_socket_recv_line(int fd, struct t_buf *buf, int timeout_ms);
bool libmygpio_socket_send_line(int fd, struct t_buf *buf);

#endif
