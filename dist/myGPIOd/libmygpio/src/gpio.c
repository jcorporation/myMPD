/*
 SPDX-License-Identifier: GPL-3.0-or-later
 libmygpio (c) 2020-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "libmygpio/include/libmygpio/libmygpio_gpio.h"
#include "libmygpio/include/libmygpio/libmygpio_parser.h"
#include "libmygpio/include/libmygpio/libmygpio_protocol.h"
#include "libmygpio/src/pair.h"
#include "libmygpio/src/protocol.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Receives the value of an input or output gpio
 * @param connection connection struct
 * @param gpio gpio number (0-64)
 * @return value of the gpio or MYGPIO_GPIO_VALUE_UNKNOWN on error
 */
enum mygpio_gpio_value mygpio_gpioget(struct t_mygpio_connection *connection, unsigned gpio) {
    enum mygpio_gpio_value value;
    struct t_mygpio_pair *pair;
    if (gpio > GPIOS_MAX) {
        return MYGPIO_GPIO_VALUE_UNKNOWN;
    }
    if (libmygpio_send_line(connection, "gpioget %u", gpio) == false ||
        libmygpio_recv_response_status(connection) == false ||
        (pair = mygpio_recv_pair(connection)) == NULL ||
        strcmp(pair->name, "value") != 0 ||
        (value = mygpio_gpio_parse_value(pair->value)) == MYGPIO_GPIO_VALUE_UNKNOWN)
    {
        return MYGPIO_GPIO_VALUE_UNKNOWN;
    }
    return value;
}

/**
 * Sets the value of a configured output gpio
 * @param connection connection struct
 * @param gpio gpio number (0-64)
 * @param value gpio value
 * @return true on success, else false
 */
bool mygpio_gpioset(struct t_mygpio_connection *connection, unsigned gpio, enum mygpio_gpio_value value) {
    return libmygpio_send_line(connection, "gpioset %u %s", gpio, mygpio_gpio_lookup_value(value)) &&
        libmygpio_recv_response_status(connection) &&
        mygpio_response_end(connection);
}

/**
 * Toggles the value of a configured output GPIO.
 * @param connection Pointer to the connection struct returned by mygpio_connection_new.
 * @param gpio GPIO number
 * @return true on success, else false.
 */
bool mygpio_gpiotoggle(struct t_mygpio_connection *connection, unsigned gpio) {
    return libmygpio_send_line(connection, "gpiotoggle %u", gpio) &&
        libmygpio_recv_response_status(connection) &&
        mygpio_response_end(connection);
}

/**
 * Toggles the value of a configured output GPIO at given timeout and interval.
 * @param connection Pointer to the connection struct returned by mygpio_connection_new.
 * @param gpio GPIO number
 * @param timeout_ms timeout in milliseconds
 * @param interval_ms interval in milliseconds, set it 0 to blink only once.
 * @return true on success, else false.
 */
bool mygpio_gpioblink(struct t_mygpio_connection *connection, unsigned gpio, int timeout_ms, int interval_ms) {
    return libmygpio_send_line(connection, "gpioblink %u %d %d", gpio, timeout_ms, interval_ms) &&
        libmygpio_recv_response_status(connection) &&
        mygpio_response_end(connection);
}
