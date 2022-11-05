---
layout: page
permalink: /references/smart-playlists
title: Smart playlists
---

In the default configuration there are three smart playlists:

 1. Best Rated
 2. Most Played
 3. Newest Songs

Additionally there is a smart playlist generation rule that creates one smart playlist per genre.

## Playlist rule

myMPD can generate smart playlists per tag value, e.g. one playlist for each genre. You can configure the rule in the settings dialog under `General -> Smart playlists`.

## Smart playlists

myMPD creates per default three smart playlists. Smart playlists are defined in the folder `/var/lib/mympd/smartpls` (one JSON file per smart playlist). myMPD creates from this definitions normal MPD playlists on startup or database changes (you can enforce updating the playlists through the update option in the gui).

| KEY | DESCRIPTION |
| --- | ----------- |
| type | Type of smart playlist: `sticker`, `newest` or `search` |
| sticker | stickername, see [Sticker]({{ site.baseurl }}/references/sticker) |
| maxentries | Maximum entries for this playlist |
| minvalue | Minimum sticker value |
| timerange | timerange in seconds |
| expression | a valid mpd filter expression |
| sort | a valid tag name (e.g. Artist), an empty string or "shuffle" |
{: .table .table-sm }

### Sticker based

 - myMPDsmart-bestRated: `{"type": "sticker", "sticker": "like", "maxentries": 200, "minvalue": 2, "sort": ""}`
 - myMPDsmart-mostPlayed: `{"type": "sticker", "sticker": "playCount", "maxentries": 200, "minvalue": 10,"sort": ""}`

### Newest songs

 - myMPDsmart-newestSongs: `{"type": "newest", "timerange": 604800, "sort":""}`

### Saved search

 - savedSearch: `{"type":"search", "expression":"((Artist contains 'test'))", "sort":"Album"}`
