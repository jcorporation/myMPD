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

#ifndef LIBMYGPIO_PARSER_H
#define LIBMYGPIO_PARSER_H

#include "libmygpio_gpio_struct.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup libmygpio_parser Parser
 *
 * @brief This module provides parsing and lookup functions for GPIO settings.
 *
 * @{
 */

/**
 * Lookups the name for the gpio direction.
 * @param direction the gpio direction.
 * @return gpio direction name
 */
const char *mygpio_gpio_lookup_direction(enum mygpio_gpio_direction direction);

/**
 * Parses a string to the gpio direction.
 * @param str string to parse
 * @return direction of the gpio
 */
enum mygpio_gpio_direction mygpio_gpio_parse_direction(const char *str);

/**
 * Lookups the name for the gpio value.
 * @param value the gpio value.
 * @return gpio value name
 */
const char *mygpio_gpio_lookup_value(enum mygpio_gpio_value value);

/**
 * Parses a string to a gpio value.
 * @param str string to parse
 * @return gpio value or GPIO_VALUE_UNKNOWN on error
 */
enum mygpio_gpio_value mygpio_gpio_parse_value(const char *str);

/**
 * Lookups the name for the gpio bias.
 * @param bias the gpio bias.
 * @return gpio bias name
 */
const char *mygpio_gpio_lookup_bias(enum mygpio_gpio_bias bias);

/**
 * Parses a string to a gpio bias.
 * @param str string to parse
 * @return gpio bias or GPIO_BIAS_UNKNOWN on error
 */
enum mygpio_gpio_bias mygpio_gpio_parse_bias(const char *str);

/**
 * Lookups the name for an event request.
 * @param event_request the gpio event request.
 * @return gpio event request name
 */
const char *mygpio_gpio_lookup_event_request(enum mygpio_event_request event_request);

/**
 * Parses a string to an event request.
 * @param str string to parse
 * @return gpio event request or GPIO_EVENT_REQUEST_UNKNOWN on error
 */
enum mygpio_event_request mygpio_gpio_parse_event_request(const char *str);

/**
 * Lookups the name for the gpio event clock.
 * @param clock the gpio clock.
 * @return gpio clock name
 */
const char *mygpio_gpio_lookup_event_clock(enum mygpio_event_clock clock);

/**
 * Parses a string to a gpio event clock.
 * @param str string to parse
 * @return gpio event clock or MYGPIO_EVENT_CLOCK_UNKNOWN on error
 */
enum mygpio_event_clock mygpio_gpio_parse_event_clock(const char *str);

/**
 * Lookups the name for the gpio drive setting.
 * @param drive the gpio drive.
 * @return gpio drive name
 */
const char *mygpio_gpio_lookup_drive(enum mygpio_drive drive);

/**
 * Parses a string to a gpio drive.
 * @param str string to parse
 * @return gpio bias or MYGPIO_DRIVE_UNKNOWN on error
 */
enum mygpio_drive mygpio_gpio_parse_drive(const char *str);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
