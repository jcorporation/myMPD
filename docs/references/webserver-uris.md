---
layout: page
permalink: /references/webserver-uris
title: Webserver uris
---

Reference of all webserver uris.

| URI | DESCRIPTION |
| --- | ----------- |
| `/` | Document root `/var/lib/mympd/empty` in release, `<srctree>/htdocs` for debug |
| `/albumart/<songuri>` | Returns the albumart |
| `/api/` | jsonrpc api endpoint |
| `/api/scripts` | jsonrpc api endpoint for mympd-script |
| `/api/serverinfo` | Returns the ip address of myMPD |
| `/browse/` | Prints the list of [published directories]({{ site.baseurl }}/references/published-directories) |
| `/ca.crt` | Returns the myMPD CA certificate |
| `/stream/` | Reverse proxy for mpd http stream |
| `/tagart/<tagname>/<tagvalue>` | Returns the tagart |
| `/ws/` | Websocket endpoint |
{: .table .table-sm }
