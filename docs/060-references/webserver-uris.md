---
title: Webserver URIs
---

Reference of all webserver uris.

| URI | DESCRIPTION |
| --- | ----------- |
| `/` | Document root `/var/lib/mympd/empty` in release, `<src tree>/htdocs` for debug |
| `/albumart/<albumid>` | Returns the albumart for simple album mode. |
| `/albumart-thumb/<albumid>` | Returns the albumart thumbnail for simple album mode |
| `/albumart?offset=<nr>&uri=<songuri>` | Returns the albumart, offset should be 0 and is only relevant to retrieve more than the first embedded image. |
| `/albumart-thumb?offset=<nr>&uri=<songuri>` | Returns the albumart thumbnail, offset should be 0 |
| `/api/<partition>` | jsonrpc api endpoint |
| `/browse/` | Prints the list of [published directories](published-directories.md) |
| `/ca.crt` | Returns the myMPD CA certificate |
| `/folderart?path=<path>` | Returns the folderart thumbnail. |
| `/playlistart?type=<plist,smartpls>&playlist=<playlist name>` | Returns the playlistart thumbnail or a redirect to the placeholder image if not found. |
| `/proxy?uri=<uri>` | Fetches the response from the uri (GET), allowed hosts: `jcorporation.github.io`, `musicbrainz.org`, `listenbrainz.org` |
| `/script/<partition>/<script>` | Executes a script (Script should return a valid http response) |
| `/script-api/<partition>` | Jsonrpc api endpoint for mympd-script |
| `/serverinfo` | Returns the ip address of myMPD |
| `/stream/<partition>` | Reverse proxy for mpd http stream |
| `/tagart?tag=<tagname>&value=<tagvalue>` | Returns the tagart thumbnail. |
| `/webradio?uri=<webradio uri>` | Webradio endpoint |
| `/ws/<partition>` | Websocket endpoint |
