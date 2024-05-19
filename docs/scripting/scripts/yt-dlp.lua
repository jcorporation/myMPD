-- {"order":1,"arguments":["uri"]}
-- yt-dlp helper functions
local yt_dlp_path = "yt-dlp"
local function yt_dlp_call(uri, args, parse_json)
    local cmd = string.format("%s %s '%s' 2>/dev/null", yt_dlp_path, args, uri)
    mympd.log(6, "running command: " ..cmd)
    local output = mympd.os_capture(cmd)

    -- return if output is nil/the empty string, or we don't have to parse json
    if not parse_json or not output or output == "" then
        return output
    end

    -- check result from yt-dlp for malformed format or bad data
    if string.sub(output, 1, 2) == "NA" then
        mympd.log(3, "bad format or no metadata: " ..output)
        local err_msg = "yt-dlp failed to parse --format string, or returned no usable metadata!"
        mympd.notify_client(2, err_msg)
        error(err_msg)
    end

    -- remove any trailing commas, pack into json array, and parse
    return json.decode("[" ..string.gsub(output, ",+$", "").. "]")
end

local yt_dlp_cache = string.format(
  "--cache-dir '%s/yt-dlp' \n",
  string.gsub((os.getenv("TMPDIR") or "/tmp"), "/+$", "")
)
local function yt_dlp_build_param(s)
    -- XXX: only return first result from gsub to not set yt_dlp_call parse_json
    local str = string.gsub(yt_dlp_cache .. s, "[\r\n]+%s*", "")
    return str
end

-- uri argument is required
if mympd_arguments.uri == "" then
    return "No URI provided"
end

if mympd_env.scriptevent == "http" then
    -- calling from a stream play event: redirect to the real stream URI
    local uri = yt_dlp_call(mympd_arguments.uri, yt_dlp_build_param[[
      --format bestaudio 
      --print '%(urls)s'
    ]])
    if not uri or uri == "" then
        error("yt-dlp did not return a URI for this track: " ..mympd_arguments.uri)
    end
    return mympd.http_redirect(uri)
else
    -- calling from user invocation/API
    mympd.notify_client(0, "Starting yt-dlp")
    mympd.init()

    -- look up the uri
    local results = yt_dlp_call(mympd_arguments.uri, yt_dlp_build_param[[
      --flat-playlist 
      --print '%(.{
        availability,
        webpage_url,
        fulltitle,title,episode,
        artist,album_artist,composer,creator,channel,uploader,
        album,playlist_title,series,season,
        disc_number,season_number,
        track_number,playlist_index,episode_number,playlist_count,
        genre,release_date,description,extractor})j,' 
      --extractor-args 'youtube:skip=hls,dash,translated_subs'
    ]], true)
    if #results < 1 then
        return "No streams found"
    end

    -- generate script URIs and process metadata to create the streams
    local uri_format = string.format(
      "%sscript/%s/%s?uri=%%s",
      mympd_state.mympd_uri,
      partition,
      mympd.urlencode(mympd_env.scriptname)
    )
    for i, x in ipairs(results) do
        local uri = string.format(uri_format, mympd.urlencode(x.webpage_url))

        -- special processing for some values
        local title = x.fulltitle or x.title or x.episode or uri
        if x.availability and x.availability ~= "public" then
            -- notify if stream is not public, to signal it will probably not play
            title = "[" ..x.availability.. "] " ..title
        end

        local album = x.album or x.playlist_title
        if not album then
            if x.series and x.season then
                album = x.series.. " / " ..x.season
            else
                album = x.series or x.season or x.extractor
            end
        end

        local track = x.track_number or x.playlist_index or x.episode_number
        if track then
            track = tostring(track)
        end
        if x.playlist_count then
            track = track.. "/" ..tostring(x.playlist_count)
        end

        local disc = x.disc_number or x.season_number
        if disc then
            disc = tostring(disc)
        end

        local description = ""
        if x.description then
            -- replace illegal characters from the tag value with a space
            description = string.gsub(x.description, "[\r\n\t]+", " ")
        end
        if #description > 3000 then
            description = string.sub(description, 1, 3000 - 3).. "..."
        end

        -- build metadata table
        local meta = {
          name    = "[" ..mympd_env.scriptname.. "] " ..x.extractor.. ": " ..mympd_arguments.uri,
          title   = title,
          artist  = x.artist or x.album_artist or x.composer or
                    x.creator or x.channel or x.uploader,
          album   = album,
          disc    = disc,
          track   = track,
          genre   = x.genre,
          date    = x.release_date,
          comment = comment
        }

        -- append result to the queue and set tags
        -- NOTE: change to MYMPD_API_QUEUE_INSERT_URI_TAGS
        --       or MYMPD_API_QUEUE_REPLACE_URI_TAGS if you prefer
        mympd.api("MYMPD_API_QUEUE_APPEND_URI_TAGS", {uri = uri, tags = meta, play = false})
    end
end
