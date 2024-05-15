-- {"order":1,"arguments":["uri"]}
local cache = "--cache-dir '" ..string.gsub((os.getenv("TMPDIR") or "/tmp"), "/+$", "").. "/yt-dlp'"
function ytdlp(uri, args)
  local output = mympd.os_capture("sh -c \"yt-dlp " ..cache.. " " ..args.. " '" ..uri.. "' 2>/dev/null\"")
  if not output then
    error("No output from yt-dlp")
  end
  return output
end
if scriptevent == "http" then
  -- calling from a stream play event
  local output = ytdlp(arguments.uri, "--format bestaudio --print '%(urls)s' --extractor-args 'youtube:skip=translated_subs'")
  -- redirect to the real stream URI
  return mympd.http_redirect(output)
else
  -- calling from user invocation/API
  mympd.init()
  local output = ytdlp(arguments.uri, "--flat-playlist --print '%(.{title,webpage_url,extractor,playlist_title,playlist_index})j' --extractor-args 'youtube:skip=hls,dash,translated_subs'")
  local uris = {}
  local x =json.decode(output)
  uris[1] = mympd_state.mympd_uri .. "script/" .. partition .. "/" .. mympd.urlencode(scriptname) ..
      "?title=" .. mympd.urlencode(x.title) ..
      "&artist=" .. mympd.urlencode(x.extractor) ..
      "&uri=" .. mympd.urlencode(x.webpage_url)
  if x.playlist_title then
      uris[1] = uris[1] ..
      "&album=" .. mympd.urlencode(x.playlist_title) ..
      "&track=" .. mympd.urlencode(x.playlist_index)
  end
  mympd.api("MYMPD_API_QUEUE_APPEND_URIS", {uris = uris, play = false})
end
