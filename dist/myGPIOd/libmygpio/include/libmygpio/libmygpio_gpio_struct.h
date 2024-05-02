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

#ifndef LIBMYGPIO_GPIO_STRUCT_H
#define LIBMYGPIO_GPIO_STRUCT_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct t_mygpio_connection;

/**
 * @struct t_mygpio_gpio
 * @{
 * The opaque GPIO object. You can not access it directly.
 * Refer to @ref libmygpio_gpio_settings for function that operate on this struct.
 * @}
 */
struct t_mygpio_gpio;

/**
 * @defgroup libmygpio_gpio_settings GPIO
 *
 * @brief This module provides functions to access the t_mygpio_gpio struct,
 * received by gpioinfo or gpiolist.
 *
 * @{
 */

/**
 * The direction of a GPIO.
 */
enum mygpio_gpio_direction {
    MYGPIO_GPIO_DIRECTION_UNKNOWN = -1,  //!< Unknown GPIO direction.
    MYGPIO_GPIO_DIRECTION_IN,            //!< Input direction, myGPIOd can read events from this GPIO.
    MYGPIO_GPIO_DIRECTION_OUT            //!< Output direction, myGPIOd can set the value to: MYGPIO_GPIO_VALUE_ACTIVE or MYGPIO_GPIO_VALUE_INACTIVE.
};

/**
 * The value of a GPIO.
 */
enum mygpio_gpio_value {
    MYGPIO_GPIO_VALUE_UNKNOWN = -1,  //!< Unknown GPIO value
    MYGPIO_GPIO_VALUE_INACTIVE,      //!< GPIO state is inactive
    MYGPIO_GPIO_VALUE_ACTIVE         //!< GPIO state is active
};

/**
 * Bias setting for an input GPIO.
 */
enum mygpio_gpio_bias {
    MYGPIO_BIAS_UNKNOWN = -1,  //!< Unknown bias setting
    MYGPIO_BIAS_AS_IS,         //!< Do not touch the bias state
    MYGPIO_BIAS_DISABLED,      //!< Disable the bias
    MYGPIO_BIAS_PULL_DOWN,     //!< Pull-down the GPIO
    MYGPIO_BIAS_PULL_UP        //!< Pull-up the GPIO
};

/**
 * Events requested for an input GPIO.
 */
enum mygpio_event_request {
    MYGPIO_EVENT_REQUEST_UNKNOWN = -1,  //!< Unknown event request setting
    MYGPIO_EVENT_REQUEST_FALLING,       //!< Request falling events
    MYGPIO_EVENT_REQUEST_RISING,        //!< Request rising events
    MYGPIO_EVENT_REQUEST_BOTH           //!< Request falling and rising events
};

/**
 * Clock setting for an input GPIO.
 */
enum mygpio_event_clock {
    MYGPIO_EVENT_CLOCK_UNKNOWN = -1,  //!< Unknown event clock setting
    MYGPIO_EVENT_CLOCK_MONOTONIC,     //!< Monotonic clock
    MYGPIO_EVENT_CLOCK_REALTIME,      //!< Realtime clock
    MYGPIO_EVENT_CLOCK_HTE            //!< Hardware timestamp engine
};

/**
 * Drive setting for an output GPIO.
 */
enum mygpio_drive {
    MYGPIO_DRIVE_UNKNOWN = -1,  //!< Unknown drive setting
    MYGPIO_DRIVE_PUSH_PULL,     //!< Drive setting is push-pull
    MYGPIO_DRIVE_OPEN_DRAIN,    //!< Drive setting is open-drain
    MYGPIO_DRIVE_OPEN_SOURCE    //!< Drive setting is open-source
};

/**
 * Returns the GPIO number from struct t_mygpio_gpio.
 * @param gpio Pointer to struct t_mygpio_gpio.
 * @return GPIO number. 
 */
unsigned mygpio_gpio_get_gpio(struct t_mygpio_gpio *gpio);

/**
 * Returns the GPIO direction from struct t_mygpio_gpio.
 * @param gpio Pointer to struct t_mygpio_gpio.
 * @return GPIO direction, one of enum mygpio_gpio_direction.
 */
enum mygpio_gpio_direction mygpio_gpio_get_direction(struct t_mygpio_gpio *gpio);

/**
 * Returns the GPIO value from struct t_mygpio_gpio.
 * @param gpio Pointer to struct t_mygpio_gpio.
 * @return GPIO value, one of enum mygpio_gpio_value.
 */
enum mygpio_gpio_value mygpio_gpio_get_value(struct t_mygpio_gpio *gpio);

/**
 * Returns the GPIO active_low from struct t_mygpio_gpio.
 * @param gpio Pointer to struct t_mygpio_gpio.
 * @return GPIO is set to active_low?
 */
bool mygpio_gpio_in_get_active_low(struct t_mygpio_gpio *gpio);

/**
 * Returns the GPIO bias from struct t_mygpio_gpio.
 * @param gpio Pointer to struct t_mygpio_gpio.
 * @return GPIO bias, one of enum mygpio_gpio_bias.
 */
enum mygpio_gpio_bias mygpio_gpio_in_get_bias(struct t_mygpio_gpio *gpio);

/**
 * Returns the requested events from struct t_mygpio_gpio.
 * @param gpio Pointer to struct t_mygpio_gpio.
 * @return requested GPIO events, one of enum event_request.
 */
enum mygpio_event_request mygpio_gpio_in_get_event_request(struct t_mygpio_gpio *gpio);

/**
 * Returns true if the GPIO is debounced.
 * @param gpio Pointer to struct t_mygpio_gpio.
 * @return GPIO debounced?
 */
bool mygpio_gpio_in_get_is_debounced(struct t_mygpio_gpio *gpio);

/**
 * Returns the GPIO debounce period from struct t_mygpio_gpio.
 * @param gpio Pointer to struct t_mygpio_gpio.
 * @return GPIO debounce period in microseconds.
 */
int mygpio_gpio_in_get_debounce_period_us(struct t_mygpio_gpio *gpio);

/**
 * Returns the GPIO event clock from struct t_mygpio_gpio.
 * @param gpio Pointer to struct t_mygpio_gpio.
 * @return GPIO event clock, one of enum mygpio_event_clock.
 */
enum mygpio_event_clock mygpio_gpio_in_get_event_clock(struct t_mygpio_gpio *gpio);

/**
 * Returns the GPIO drive setting from struct t_mygpio_gpio.
 * @param gpio Pointer to struct t_mygpio_gpio.
 * @return GPIO drive setting, one of enum mygpio_drive.
 */
enum mygpio_drive mygpio_gpio_out_get_drive(struct t_mygpio_gpio *gpio);

/**
 * Frees the struct received by mygpio_recv_gpio.
 * @param gpio Pointer to struct mygpio_recv_gpio.
 */
void mygpio_free_gpio(struct t_mygpio_gpio *gpio);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
