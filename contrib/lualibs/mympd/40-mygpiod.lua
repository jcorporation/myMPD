---
--- myGPIOd integration functions
--- This functions requires a running myGPIOd REST-API service.
---

-- Default myGPIOd uri
mympd.mygpiod_uri = "http://localhost:8081/api"

--- Returns the active state of a GPIO
-- @param gpio GPIO number
-- @return GPIO state: active or inactive
function mympd.gpio_get(gpio)
  local uri = mympd.mygpiod_uri .. "gpio/" .. gpio
  local rc, code, headers, body = mympd.http_client("GET", uri , "", "", false)
  if rc == 0 then
    local decoded = json.decode(body)
    return decoded.value
  end
  return nil
end

--- Blinks a GPIO with given timeout and interval
-- @param gpio GPIO number
-- @param timeout_ms Timeout in milliseconds for first blink
-- @param interval_ms Blink interval in milliseconds
-- @return 0 on success
function mympd.gpio_blink(gpio, timeout_ms, interval_ms)
  local uri = mympd.mygpiod_uri .. "gpio/" .. gpio .. "/blink?timeout=" .. timeout_ms .. "&interval=" .. interval_ms
  local rc, code, headers, body = mympd.http_client("PATCH", uri , "", "", false)
  return rc
end

--- Sets the active value of a GPIO
-- @param gpio GPIO number
-- @param value active or inactive
-- @return 0 on success
function mympd.gpio_set(gpio, value)
  local uri = mympd.mygpiod_uri .. "gpio/" .. gpio .. "/set?value=" .. value
  local rc, code, headers, body = mympd.http_client("PATCH", uri , "", "", false)
  return rc
end

--- Toggles the active value of a GPIO
-- @param gpio GPIO number
-- @return 0 on success
function mympd.gpio_toggle(gpio)
  local uri = mympd.mygpiod_uri .. "gpio/" .. gpio .. "/toggle"
  local rc, code, headers, body = mympd.http_client("PATCH", uri , "", "", false)
  return rc
end
