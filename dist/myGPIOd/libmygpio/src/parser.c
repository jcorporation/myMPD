/*
 SPDX-License-Identifier: GPL-3.0-or-later
 libmygpio (c) 2020-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "libmygpio/include/libmygpio/libmygpio_gpio.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Lookups the name for the gpio direction.
 * @param direction the gpio direction.
 * @return gpio direction name
 */
const char *mygpio_gpio_lookup_direction(enum mygpio_gpio_direction direction) {
    switch(direction) {
        case MYGPIO_GPIO_DIRECTION_IN:
            return "in";
        case MYGPIO_GPIO_DIRECTION_OUT:
            return "out";
        case MYGPIO_GPIO_DIRECTION_UNKNOWN:
            return "unknown";
    }
    return "unknown";
}

/**
 * Parses a string to the gpio direction.
 * @param str string to parse
 * @return direction of the gpio
 */
enum mygpio_gpio_direction mygpio_gpio_parse_direction(const char *str) {
    if (strcmp(str, "in") == 0) {
        return MYGPIO_GPIO_DIRECTION_IN;
    }
    if (strcmp(str, "out") == 0) {
        return MYGPIO_GPIO_DIRECTION_OUT;
    }
    return MYGPIO_GPIO_DIRECTION_UNKNOWN;
}

/**
 * Lookups the name for the gpio value.
 * @param value the gpio value.
 * @return gpio value name
 */
const char *mygpio_gpio_lookup_value(enum mygpio_gpio_value value) {
    switch(value) {
        case MYGPIO_GPIO_VALUE_ACTIVE:
            return "active";
        case MYGPIO_GPIO_VALUE_INACTIVE:
            return "inactive";
        case MYGPIO_GPIO_VALUE_UNKNOWN:
            return "unknown";
    }
    return "unknown";
}

/**
 * Parses a string to a gpio value.
 * @param str string to parse
 * @return gpio value or GPIO_VALUE_LOW on error
 */
enum mygpio_gpio_value mygpio_gpio_parse_value(const char *str) {
    if (strcasecmp(str, "active") == 0) {
        return MYGPIO_GPIO_VALUE_ACTIVE;
    }
    if (strcasecmp(str, "inactive") == 0) {
        return MYGPIO_GPIO_VALUE_INACTIVE;
    }
    return MYGPIO_GPIO_VALUE_UNKNOWN;
}

/**
 * Lookups the name for the gpio bias.
 * @param bias the gpio bias.
 * @return gpio bias name
 */
const char *mygpio_gpio_lookup_bias(enum mygpio_gpio_bias bias) {
    switch(bias) {
        case MYGPIO_BIAS_AS_IS:
            return "as-is";
        case MYGPIO_BIAS_DISABLED:
            return "disable";
        case MYGPIO_BIAS_PULL_DOWN:
            return "pull-down";
        case MYGPIO_BIAS_PULL_UP:
            return "pull-up";
        case MYGPIO_EVENT_REQUEST_UNKNOWN:
            return "unknown";
    }
    return "unknown";
}

/**
 * Parses a string to a gpio bias.
 * @param str string to parse
 * @return gpio bias or GPIO_BIAS_UNKNOWN on error
 */
enum mygpio_gpio_bias mygpio_gpio_parse_bias(const char *str) {
    if (strcasecmp(str, "as-is") == 0) {
        return MYGPIO_BIAS_AS_IS;
    }
    if (strcasecmp(str, "disable") == 0) {
        return MYGPIO_BIAS_DISABLED;
    }
    if (strcasecmp(str, "pull-down") == 0) {
        return MYGPIO_BIAS_PULL_DOWN;
    }
    if (strcasecmp(str, "pull-up") == 0) {
        return MYGPIO_BIAS_PULL_UP;
    }
    return MYGPIO_BIAS_UNKNOWN;
}

/**
 * Lookups the name for an event request.
 * @param value the gpio event request.
 * @return gpio value name
 */
const char *mygpio_gpio_lookup_event_request(enum mygpio_event_request event_request) {
    switch(event_request) {
        case MYGPIO_EVENT_REQUEST_FALLING:
            return "falling";
        case MYGPIO_EVENT_REQUEST_RISING:
            return "rising";
        case MYGPIO_EVENT_REQUEST_BOTH:
            return "both";
        case MYGPIO_EVENT_REQUEST_UNKNOWN:
            return "unknown";
    }
    return "unknown";
}

/**
 * Parses a string to an event request.
 * @param str string to parse
 * @return gpio event request or GPIO_EVENT_REQUEST_UNKNOWN on error
 */
enum mygpio_event_request mygpio_gpio_parse_event_request(const char *str) {
    if (strcasecmp(str, "falling") == 0) {
        return MYGPIO_EVENT_REQUEST_FALLING;
    }
    if (strcasecmp(str, "rising") == 0) {
        return MYGPIO_EVENT_REQUEST_RISING;
    }
    if (strcasecmp(str, "both") == 0) {
        return MYGPIO_EVENT_REQUEST_BOTH;
    }
    return MYGPIO_EVENT_REQUEST_UNKNOWN;
}

/**
 * Lookups the name for the gpio event clock.
 * @param clock the gpio clock.
 * @return gpio clock name
 */
const char *mygpio_gpio_lookup_event_clock(enum mygpio_event_clock clock) {
    switch(clock) {
        case MYGPIO_EVENT_CLOCK_MONOTONIC:
            return "monotonic";
        case MYGPIO_EVENT_CLOCK_REALTIME:
            return "realtime";
        case MYGPIO_EVENT_CLOCK_HTE:
            return "hte";
        case MYGPIO_EVENT_CLOCK_UNKNOWN:
            return "unknown";
    }
    return "unknown";
}

/**
 * Parses a string to a gpio event clock.
 * @param str string to parse
 * @return gpio event clock or MYGPIO_EVENT_CLOCK_UNKNOWN on error
 */
enum mygpio_event_clock mygpio_gpio_parse_event_clock(const char *str) {
    if (strcasecmp(str, "monotonic") == 0) {
        return MYGPIO_EVENT_CLOCK_MONOTONIC;
    }
    if (strcasecmp(str, "realtime") == 0) {
        return MYGPIO_EVENT_CLOCK_REALTIME;
    }
    if (strcasecmp(str, "hte") == 0) {
        return MYGPIO_EVENT_CLOCK_HTE;
    }
    return MYGPIO_EVENT_CLOCK_UNKNOWN;
}

/**
 * Lookups the name for the gpio drive setting.
 * @param drive the gpio drive.
 * @return gpio drive name
 */
const char *mygpio_gpio_lookup_drive(enum mygpio_drive drive) {
    switch(drive) {
        case MYGPIO_DRIVE_PUSH_PULL:
            return "push-pull";
        case MYGPIO_DRIVE_OPEN_DRAIN:
            return "open-drain";
        case MYGPIO_DRIVE_OPEN_SOURCE:
            return "open-source";
        case MYGPIO_DRIVE_UNKNOWN:
            return "unknown";
    }
    return "unknown";
}

/**
 * Parses a string to a gpio drive.
 * @param str string to parse
 * @return gpio bias or MYGPIO_DRIVE_UNKNOWN on error
 */
enum mygpio_drive mygpio_gpio_parse_drive(const char *str) {
    if (strcasecmp(str, "push-pull") == 0) {
        return MYGPIO_DRIVE_PUSH_PULL;
    }
    if (strcasecmp(str, "open-drain") == 0) {
        return MYGPIO_DRIVE_OPEN_DRAIN;
    }
    if (strcasecmp(str, "open-source") == 0) {
        return MYGPIO_DRIVE_OPEN_SOURCE;
    }
    return MYGPIO_DRIVE_UNKNOWN;
}
