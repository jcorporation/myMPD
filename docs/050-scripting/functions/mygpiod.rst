GPIO
====

The GPIO interface requires a configured `myGPIOd <https://github.com/jcorporation/myGPIOd>`__.

All functions are using the integrated http client to connect to the REST-API of myGPIOd.

.. code:: lua

   -- Set non default uri for myGPIOd connection
   mympd.mygpiod_uri = "http://localhost:8081/api/"

   -- Blink a GPIO at given timeout and interval
   local rc = mympd.gpio_blink(gpio, timeout_ms, interval_ms)

   -- Get the active state of a GPIO
   local state = mympd.gpio_get(gpio)

   -- Sets the active state of a GPIO
   local rc = mympd.gpio_set(gpio, "<active|inactive>")

   -- Toggles the active state of a GPIO
   local rc = mympd.gpio_toggle(gpio)
