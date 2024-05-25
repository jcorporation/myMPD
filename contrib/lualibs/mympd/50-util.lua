---
--- myMPD utility functions
---

--- Write a file for the covercache
-- @param src File to rename
-- @param uri URI to create the covercache file for
function mympd.covercache_write(src, uri)
  return mympd_util_covercache_write(mympd_env.cachedir, src, uri)
end

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

--- Returns the SHA1 hash of string.
-- @param string String to hash
-- @return SHA1 hash of string
function mympd.hash_sha1(string)
  return mympd_util_hash(string, "sha1")
end

--- Returns the SHA256 hash of string.
-- @param string String to hash
-- @return SHA256 hash of string
function mympd.hash_sha256(string)
  return mympd_util_hash(string, "sha256")
end

--- URL decoding
-- @param string String to URL decode
-- @param form true = decode a url form encoded string
-- @return Decoded string
function mympd.urldecode(string, form)
  return mympd_util_urldecode(string, form)
end

--- URL encoding
-- @param string String to URL encode
-- @return Encoded string
function mympd.urlencode(string)
  return mympd_util_urlencode(string)
end

-- Map http status code to status text
local status_text = {
  c100 = "Continue",
  c101 = "Switching Protocols",
  c102 = "Processing",
  c200 = "OK",
  c201 = "Created",
  c202 = "Accepted",
  c203 = "Non-authoritative Information",
  c204 = "No Content",
  c205 = "Reset Content",
  c206 = "Partial Content",
  c207 = "Multi-Status",
  c208 = "Already Reported",
  c226 = "IM Used",
  c300 = "Multiple Choices",
  c301 = "Moved Permanently",
  c302 = "Found",
  c303 = "See Other",
  c304 = "Not Modified",
  c305 = "Use Proxy",
  c307 = "Temporary Redirect",
  c308 = "Permanent Redirect",
  c400 = "Bad Request",
  c401 = "Unauthorized",
  c402 = "Payment Required",
  c403 = "Forbidden",
  c404 = "Not Found",
  c405 = "Method Not Allowed",
  c406 = "Not Acceptable",
  c407 = "Proxy Authentication Required",
  c408 = "Request Timeout",
  c409 = "Conflict",
  c410 = "Gone",
  c411 = "Length Required",
  c412 = "Precondition Failed",
  c413 = "Payload Too Large",
  c414 = "Request-URI Too Long",
  c415 = "Unsupported Media Type",
  c416 = "Requested Range Not Satisfiable",
  c417 = "Expectation Failed",
  c418 = "I'm a teapot",
  c421 = "Misdirected Request",
  c422 = "Unprocessable Entity",
  c423 = "Locked",
  c424 = "Failed Dependency",
  c426 = "Upgrade Required",
  c428 = "Precondition Required",
  c429 = "Too Many Requests",
  c431 = "Request Header Fields Too Large",
  c444 = "Connection Closed Without Response",
  c451 = "Unavailable For Legal Reasons",
  c499 = "Client Closed Request",
  c500 = "Internal Server Error",
  c501 = "Not Implemented",
  c502 = "Bad Gateway",
  c503 = "Service Unavailable",
  c504 = "Gateway Timeout",
  c505 = "HTTP Version Not Supported",
  c506 = "Variant Also Negotiates",
  c507 = "Insufficient Storage",
  c508 = "Loop Detected",
  c510 = "Not Extended",
  c511 = "Network Authentication Required",
  c599 = "Network Connect Timeout Error"
}

--- Sends a HTTP reply.
-- Can be only used in the "http" event
-- @param status HTTP status code
-- @param header Additional headers terminated by "\r\n"
-- @param body HTTP Body to send
function mympd.http_reply(status, header, body)
  return "HTTP/1.1 " .. status .. " " .. status_text["c" .. status] .. "\r\n" ..
    "Content-length: " .. #body .. "\r\n" ..
    "Connection: close\r\n" ..
    header ..
    "\r\n" ..
    body
end

--- Sends a HTTP temporary redirect (302).
-- Can be only used in the "http" event
-- @param location Location to redirect
function mympd.http_redirect(location)
  return "HTTP/1.1 302 Found\r\n" ..
    "Content-length: 0\r\n" ..
    "Location: " .. location .. "\r\n" ..
    "Connection: close\r\n" ..
    "\r\n"
end
