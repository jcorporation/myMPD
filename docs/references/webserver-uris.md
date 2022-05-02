---
layout: page
permalink: /references/webserver-uris
title: Webserver uris
---

Reference of all webserver uris.

| URI | DESCRIPTION |
| --- | ----------- |
| `/` | Document root `/var/lib/mympd/empty` in release, `<srctree>/htdocs` for debug |
| `/albumart?offset=<nr>&uri=<songuri>` | Returns the albumart, offset should be 0 and is only relevant to retrieve more than the first embedded image.  |
| `/albumart-thumb?offset=<nr>&uri=<songuri>` | Returns the albumart thumbnail, offset should be 0 |
| `/api/` | jsonrpc api endpoint |
| `/api/scripts` | jsonrpc api endpoint for mympd-script |
| `/api/serverinfo` | Returns the ip address of myMPD |
| `/browse/` | Prints the list of [published directories]({{ site.baseurl }}/references/published-directories) |
| `/ca.crt` | Returns the myMPD CA certificate |
| `/stream/` | Reverse proxy for mpd http stream |
| `/tagart?uri=<tagname>/<tagvalue>` | Returns the tagart |
| `/ws/` | Websocket endpoint |
{: .table .table-sm }
