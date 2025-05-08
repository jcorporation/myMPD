---
--- myMPD utility functions
---

-- Jsonrpc severities
local jsonrpc_severities = {
  SEVERITY_EMERG = 0,
  SEVERITY_ALERT = 1,
  SEVERITY_CRIT = 2,
  SEVERITY_ERR = 3,
  SEVERITY_WARNING = 4,
  SEVERITY_NOTICE = 5,
  SEVERITY_INFO = 6,
  SEVERITY_DEBUG = 7
}

local jsonrpc_severity_names = {
  ["0"] = "emerg",
  ["1"] = "alert",
  ["2"] = "crit",
  ["3"] = "error",
  ["4"] = "warn",
  ["5"] = "notice",
  ["6"] = "info",
  ["7"] = "debug"
}

--- Sends a notification to the client that started this script.
-- @param severity Severity
--                 0 = SEVERITY_EMERG = Emergency
--                 1 = SEVERITY_ALERT = Alert
--                 2 = SEVERITY_CRIT = Critical
--                 3 = SEVERITY_ERR = Error
--                 4 = SEVERITY_WARNING = Warning
--                 5 = SEVERITY_NOTICE = Notice
--                 6 = SEVERITY_INFO = Info
--                 7 = SEVERITY_DEBUG = Debug
-- @param message Jsonrpc message to send
function mympd.notify_client(severity, message)
  if type(severity) == "string" then
    severity = jsonrpc_severities[severity]
  end
  mympd_util_notify(mympd_env.partition, mympd_env.requestid, severity, message)
end

--- Sends a notification to all clients in the current partition.
-- @param severity Severity
--                 0 = SEVERITY_EMERG = Emergency
--                 1 = SEVERITY_ALERT = Alert
--                 2 = SEVERITY_CRIT = Critical
--                 3 = SEVERITY_ERR = Error
--                 4 = SEVERITY_WARNING = Warning
--                 5 = SEVERITY_NOTICE = Notice
--                 6 = SEVERITY_INFO = Info
--                 7 = SEVERITY_DEBUG = Debug
-- @param message Jsonrpc message to send
function mympd.notify_partition(severity, message)
  if type(severity) == "string" then
    severity = jsonrpc_severities[severity]
  end
  mympd_util_notify(mympd_env.partition, 0, severity, message)
end

-- Posix loglevel names
local loglevel_names = {
  LOG_EMERG = 0,
  LOG_ALERT = 1,
  LOG_CRIT = 2,
  LOG_ERR = 3,
  LOG_WARNING = 4,
  LOG_NOTICE = 5,
  LOG_INFO = 6,
  LOG_DEBUG = 7
}

--- Logs to the myMPD log.
-- @param loglevel Syslog loglevel
--                 0 = LOG_EMERG = Emergency
--                 1 = LOG_ALERT = Alert
--                 2 = LOG_CRIT = Critical
--                 3 = LOG_ERR = Error
--                 4 = LOG_WARNING = Warning
--                 5 = LOG_NOTICE = Notice
--                 6 = LOG_INFO = Info
--                 7 = LOG_DEBUG = Debug
function mympd.log(loglevel, message)
  if type(loglevel) == "string" then
    loglevel = loglevel_names[loglevel]
  end
  mympd_util_log(mympd_env.partition, mympd_env.scriptname, loglevel, message)
end

--- Creates a JSONRPC 2.0 notification.
-- @param severity Severity
--                 0 = SEVERITY_EMERG = Emergency
--                 1 = SEVERITY_ALERT = Alert
--                 2 = SEVERITY_CRIT = Critical
--                 3 = SEVERITY_ERR = Error
--                 4 = SEVERITY_WARNING = Warning
--                 5 = SEVERITY_NOTICE = Notice
--                 6 = SEVERITY_INFO = Info
--                 7 = SEVERITY_DEBUG = Debug
-- @param msg Error message
-- @return HTTP response
function mympd.jsonrpc_notification(severity, msg)
  if type(severity) == "string" then
    severity = jsonrpc_severities[severity]
  end
  return json.encode({
    jsonrpc = "2.0",
    method = "notify",
    params = {
      facility = "script",
      severity = jsonrpc_severity_names[tostring(severity)],
      message = msg,
      data = {}
    }
  })
end

--- Creates a JSONRPC 2.0 error.
-- @param msg Error message
-- @return String with jsonrpc message
function mympd.jsonrpc_error(msg)
  return mympd.jsonrpc_notification(3, msg)
end

--- Creates a JSONRPC 2.0 warning.
-- @param msg Error message
-- @return String with jsonrpc message
function mympd.jsonrpc_warn(msg)
  return mympd.jsonrpc_notification(4, msg)
end

--- Returns the MD5 hash of string.
-- @param str String to hash
-- @return MD5 hash of string
function mympd.hash_md5(str)
  return mympd_util_hash(str, "md5")
end

--- Returns the SHA1 hash of string.
-- @param str String to hash
-- @return SHA1 hash of string
function mympd.hash_sha1(str)
  return mympd_util_hash(str, "sha1")
end

--- Returns the SHA256 hash of string.
-- @param str String to hash
-- @return SHA256 hash of string
function mympd.hash_sha256(str)
  return mympd_util_hash(str, "sha256")
end

--- URL decoding
-- @param str String to URL decode
-- @param form true = decode a url form encoded string
-- @return Decoded string
function mympd.urldecode(str, form)
  return mympd_util_urldecode(str, form)
end

--- URL encoding
-- @param str String to URL encode
-- @return Encoded string
function mympd.urlencode(str)
  return mympd_util_urlencode(str)
end

--- Simple HTML encoding
-- @param str String to HTML encode
-- @return Encoded string
function mympd.htmlencode(str)
  str = string.gsub(str, "<", "&lt;")
  str = string.gsub(str, "&", "&amp;")
  str = string.gsub(str, "\"", "&quot;")
  str = string.gsub(str, "'", "&#39;")
  return str
end

--- Sleep ms
-- @param ms Milliseconds to sleep
function mympd.sleep(ms)
  mympd_util_sleep(ms)
end

--- Read an ascii file
-- @param path Filename
-- @return File content or nil on error
function mympd.read_file(path)
  local file = io.open(path, "r")
  if not file then
      return nil
  end
  local content = file:read "*a"
  file:close()
  return content
end

--- Wrapper for os.remove that logs the error on failure
-- @param path File to remove
-- @return true on success, else nil
-- @return Error message on failure
function mympd.remove_file(path)
  local rc, errorstr = os.remove(path)
  if rc == nil then
    mympd.log(3, "Failure removing ".. path .. ": " .. errorstr)
  end
  return rc, errorstr
end

--- Checks arguments from the mympd_arguments global variable.
-- @param tocheck Table of arguments with options to check
--                Available options: notempty, required, number
-- @return true on success, else nil
-- @return Error message on failure
function mympd.check_arguments(tocheck)
  for arg, option in pairs(tocheck) do
    if mympd_arguments[arg] == nil then
      mympd.log(3, "Argument " .. arg  .. " not found.")
      return false, "Argument " .. arg  .. " not found."
    end
    if option == "notempty" then
      if mympd_arguments[arg] == "" then
        mympd.log(3, "Argument " .. arg  .. " is empty.")
        return false, "Argument " .. arg  .. " is empty."
      end
    elseif option == 'number' then
      if mympd_arguments[arg] == nil or tonumber(mympd_arguments[arg]) == nil then
        mympd.log(3, "Argument " .. arg  .. " is not a number.")
        return false, "Argument " .. arg  .. " is not a number."
      end
    end
  end
  return true
end
