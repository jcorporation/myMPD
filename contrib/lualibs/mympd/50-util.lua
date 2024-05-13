---
--- myMPD utility functions
---

--
-- Return SHA1 hash of string
--
function mympd.hash_sha1(string)
  return mympd_api_util_hash(string, "sha1")
end

--
-- Return SHA256 hash of string
--
function mympd.hash_sha256(string)
  return mympd_api_util_hash(string, "sha256")
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
function mympd.urlencode(string, form)
  return mympd_api_util_urlencode(string)
end

--
-- HTTP reply
--
function mympd.http_reply(status, status_text, header, body)
  return "HTTP/1.1 " .. status .. " " .. status_text .. "\r\n" ..
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
