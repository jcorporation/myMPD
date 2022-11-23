-- {"order":1,"arguments":[]}
mympd.init()
uri = "https://api.listenbrainz.org/1/submit-listens"
headers = "Content-type: application/json\r\n"..
  "Authorization: Token "..mympd_state["listenbrainz_token"].."\r\n"

rc, result = mympd.api("MYMPD_API_PLAYER_CURRENT_SONG")
if rc ~= 0 then
  return
end

for k, v in pairs(result) do
  if v == "-" or v == nil then
    result[k] = ""
  end
end
artist_mbids = {}
if result["MUSICBRAINZ_ARTISTID"] ~= nil then
  for k, v in pairs(result["MUSICBRAINZ_ARTISTID"]) do
    if v ~= "-" then
      artist_mbids[#artist_mbids + 1] = v
    end
  end
end
if result["MUSICBRAINZ_ALBUMARTISTID"] ~= nil then
  for k, v in pairs(result["MUSICBRAINZ_ALBUMARTISTID"]) do
    if v ~= "-" then
      artist_mbids[#artist_mbids + 1] = v
    end
  end
end
payload = json.encode({
  listen_type = "single",
  payload = {{
    listened_at = result["startTime"],
    track_metadata = {
      additional_info = {
        release_mbid = result["MUSICBRAINZ_RELEASETRACKID"],
        recording_mbid = result["MUSICBRAINZ_TRACKID"],
        artist_mbids = artist_mbids
      },
      artist_name = result["Artist"][1],
      track_name = result["Title"],
      release_name = result["Album"]
    }
  }}
});
rc, code, header, body = mympd.http_client("POST", uri, headers, payload)
if rc > 0 then
  return body
end
