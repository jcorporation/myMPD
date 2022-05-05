-- {"order":1,"arguments":["uri","blurb_content","pinned_until"]}
mympd.init()
pin_uri = "https://api.listenbrainz.org/1/pin"
unpin_uri = "https://api.listenbrainz.org/1/pin/unpin"
headers = "Content-type: application/json\r\n"..
  "Authorization: Token "..mympd_state["listenbrainz_token"].."\r\n"
payload = ""
uri = ""

if arguments["uri"] ~= "" then
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
      uri = pin_uri
    end
  end
else
  uri = unpin_uri
end

if uri ~= "" then
  rc, response, header, body = mympd_api_http_client("POST", uri, headers, payload)
  if rc > 0 then
    return body
  end
end
