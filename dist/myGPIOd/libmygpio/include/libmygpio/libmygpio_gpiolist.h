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

#ifndef LIBMYGPIO_GPIOLIST_H
#define LIBMYGPIO_GPIOLIST_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct t_mygpio_connection;
struct t_mygpio_gpio;

/**
 * @defgroup libmygpio_gpiolist GPIO list
 *
 * @brief This module provides functions for the gpiolist protocol command.
 *
 * @{
 */

/**
 * Lists the modes and values of all configured GPIOs.
 * Retrieve the list elements with mygpio_recv_gpio_list and end the response with mygpio_response_end.
 * @param connection Pointer to the connection struct returned by mygpio_connection_new.
 * @return bool true on success, else false.
 */
bool mygpio_gpiolist(struct t_mygpio_connection *connection);

/**
 * Receives a list element of mygpio_gpiolist.
 * Use the mygpio_gpio_get_gpio_* functions to access the values.
 * The caller must free it with mygpio_free_gpio.
 * @param connection Pointer to the connection struct returned by mygpio_connection_new.
 * @return Allocated struct t_mygpio_gpio or NULL on list end or error.
 */
struct t_mygpio_gpio *mygpio_recv_gpio_list(struct t_mygpio_connection *connection);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
