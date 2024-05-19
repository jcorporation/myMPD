-- {"order":1,"arguments":["uri","blurb_content","pinned_until"]}
mympd.init()
if mympd_state.var_listenbrainz_token == nil then
  return "No ListenBrainz token set"
end

pin_uri = "https://api.listenbrainz.org/1/pin"
unpin_uri = "https://api.listenbrainz.org/1/pin/unpin"
headers = "Content-type: application/json\r\n"..
  "Authorization: Token "..mympd_state["var_listenbrainz_token"].."\r\n"
payload = ""
uri = ""

if mympd_arguments.uri ~= "" then
  rc, song = mympd.api("MYMPD_API_SONG_DETAILS", {uri = mympd_arguments.uri})
  if rc == 0 then
    mbid = song["MUSICBRAINZ_TRACKID"]
    if mbid ~= nil then
      payload = json.encode({
        recording_mbid = mbid,
        blurb_content = mympd_arguments.blurb_content,
        pinned_until = mympd_arguments.pinned_until
      });
      uri = pin_uri
    end
  end
else
  uri = unpin_uri
end

if uri ~= "" then
  rc, code, header, body = mympd.http_client("POST", uri, headers, payload)
  if rc > 0 then
    return body
  end
end
