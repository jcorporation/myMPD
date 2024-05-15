-- {"order":1,"arguments":["URI"]}
-- yt-dlp helper function
local yt_dlp_cache = string.format(
  "--cache-dir '%s/yt-dlp'",
  string.gsub((os.getenv("TMPDIR") or "/tmp"), "/+$", "")
)
function yt_dlp(uri, args)
    local output = mympd.os_capture(
      string.format(
        "yt-dlp %s %s '%s' 2>/dev/null",
        yt_dlp_cache, args, uri
      )
    )
    if not output then
        error("No output from yt-dlp")
    end
    return output
end

local yt_dlp_args_download = [[\
  --format bestaudio \
  --print '%(urls)s'\
]]

local yt_dlp_args_search = [[\
  --flat-playlist \
  --print '%(.{webpage_url, title,extractor,playlist_title,playlist_index})j,' \
  --extractor-args 'youtube:skip=hls,dash,translated_subs'\
]]

if scriptevent == "http" then
  -- calling from a stream play event, redirect to the real stream URI
    return mympd.http_redirect(yt_dlp(arguments.URI, yt_dlp_args_download))
else
    -- URI argument is required
    if arguments.URI == "" then
      return "No URI submitted"
    end

    -- calling from user invocation/API
    mympd.init()

    -- get videos and convert to json array
    local output = "[" ..string.gsub(yt_dlp(arguments.URI, yt_dlp_args_search), ",+$", "") .."]"

    -- check result from yt-dlp
    if string.sub(output, 1, 3) == "[NA" then
      return "No streams found"
    end

    -- generate script URIs and collect metadata for the results
    local uri_format = string.format(
      "%sscript/%s/%s?URI=%%s",
      mympd_state.mympd_uri,
      partition,
      mympd.urlencode(scriptname)
    )
    for i, x in ipairs(json.decode(output)) do
        local uri = string.format(uri_format, mympd.urlencode(x.webpage_url))
        local meta = {
          title = x.title,
          artist = x.extractor
        }
        if x.playlist_title then
            meta["album"] = x.playlist_title
            meta["track"] = x.playlist_index
        end
        -- append result to the queue and set tags
        mympd.api("MYMPD_API_QUEUE_APPEND_URI_TAGS", {uri = uri, tags = meta, play = false})
    end
end
