--
-- mympd.lua
--
--  SPDX-License-Identifier: GPL-2.0-or-later
-- myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
-- https://github.com/jcorporation/mympd
--

mympd = { _version = "0.2.0" }

--
-- Function to retriebe mympd_state
-- 
function mympd.init()
    return mympd_api("MPD_API_SCRIPT_INIT")
end

--
-- Function to retrieve console output
-- 
function mympd.os_capture(cmd, raw)
    if io then
    	local handle = assert(io.popen(cmd, 'r'))
    	local output = assert(handle:read('*a'))
    
    	handle:close()
    
    	if raw then 
            return output 
    	end
   
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

return mympd
