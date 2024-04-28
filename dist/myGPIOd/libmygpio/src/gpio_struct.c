/*
 SPDX-License-Identifier: GPL-3.0-or-later
 libmygpio (c) 2020-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "libmygpio/src/gpio_struct.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Returns the GPIO number from struct t_mygpio_gpio.
 * @param gpio Pointer to struct t_mygpio_gpio.
 * @return GPIO number. 
 */
unsigned mygpio_gpio_get_gpio(struct t_mygpio_gpio *gpio) {
    return gpio->gpio;
}

/**
 * Returns the GPIO direction from struct t_mygpio_gpio.
 * @param gpio Pointer to struct t_mygpio_gpio.
 * @return GPIO direction, one of enum mygpio_gpio_direction.
 */
enum mygpio_gpio_direction mygpio_gpio_get_direction(struct t_mygpio_gpio *gpio) {
    return gpio->direction;
}

/**
 * Returns the GPIO value from struct t_mygpio_gpio.
 * @param gpio Pointer to struct t_mygpio_gpio.
 * @return GPIO value, one of enum mygpio_gpio_value.
 */
enum mygpio_gpio_value mygpio_gpio_get_value(struct t_mygpio_gpio *gpio) {
    return gpio->value;
}

/**
 * Returns the GPIO active_low from struct t_mygpio_gpio.
 * @param gpio Pointer to struct t_mygpio_gpio.
 * @return GPIO is set to active_low?
 */
bool mygpio_gpio_in_get_active_low(struct t_mygpio_gpio *gpio) {
    assert(gpio->in);
    return gpio->in->active_low;
}

/**
 * Returns the GPIO bias from struct t_mygpio_gpio.
 * @param gpio Pointer to struct t_mygpio_gpio.
 * @return GPIO bias, one of enum mygpio_gpio_bias.
 */
enum mygpio_gpio_bias mygpio_gpio_in_get_bias(struct t_mygpio_gpio *gpio) {
    assert(gpio->in);
    return gpio->in->bias;
}

/**
 * Returns the requested events from struct t_mygpio_gpio.
 * @param gpio Pointer to struct t_mygpio_gpio.
 * @return requested GPIO events, one of enum event_request.
 */
enum mygpio_event_request mygpio_gpio_in_get_event_request(struct t_mygpio_gpio *gpio) {
    assert(gpio->in);
    return gpio->in->event_request;
}

/**
 * Returns if the GPIO is debounced.
 * @param gpio Pointer to struct t_mygpio_gpio.
 * @return GPIO debounced?
 */
bool mygpio_gpio_in_get_is_debounced(struct t_mygpio_gpio *gpio) {
    assert(gpio->in);
    return gpio->in->is_debounced;
}

/**
 * Returns the GPIO debounce period from struct t_mygpio_gpio.
 * @param gpio Pointer to struct t_mygpio_gpio.
 * @return GPIO debounce period in microseconds.
 */
int mygpio_gpio_in_get_debounce_period_us(struct t_mygpio_gpio *gpio) {
    assert(gpio->in);
    return gpio->in->debounce_period_us;
}

/**
 * Returns the GPIO event clock from struct t_mygpio_gpio.
 * @param gpio Pointer to struct t_mygpio_gpio.
 * @return GPIO event clock, one of enum mygpio_event_clock.
 */
enum mygpio_event_clock mygpio_gpio_in_get_event_clock(struct t_mygpio_gpio *gpio) {
    assert(gpio->in);
    return gpio->in->event_clock;
}

/**
 * Returns the GPIO drive setting from struct t_mygpio_gpio.
 * @param gpio Pointer to struct t_mygpio_gpio.
 * @return GPIO drive setting, one of enum mygpio_drive.
 */
enum mygpio_drive mygpio_gpio_out_get_drive(struct t_mygpio_gpio *gpio) {
    assert(gpio->out);
    return gpio->out->drive;
}

/**
 * Creates a new gpio struct
 * @return struct t_mygpio_gpio* 
 */
struct t_mygpio_gpio *mygpio_gpio_new(enum mygpio_gpio_direction direction) {
    struct t_mygpio_gpio *gpio = malloc(sizeof(struct t_mygpio_gpio));
    assert(gpio);
    gpio->in = NULL;
    gpio->out = NULL;
    if (direction == MYGPIO_GPIO_DIRECTION_IN) {
        gpio->in = malloc(sizeof(struct t_mygpio_in));
        assert(gpio->in);
    }
    else if (direction == MYGPIO_GPIO_DIRECTION_OUT) {
        gpio->out = malloc(sizeof(struct t_mygpio_out));
        assert(gpio->out);
    }
    return gpio;
}

/**
 * Frees the gpio struct
 * @param gpio 
 */
void mygpio_free_gpio(struct t_mygpio_gpio *gpio) {
    if (gpio->in != NULL) {
        free(gpio->in);
    }
    if (gpio->out != NULL) {
        free(gpio->out);
    }
    free(gpio);
}
