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

#ifndef LIBMYGPIO_GPIO_H
#define LIBMYGPIO_GPIO_H

#include "libmygpio_gpio_struct.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct t_mygpio_connection;

/**
 * @defgroup libmygpio_gpio_functions GPIO functions
 *
 * @brief This module provides functions to set and get values of a GPIO.
 *
 * @{
 */

/**
 * Returns the current value of a configured input or output GPIO.
 * @param connection Pointer to the connection struct returned by mygpio_connection_new.
 * @param gpio GPIO number
 * @return Value of the GPIO or MYGPIO_GPIO_VALUE_UNKNOWN on error.
 */
enum mygpio_gpio_value mygpio_gpioget(struct t_mygpio_connection *connection, unsigned gpio);

/**
 * Sets the value of a configured output GPIO.
 * @param connection Pointer to the connection struct returned by mygpio_connection_new.
 * @param gpio GPIO number
 * @param value Value to set: MYGPIO_GPIO_VALUE_INACTIVE or MYGPIO_GPIO_VALUE_ACTIVE
 * @return true on success, else false.
 */
bool mygpio_gpioset(struct t_mygpio_connection *connection, unsigned gpio, enum mygpio_gpio_value value);

/**
 * Toggles the value of a configured output GPIO.
 * @param connection Pointer to the connection struct returned by mygpio_connection_new.
 * @param gpio GPIO number
 * @return true on success, else false.
 */
bool mygpio_gpiotoggle(struct t_mygpio_connection *connection, unsigned gpio);

/**
 * Toggles the value of a configured output GPIO at given timeout and interval.
 * @param connection Pointer to the connection struct returned by mygpio_connection_new.
 * @param gpio GPIO number
 * @param timeout_ms timeout in milliseconds
 * @param interval_ms interval in milliseconds, set it 0 to blink only once.
 * @return true on success, else false.
 */
bool mygpio_gpioblink(struct t_mygpio_connection *connection, unsigned gpio, int timeout_ms, int interval_ms);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
