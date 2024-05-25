--- Populate the global mympd_state lua table
function mympd.init()
  return mympd.api("INTERNAL_API_SCRIPT_INIT")
end

--- Calls the myMPD jsonrpc api
-- @param method
-- @param params
-- @return 0 for success, else 1
-- @return jsonrpc result
function mympd.api(method, params)
  local rc, raw_result = mympd_api(mympd_env.partition, method, json.encode(params))
  local result = json.decode(raw_result)
  if rc == 0 then
    return rc, result["result"]
  end
  return rc, result["error"]
end
