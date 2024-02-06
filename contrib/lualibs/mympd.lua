--
-- mympd.lua
--
-- SPDX-License-Identifier: GPL-3.0-or-later
-- myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
-- https://github.com/jcorporation/mympd
--

mympd = { _version = "0.5.0" }

--
-- Calls the myMPD jsonrpc api
--
function mympd.api(method, params)
  rc, raw_result = mympd_api(method, json.encode(params))
  result = json.decode(raw_result)
  if rc == 0 then
    return rc, result["result"]
  end
  return rc, result["error"]
end

--
-- Simple HTTP client
--
function mympd.http_client(method, uri, headers, payload)
  rc, code, header, body = mympd_api_http_client(method, uri, headers, payload)
  return rc, code, header, body
end

--
-- Populate the global mympd_state lua table
--
function mympd.init()
  return mympd.api("INTERNAL_API_SCRIPT_INIT")
end

--
-- Execute a system command and capture its output
--
function mympd.os_capture(cmd)
  if io then
    local handle = assert(io.popen(cmd, 'r'))
    local output = assert(handle:read('*a'))
    handle:close()

    output = string.gsub(
      string.gsub(
        string.gsub(output, '^%s+', ''),
          '%s+$',
          ''
      ),
      '[\n\r]+',
      ' '
    )
    return output
  else
    return "io library must be loaded"
  end
end

---
--- myGPIOd integration functions
--- This functions requires a running myGPIOd service.
--- myMPD must be compiled with libmygpio.
---

mympd.mygpiod_socket = "/run/mygpiod/socket"

--
-- Blinks a GPIO with given timeout and interval
--
function mympd.gpio_blink(gpio, timeout_ms, interval_ms)
  return mygpio_gpio_blink(mymypd.mygpiod_socket, gpio, timeout_ms, interval_ms)
end

--
-- Returns the active state of a GPIO
--
function mympd.gpio_get(gpio)
  return mygpio_gpio_get(mymypd.mygpiod_socket, gpio)
end

--
-- Sets the active value of a GPIO
--
function mympd.gpio_set(gpio, value)
  return mygpio_gpio_set(mymypd.mygpiod_socket, gpio, value)
end

--
-- Toggles the active value of a GPIO
--
function mympd.gpio_toggle(gpio)
  return mygpio_gpio_toggle(mymypd.mygpiod_socket, gpio)
end

return mympd
