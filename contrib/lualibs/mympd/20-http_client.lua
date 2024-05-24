--
-- Simple HTTP client
--
function mympd.http_client(method, uri, headers, payload)
  local rc, code, header, body = mympd_http_client(method, uri, headers, payload)
  return rc, code, header, body
end

--
-- Download a file over http
--
function mympd.http_download(uri, out)
  return mympd_http_download(uri, out)
end
