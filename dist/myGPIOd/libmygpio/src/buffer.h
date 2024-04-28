/*
 SPDX-License-Identifier: GPL-3.0-or-later
 libmygpio (c) 2020-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef LIBMYGPIO_SRC_BUFFER_H
#define LIBMYGPIO_SRC_BUFFER_H

#include <stddef.h>

#define BUFFER_SIZE_MAX 1025

/**
 * Struct for input and output buffers
 */
struct t_buf {
    char buffer[BUFFER_SIZE_MAX]; //!< the buffer
    size_t len;                   //!< current size
};

void libmygpio_buf_init(struct t_buf *buf);

#endif
