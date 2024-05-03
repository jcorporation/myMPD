---
--- myMPD utility functions
---

--
-- Return MD5 hash of string
--
function mympd.hash_md5(string)
  return mympd_api_util_hash(string, "md5")
end

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
