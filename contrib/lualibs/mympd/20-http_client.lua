--- Simple HTTP client
-- @param method HTTP method, only GET or POST is supported
-- @param uri The uri to access
-- @param headers Additional headers terminated by "\r\n"
-- @param payload Payload to send (POST only)
-- @return rc 0 for success, else 1
-- @return HTTP status code
-- @return HTTP Headers
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
