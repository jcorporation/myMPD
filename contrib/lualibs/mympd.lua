--
-- mympd.lua
--
-- SPDX-License-Identifier: GPL-3.0-or-later
-- myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
-- https://github.com/jcorporation/mympd
--

mympd = { _version = "0.4.0" }

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

return mympd
