---
--- myGPIOd integration functions
--- This functions requires a running myGPIOd service.
--- myMPD must be compiled with libmygpio.
---

-- Default myGPIOd socket
mympd.mygpiod_socket = "/run/mygpiod/socket"

--- Blinks a GPIO with given timeout and interval
-- @param gpio GPIO number
-- @param timeout_ms Timeout in milliseconds for first blink
-- @param interval_ms Blink interval in milliseconds
-- @return 0 on success
function mympd.gpio_blink(gpio, timeout_ms, interval_ms)
  return mygpio_gpio_blink(mympd.mygpiod_socket, gpio, timeout_ms, interval_ms)
end

--- Returns the active state of a GPIO
-- @param gpio GPIO number
-- @return 0 on success
function mympd.gpio_get(gpio)
  return mygpio_gpio_get(mympd.mygpiod_socket, gpio)
end

--- Sets the active value of a GPIO
-- @param gpio GPIO number
-- @return 0 on success
function mympd.gpio_set(gpio, value)
  return mygpio_gpio_set(mympd.mygpiod_socket, gpio, value)
end

--- Toggles the active value of a GPIO
-- @param gpio GPIO number
-- @return 0 on success
function mympd.gpio_toggle(gpio)
  return mygpio_gpio_toggle(mympd.mygpiod_socket, gpio)
end
