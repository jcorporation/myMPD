-- {"order":1,"arguments":[]}
mympd.init()
uri = "https://api.listenbrainz.org/1/pin/unpin"
headers = "Content-type: application/json\r\n"..
  "Authorization: Token "..mympd_state["listenbrainz_token"].."\r\n"

payload = ""
rc, response, header, body = mympd_api_http_client("POST", uri, headers, payload)
if rc > 0 then
  return body
end
