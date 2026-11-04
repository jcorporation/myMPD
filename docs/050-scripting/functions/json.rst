JSON
====

Encoding and decoding of JSON to/from Lua tables.

.. code:: lua

   -- Encode a Lua table as Json
   local payload = json.encode({
       recording_mbid = mbid,
       blurb_content = arguments["blurb_content"],
       pinned_until = arguments["pinned_until"]
   });

   -- Decode JSON as Lua table
   local string = "{\"str\":\"value\", \"number\": 1}"
   local decoded = json.decode(string)
   print(decoded.str)
   print(decoded.number)

Error handling
--------------

``json.decode`` throws an error on invalid JSON, use ``pcall`` to handle the exception.

.. code:: lua

    local is_json, ret = pcall(json.decode, body)
    if is_json == true then
        -- ret is the decoded json
    else
        -- ret is the error
    end
