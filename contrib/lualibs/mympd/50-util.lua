---
--- myMPD utility functions
---

--- Sends a notification to the client that started this script.
-- @param severity Severity
--                 0 = Info
--                 1 = Warning
--                 2 = Error
-- @param message Jsonrpc message to send
function mympd.notify_client(severity, message)
  mympd_util_notify(mympd_env.partition, mympd_env.requestid, severity, message)
end

--- Sends a notification to all clients in the current partition.
-- @param severity Severity
--                 0 = Info
--                 1 = Warning
--                 2 = Error
-- @param message Jsonrpc message to send
function mympd.notify_partition(severity, message)
  mympd_util_notify(mympd_env.partition, 0, severity, message)
end

--- Logs to the myMPD log.
-- @param loglevel Syslog loglevel
--                 0 = Emergency
--                 1 = Alert
--                 2 = Critical
--                 3 = Error
--                 4 = Warning
--                 5 = Notice
--                 6 = Info
--                 7 = Debug
function mympd.log(loglevel, message)
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
