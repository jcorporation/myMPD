/*
 SPDX-License-Identifier: GPL-3.0-or-later
 libmygpio (c) 2020-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myGPIOd client library
 *
 * Do not include this header directly. Use libmygpio/libmygpio.h instead.
 */

#ifndef LIBMYGPIO_PROTOCOL_H
#define LIBMYGPIO_PROTOCOL_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct t_mygpio_connection;

/**
 * @defgroup libmygpio_protocol Protocol
 *
 * @brief This module provides generic myGPIOd protocol functions.
 *
 * @{
 */

/**
 * Finishes reading the response from myGPIOd and empties the input buffer.
 * @param connection Pointer to the connection struct returned by mygpio_connection_new.
 * @return true on success, else false
 */
bool mygpio_response_end(struct t_mygpio_connection *connection);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
