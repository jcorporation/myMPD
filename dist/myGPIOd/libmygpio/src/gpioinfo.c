/*
 SPDX-License-Identifier: GPL-3.0-or-later
 libmygpio (c) 2020-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"

#include "libmygpio/include/libmygpio/libmygpio_gpioinfo.h"
#include "libmygpio/include/libmygpio/libmygpio_parser.h"
#include "libmygpio/src/gpio_struct.h"
#include "libmygpio/src/pair.h"
#include "libmygpio/src/protocol.h"
#include "mygpio-common/util.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Send the gpioinfo command and receives the status
 * @param connection connection struct
 * @return true on success, else false
 */
bool mygpio_gpioinfo(struct t_mygpio_connection *connection, unsigned gpio) {
    return libmygpio_send_line(connection, "gpioinfo %u", gpio) &&
        libmygpio_recv_response_status(connection);
}

/**
 * Receives a list element of mygpio_gpioinfo.
 * Free it with mygpio_free_gpio.
 * @param connection Pointer to the connection struct returned by mygpio_connection_new.
 * @return Allocated struct t_mygpio_gpio or NULL on list end or error.
 */
struct t_mygpio_gpio *mygpio_recv_gpio_info(struct t_mygpio_connection *connection) {
    unsigned gpio_nr;
    enum mygpio_gpio_direction direction;
    enum mygpio_gpio_value value;
    bool active_low = false;
    enum mygpio_gpio_bias bias = MYGPIO_BIAS_UNKNOWN;
    enum mygpio_event_request event_request = MYGPIO_EVENT_REQUEST_UNKNOWN;
    bool is_debounced = false;
    int debounce_period_us = 0;
    enum mygpio_event_clock event_clock = MYGPIO_EVENT_CLOCK_UNKNOWN;
    enum mygpio_drive drive = MYGPIO_DRIVE_UNKNOWN;

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

    if (direction == MYGPIO_GPIO_DIRECTION_IN) {
        if ((pair = mygpio_recv_pair_name(connection, "active_low")) == NULL) {
            return NULL;
        }
        active_low = mygpio_parse_bool(pair->value);
        mygpio_free_pair(pair);

        if ((pair = mygpio_recv_pair_name(connection, "bias")) == NULL) {
            return NULL;
        }
        if ((bias = mygpio_gpio_parse_bias(pair->value)) == MYGPIO_BIAS_UNKNOWN) {
            mygpio_free_pair(pair);
            return NULL;
        }
        mygpio_free_pair(pair);

        if ((pair = mygpio_recv_pair_name(connection, "event_request")) == NULL) {
            return NULL;
        }
        if ((event_request = mygpio_gpio_parse_event_request(pair->value)) == MYGPIO_EVENT_REQUEST_UNKNOWN) {
            mygpio_free_pair(pair);
            return NULL;
        }
        mygpio_free_pair(pair);

        if ((pair = mygpio_recv_pair_name(connection, "is_debounced")) == NULL) {
            return NULL;
        }
        is_debounced = mygpio_parse_bool(pair->value);
        mygpio_free_pair(pair);

        if ((pair = mygpio_recv_pair_name(connection, "debounce_period_us")) == NULL) {
            return NULL;
        }
        if (mygpio_parse_int(pair->value, &debounce_period_us, NULL, 0, INT_MAX) == false) {
            mygpio_free_pair(pair);
            return NULL;
        }
        mygpio_free_pair(pair);

        if ((pair = mygpio_recv_pair_name(connection, "event_clock")) == NULL) {
            return NULL;
        }
        if ((event_clock = mygpio_gpio_parse_event_clock(pair->value)) == MYGPIO_EVENT_CLOCK_UNKNOWN) {
            mygpio_free_pair(pair);
            return NULL;
        }
        mygpio_free_pair(pair);
    }
    else if (direction == MYGPIO_GPIO_DIRECTION_OUT) {
        if ((pair = mygpio_recv_pair_name(connection, "drive")) == NULL) {
            return NULL;
        }
        if ((drive = mygpio_gpio_parse_drive(pair->value)) == MYGPIO_DRIVE_UNKNOWN) {
            mygpio_free_pair(pair);
            return NULL;
        }
        mygpio_free_pair(pair);
    }

    struct t_mygpio_gpio *gpio = mygpio_gpio_new(direction);
    assert(gpio);
    gpio->gpio = gpio_nr;
    gpio->direction = direction;
    gpio->value = value;
    if (direction == MYGPIO_GPIO_DIRECTION_IN) {
        gpio->in->active_low = active_low;
        gpio->in->bias = bias;
        gpio->in->event_request = event_request;
        gpio->in->is_debounced = is_debounced;
        gpio->in->debounce_period_us = debounce_period_us;
        gpio->in->event_clock = event_clock;
    }
    else if (direction == MYGPIO_GPIO_DIRECTION_OUT) {
        gpio->out->drive = drive;
    }
    return gpio;
}
