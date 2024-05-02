/*
 SPDX-License-Identifier: GPL-3.0-or-later
 libmygpio (c) 2020-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef LIBMYGPIO_SRC_CONNECTION_H
#define LIBMYGPIO_SRC_CONNECTION_H

#include "libmygpio/include/libmygpio/libmygpio_connection.h"
#include "libmygpio/src/buffer.h"

struct t_mygpio_connection {
    int fd;                        //!< myGPIOd socket
    char *socket_path;             //!< path to myGPIOd socket
    struct t_buf buf_in;           //!< input buffer
    struct t_buf buf_out;          //!< output buffer
    unsigned version[3];           //!< myGPIOd version
    int timeout_ms;                //!< connection timeout in ms
    enum mygpio_conn_state state;  //!< connection state
    char *error;                   //!< error message
};

void libmygpio_connection_set_state(struct t_mygpio_connection *connection,
        enum mygpio_conn_state state, const char *message);
bool mygpio_connection_check(struct t_mygpio_connection *connection);

#endif
