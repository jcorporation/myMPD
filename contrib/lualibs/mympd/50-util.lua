---
--- myMPD utility functions
---

--
-- Notifications
--
function mympd.notify_client(severity, message)
  return mympd_util_notify(partition, requestid, severity, message)
end

function mympd.notify_partition(severity, message)
  return mympd_util_notify(partition, 0, severity, message)
end

--
-- Logging
--
function mympd.log(loglevel, message)
  return mympd_util_log(partition .. " - " .. scriptname, loglevel, message)
end

--
-- Return SHA1 hash of string
--
function mympd.hash_sha1(string)
  return mympd_util_hash(string, "sha1")
end

--
-- Return SHA256 hash of string
--
function mympd.hash_sha256(string)
  return mympd_util_hash(string, "sha256")
end

--
-- URL decoding
--
function mympd.urldecode(string, form)
  return mympd_api_util_urldecode(string, form)
end

--
-- URL encoding
--
function mympd.urlencode(string)
  return mympd_util_urlencode(string)
end

--
-- Map http status code to status text
--
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

--
-- HTTP reply
--
function mympd.http_reply(status, header, body)
  return "HTTP/1.1 " .. status .. " " .. status_text["c" .. status] .. "\r\n" ..
    "Content-length: " .. #body .. "\r\n" ..
    "Connection: close\r\n" ..
    header ..
    "\r\n" ..
    body
end

--
-- HTTP redirect
--
function mympd.http_redirect(location)
  return "HTTP/1.1 302 Found\r\n" ..
    "Content-length: 0\r\n" ..
    "Location: " .. location .. "\r\n" ..
    "Connection: close\r\n" ..
    "\r\n"
end
