---
--- myMPD tmp var functions
---

--- Sets a temporary variable
-- @param key Variable name
-- @param value Variable value
-- @param lifetime Lifetime of the variable
-- @return 0 for success, else 1
-- @return jsonrpc result for success, else error
function mympd.tmpvar_set(key, value, lifetime)
  return mympd.api("MYMPD_API_SCRIPT_TMP_SET",
    {
      key = key,
      value = value,
      lifetime = lifetime
    }
  )
end

--- Deletes a temporary variable
-- @param key Variable name
-- @return 0 for success, else 1
-- @return jsonrpc result for success, else error
function mympd.tmpvar_delete(key)
  return mympd.api("MYMPD_API_SCRIPT_TMP_DELETE",
    {
      key = key
    }
  )
end

--- Gets a temporary variable
-- @param key Variable name
-- @return Variable value
-- @return Unix timestamp of variable expiration
function mympd.tmpvar_get(key)
  local rc, result = mympd.api("MYMPD_API_SCRIPT_TMP_GET",
    {
      key = key
    }
  )
  if rc == 0 then
    return result.value, result.expires
  end
  mympd.log(3, result.message)
  return "", 0
end

--- Lists all temporary variable
-- @return Table of variables or nil on error
function mympd.tmpvar_list()
  local rc, result = mympd.api("MYMPD_API_SCRIPT_TMP_LIST")
  if rc == 0 then
    return result.data
  end
  mympd.log(3, result.message)
  return nil
end
