-- {"order":1,"arguments":["uri","vote","type"]}
mympd.init()
uri = "https://api.listenbrainz.org/1/feedback/recording-feedback"
headers = "Content-type: application/json\r\n"..
  "Authorization: Token "..mympd_state["listenbrainz_token"].."\r\n"

if arguments["type"] == "like" then
  -- thumbs up/down
  vote = arguments["vote"] - 1
else
  -- stars rating
  if arguments["vote"] > 5 then
    -- treat more than 5 stars as like
    vote = 1
  else
    -- do not send feedback to ListenBrainz
    return
  end
end

-- get song details
rc, song = mympd.api("MYMPD_API_SONG_DETAILS", { uri = arguments["uri"] })
if rc == 0 then
  mbid = song["MUSICBRAINZ_TRACKID"]
  if mbid ~= nil and mbid ~= "" then
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
