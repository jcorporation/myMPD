-- {"order":1,"arguments":["uri","blurb_content","pinned_until"]}
mympd.init()
uri = "https://api.listenbrainz.org/1/pin"
headers = "Content-type: application/json\r\n"..
  "Authorization: Token "..mympd_state["listenbrainz_token"].."\r\n"

rc, raw_song = mympd_api("MYMPD_API_DATABASE_SONGDETAILS", "uri", arguments["uri"])
if rc == 0 then
  song = json.decode(raw_song)
  mbid = song["result"]["MUSICBRAINZ_TRACKID"]
  if mbid ~= nil then
    payload = json.encode({
      recording_mbid = mbid,
      blurb_content = arguments["blurb_content"],
      pinned_until = arguments["pinned_until"]
    });
    rc, response, header, body = mympd_api_http_client("POST", uri, headers, payload)
    if rc > 0 then
      return body
    end
  end
end
