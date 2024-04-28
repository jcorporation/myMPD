/*
 SPDX-License-Identifier: GPL-3.0-or-later
 libmygpio (c) 2020-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef LIBMYGPIO_SRC_GPIO_H
#define LIBMYGPIO_SRC_GPIO_H

#include "libmygpio/include/libmygpio/libmygpio_gpio_struct.h"

struct t_mygpio_in {
    bool active_low;
    enum mygpio_gpio_bias bias;
    enum mygpio_event_request event_request;
    bool is_debounced;
    int debounce_period_us;
    enum mygpio_event_clock event_clock;
};

struct t_mygpio_out {
    enum mygpio_drive drive;
};

/**
 * Struct holding the configuration of a GPIO.
 */
struct t_mygpio_gpio {
    unsigned gpio;                         //!< GPIO number
    enum mygpio_gpio_direction direction;  //!< GPIO direction
    enum mygpio_gpio_value value;          //!< GPIO value
    struct t_mygpio_in *in;                //!< GPIO settings for input
    struct t_mygpio_out *out;              //!< GPIO settings for output
};

struct t_mygpio_gpio *mygpio_gpio_new(enum mygpio_gpio_direction direction);

#endif
