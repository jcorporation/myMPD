--- Execute a system command and capture its output
-- @param cmd Command to execute
-- @return Captured output of cmd
function mympd.os_capture(cmd)
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
end
