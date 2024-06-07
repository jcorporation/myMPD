--- Simple HTTP client
-- @param method HTTP method, only GET or POST is supported
-- @param uri The uri to access
-- @param headers Additional headers terminated by "\r\n"
-- @param payload Payload to send (POST only)
-- @return rc 0 for success, else 1
-- @return HTTP status code
-- @return HTTP Headers as Lua table
-- @return HTTP Body
function mympd.http_client(method, uri, headers, payload)
  local rc, code, header, body = mympd_http_client(method, uri, headers, payload)
  return rc, code, header, body
end

--- Download a file over http
-- @param uri The uri to access
-- @param out Filename to write the response body
-- @return 0 for success, else 1
function mympd.http_download(uri, out)
  return mympd_http_download(uri, out)
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
-- @return HTTP response
function mympd.http_reply(status, header, body)
  local reply = "HTTP/1.1 " .. status .. " " .. status_text["c" .. status] .. "\r\n" ..
    "Content-length: " .. #body .. "\r\n" ..
    "Connection: close\r\n" ..
    header ..
    "\r\n" ..
    body
  return reply
end

--- Sends a HTTP temporary redirect (302).
-- Can be only used in the "http" event
-- @param location Location to redirect
-- @return HTTP response
function mympd.http_redirect(location)
  local reply = "HTTP/1.1 302 Found\r\n" ..
    "Content-length: 0\r\n" ..
    "Location: " .. location .. "\r\n" ..
    "Connection: close\r\n" ..
    "\r\n"
  return reply
end

--- Serves a file from the filesystem.
-- Only files from the diskcache are allowed.
-- @param file file to serve
-- @return HTTP response
function mympd.http_serve_file(file)
  return mympd_http_serve_file(file)
end

--- Sends a JSONRPC 2.0 response.
-- @param result jsonrpc result object
-- @return HTTP response
function mympd.http_jsonrpc_response(obj)
  local response = json.encode({
    jsonrpc = "2.0",
    id = mympd_env.requestid,
    result = obj
  })
  return mympd.http_reply(200, "Content-Type: application/json\r\n", response)
end

--- Sends a JSONRPC 2.0 error.
-- @param method API method
-- @param msg Error message
-- @return HTTP response
function mympd.http_jsonrpc_error(method, msg)
  local response = json.encode({
    jsonrpc = "2.0",
    id = mympd_env.requestid,
    error = {
      method = method,
      facility = "script",
      severity = "error",
      message = msg,
      data = {}
    }
  })
  return mympd.http_reply(200, "Content-Type: application/json\r\n", response)
end

--- Sends a JSONRPC 2.0 warning.
-- @param method API method
-- @param msg Error message
-- @return HTTP response
function mympd.http_jsonrpc_warn(method, msg)
  local response = json.encode({
    jsonrpc = "2.0",
    id = mympd_env.requestid,
    error = {
      method = method,
      facility = "script",
      severity = "warn",
      message = msg,
      data = {}
    }
  })
  return mympd.http_reply(200, "Content-Type: application/json\r\n", response)
end
