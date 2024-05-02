--
-- Populate the global mympd_state lua table
--
function mympd.init()
  return mympd.api("INTERNAL_API_SCRIPT_INIT")
end

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
