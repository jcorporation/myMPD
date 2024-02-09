--
-- Simple HTTP client
--
function mympd.http_client(method, uri, headers, payload)
  rc, code, header, body = mympd_api_http_client(method, uri, headers, payload)
  return rc, code, header, body
end
