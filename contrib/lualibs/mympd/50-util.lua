---
--- myMPD utility functions
---

-- Jsonrpc severities
local jsonrpc_severities = {
  SEVERITY_INFO = 0,
  SEVERITY_WARNING = 1,
  SEVERITY_ERR = 2
}

--- Sends a notification to the client that started this script.
-- @param severity Severity
--                 0 = SEVERITY_INFO = Info
--                 1 = SEVERITY_WARNING = Warning
--                 2 = SEVERITY_ERR = Error
-- @param message Jsonrpc message to send
function mympd.notify_client(severity, message)
  if type(severity) == "string" then
    severity = jsonrpc_severities[severity]
  end
  mympd_util_notify(mympd_env.partition, mympd_env.requestid, severity, message)
end

--- Sends a notification to all clients in the current partition.
-- @param severity Severity
--                 0 = SEVERITY_INFO = Info
--                 1 = SEVERITY_WARNING = Warning
--                 2 = SEVERITY_ERR = Error
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
