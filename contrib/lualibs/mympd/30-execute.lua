--
-- Execute a system command and capture its output
--
function mympd.os_capture(cmd)
  if io then
    local handle = assert(io.popen(cmd, 'r'))
    local output = assert(handle:read('*a'))
    handle:close()

    output = string.gsub(
      string.gsub(
        string.gsub(output, '^%s+', ''),
          '%s+$',
          ''
      ),
      '[\n\r]+',
      ' '
    )
    return output
  else
    return "io library must be loaded"
  end
end
