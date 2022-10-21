-- {"order":1,"arguments":[]}
mympd.init()
uri = "https://api.listenbrainz.org/1/submit-listens"
headers = "Content-type: application/json\r\n"..
  "Authorization: Token "..mympd_state["listenbrainz_token"].."\r\n"

rc, raw_result = mympd_api("MYMPD_API_PLAYER_CURRENT_SONG")
if rc == 0 then
  current_song = json.decode(raw_result)
  for k, v in pairs(current_song["result"]) do
    if v == "-" or v == nil then
      current_song["result"][k] = ""
    end
  end
  artist_mbids = {}
  if current_song["result"]["MUSICBRAINZ_ARTISTID"] ~= nil then
    for k, v in pairs(current_song["result"]["MUSICBRAINZ_ARTISTID"]) do
      artist_mbids[#artist_mbids + 1] = v
    end
  end
  if current_song["result"]["MUSICBRAINZ_ALBUMARTISTID"] ~= nil then
    for k, v in pairs(current_song["result"]["MUSICBRAINZ_ALBUMARTISTID"]) do
      artist_mbids[#artist_mbids + 1] = v
    end
  end
  payload = json.encode({
    listen_type = "single",
    payload = {{
      listened_at = current_song["result"]["startTime"],
      track_metadata = {
        additional_info = {
          release_mbid = current_song["result"]["MUSICBRAINZ_RELEASETRACKID"],
          recording_mbid = current_song["result"]["MUSICBRAINZ_TRACKID"],
          artist_mbids = artist_mbids
        },
        artist_name = current_song["result"]["Artist"][1],
        track_name = current_song["result"]["Title"],
        release_name = current_song["result"]["Album"]
      }
    }}
  });
  rc, code, header, body = mympd_api_http_client("POST", uri, headers, payload)
  if rc > 0 then
    return body
  end
end
