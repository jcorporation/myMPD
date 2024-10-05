---
--- myMPD string functions
---

--- Trims a string
-- @param str String to trim
-- @return Trimed string
function mympd.trim(str)
  return (string.gsub(str, "^%s*(.-)%s*$", "%1"))
end

--- Split string by newline characters and trims the lines
-- @param str String to split
-- @return tables of lines
function mympd.splitlines(str)
  local lines = {}
  for line in string.gmatch(str, "[^\n]+") do
    line = mympd.trim(line)
    table.insert(lines, line)
  end
  return lines
end
