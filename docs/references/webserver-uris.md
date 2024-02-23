---
layout: page
permalink: /references/webserver-uris
title: Webserver uris
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
| `/script-api/<partition>` | jsonrpc api endpoint for mympd-script |
| `/serverinfo` | Returns the ip address of myMPD |
| `/browse/` | Prints the list of [published directories]({{ site.baseurl }}/references/published-directories) |
| `/ca.crt` | Returns the myMPD CA certificate |
| `/folderart?uri=<path>` | Returns the folderart thumbnail. |
| `/playlistart?playlist=<playlist name>` | Returns the playlistart thumbnail. |
| `/proxy?uri=<uri>` | Fetches the response from the uri (GET), allowed hosts: `jcorporation.github.io`, `musicbrainz.org`, `listenbrainz.org` |
| `/stream/<partition>` | Reverse proxy for mpd http stream |
| `/tagart?tag=<tagname>&value=<tagvalue>` | Returns the tagart thumbnail. |
| `/ws/<partition>` | Websocket endpoint |
{: .table .table-sm }
