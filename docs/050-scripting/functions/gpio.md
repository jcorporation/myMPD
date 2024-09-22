
title: GPIO
---

The GPIO interface requires a configured [myGPIOd](https://github.com/jcorporation/myGPIOd).

All functions are connecting to the socket `/run/mygpiod/socket`, issues the command and disconnects.

```lua
-- Set non default socket for myGPIOd connection
mympd.mygpiod_socket = "/run/mygpiod/socket.debug"

-- Blink a GPIO at given timeout and interval
local rc = mympd.gpio_blink(gpio, timeout_ms, interval_ms)

-- Get the active state of a GPIO
-- 0 = inactive, 1 = active
local state = mympd.gpio_get(gpio)

-- Sets the active state of a GPIO
-- 0 = inactive, 1 = active
local rc = mympd.gpio_set(gpio, state)

-- Toggles the active state of a GPIO
local rc = mympd.gpio_toggle(gpio)
```
