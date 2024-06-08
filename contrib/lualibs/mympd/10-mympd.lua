--- Populate the global mympd_state lua table
-- @return 0 for success, else 1
-- @return jsonrpc result for success, else error
function mympd.init()
  return mympd.api("INTERNAL_API_SCRIPT_INIT")
end

--- Calls the myMPD jsonrpc api
-- @param method
-- @param params
-- @return 0 for success, else 1
-- @return jsonrpc result for success, else error
function mympd.api(method, params)
  local rc, raw_result = mympd_api(mympd_env.partition, method, json.encode(params))
  local result = json.decode(raw_result)
  if rc == 0 then
    return rc, result["result"]
  end
  return rc, result["error"]
end

--- Returns an Jsonrpc response for a script dialog.
-- @param title Dialog title
-- @param data Dialog definition
-- @param callback Script to call for the submit button
-- @return Jsonrpc response
function mympd.dialog(title, data, callback)
  return json.encode({
    jsonrpc = "2.0",
    method = "script_dialog",
    params = {
      facility = "script",
      severity = "info",
      message = title,
      data = data,
      callback = callback
    }
  })
end
