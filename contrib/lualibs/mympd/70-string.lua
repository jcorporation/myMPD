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

--- Checks for empty string and nil
-- @param str String to check
-- @return true if string is empty or nil, else false
function mympd.isnilorempty(str)
  return str == nil or str == ''
end

--- Checks a Lua table of tags against a comma separated list
-- @param list_str Comma separated list values
-- @param tbl Lua table of values to check against the list
-- @return true if
function mympd.tblvalue_in_list(list_str, tbl)
  if mympd.isnilorempty(list_str) or
     mympd.isnilorempty(tbl)
  then
    return false
  end
  local list = {}
  for v in string.gmatch(list_str, '([^,]+)') do
    list[string.lower(mympd.trim(v))] = true
  end
  for _, v in pairs(tbl) do
    if list[string.lower(v)] == true then
      mympd.log(7, "List matched")
      return true
    end
  end
  return false
end
