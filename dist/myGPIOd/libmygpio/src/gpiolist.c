/*
 SPDX-License-Identifier: GPL-3.0-or-later
 libmygpio (c) 2020-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"

#include "libmygpio/include/libmygpio/libmygpio_gpiolist.h"
#include "libmygpio/include/libmygpio/libmygpio_parser.h"
#include "libmygpio/src/gpio_struct.h"
#include "libmygpio/src/pair.h"
#include "libmygpio/src/protocol.h"
#include "mygpio-common/util.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Send the gpiolist command and receives the status
 * @param connection connection struct
 * @return true on success, else false
 */
bool mygpio_gpiolist(struct t_mygpio_connection *connection) {
    return libmygpio_send_line(connection, "gpiolist") &&
        libmygpio_recv_response_status(connection);
}

/**
 * Receives the response for the gpiolist command
 * @param connection connection struct
 * @return gpio config struct or NULL on error or list end
 */
struct t_mygpio_gpio *mygpio_recv_gpio_list(struct t_mygpio_connection *connection) {
    unsigned gpio_nr;
    enum mygpio_gpio_direction direction;
    enum mygpio_gpio_value value;

    struct t_mygpio_pair *pair;
    if ((pair = mygpio_recv_pair_name(connection, "gpio")) == NULL) {
        return NULL;
    }
    if (mygpio_parse_uint(pair->value, &gpio_nr, NULL, 0, GPIOS_MAX) == false) {
        mygpio_free_pair(pair);
        return NULL;
    }
    mygpio_free_pair(pair);

    if ((pair = mygpio_recv_pair_name(connection, "direction")) == NULL) {
        return NULL;
    }
    if ((direction = mygpio_gpio_parse_direction(pair->value)) == MYGPIO_GPIO_DIRECTION_UNKNOWN) {
        mygpio_free_pair(pair);
        return NULL;
    }
    mygpio_free_pair(pair);

    if ((pair = mygpio_recv_pair_name(connection, "value")) == NULL) {
        return NULL;
    }
    if ((value = mygpio_gpio_parse_value(pair->value)) == MYGPIO_GPIO_VALUE_UNKNOWN) {
        mygpio_free_pair(pair);
        return NULL;
    }
    mygpio_free_pair(pair);

    struct t_mygpio_gpio *gpio = mygpio_gpio_new(MYGPIO_GPIO_DIRECTION_UNKNOWN);
    gpio->gpio = gpio_nr;
    gpio->direction = direction;
    gpio->value = value;
    return gpio;
}
