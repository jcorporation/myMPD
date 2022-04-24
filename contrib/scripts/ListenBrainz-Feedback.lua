-- {"order":1,"arguments":["MUSICBRAINZ_TRACKID","feedback"]}
mympd.init()
uri = "https://api.listenbrainz.org/1/feedback/recording-feedback"
headers = "Content-type: application/json\r\n"..
  "Authorization: Token "..mympd_state["listenbrainz_token"].."\r\n"

feedback = arguments["feedback"] - 1

payload = json.encode({
  recording_msid = arguments["MUSICBRAINZ_TRACKID"],
  score = feedback
});
rc, response, header, body = mympd_api_http_client("POST", uri, headers, payload)
if rc > 0 then
  return body
end
