/*
 SPDX-License-Identifier: GPL-3.0-or-later
 libmygpio (c) 2020-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "libmygpio/src/buffer.h"

#include <assert.h>
#include <stdlib.h>

/**
 * Initializes the buffer struct
 * @param buf 
 */
void libmygpio_buf_init(struct t_buf *buf) {
    buf->buffer[0] = '\0';
    buf->len = 0;
}
