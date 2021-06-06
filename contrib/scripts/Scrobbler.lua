token = ""
uri = "https://api.listenbrainz.org/1/submit-listens"
headers = "Content-type: application/json\r\n"..
  "Authorization: Token "..token.."\r\n"

rc, raw_result = mympd_api("MPD_API_PLAYER_CURRENT_SONG")
if rc == 0 then
  current_song = json.decode(raw_result)
  payload = json.encode({
    listen_type: "single",
    payload: [{
      listened_at: current_song["result"]["startTime"],
      track_metadata: {
        artist_name: current_song["result"]["Artist"],
        track_name: current_song["result"]["Title"]
      }
    }]
  });
  rc, response, header, body = mympd_api_http_client("POST", uri, headers, payload)
  if rc > 0 then
    return body
  end
end
