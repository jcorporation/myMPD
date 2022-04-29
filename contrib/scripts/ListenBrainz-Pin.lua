-- {"order":1,"arguments":["MUSICBRAINZ_TRACKID","blurb_content","pinned_until"]}
mympd.init()
uri = "https://api.listenbrainz.org/1/pin"
headers = "Content-type: application/json\r\n"..
  "Authorization: Token "..mympd_state["listenbrainz_token"].."\r\n"

payload = json.encode({
  recording_mbid = arguments["MUSICBRAINZ_TRACKID"],
  blurb_content = arguments["blurb_content"],
  pinned_until = arguments["pinned_until"]
});
rc, response, header, body = mympd_api_http_client("POST", uri, headers, payload)
if rc > 0 then
  return body
end
