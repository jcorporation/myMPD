-- {"order":1,"arguments":["uri","vote"]}
mympd.init()
uri = "https://api.listenbrainz.org/1/feedback/recording-feedback"
headers = "Content-type: application/json\r\n"..
  "Authorization: Token "..mympd_state["listenbrainz_token"].."\r\n"

vote = arguments["vote"] - 1
rc, song = mympd.api("MYMPD_API_SONG_DETAILS", {uri = arguments["uri"]})
if rc == 0 then
  mbid = song["MUSICBRAINZ_TRACKID"]
  if mbid ~= nil then
    payload = json.encode({
      recording_mbid = mbid,
      score = vote
    });
    rc, code, header, body = mympd.http_client("POST", uri, headers, payload)
    if rc > 0 then
      return body
    end
  end
end
