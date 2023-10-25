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

myMPD creates per default three smart playlists. Smart playlists are defined in the folder `/var/lib/mympd/smartpls` (one JSON file per smart playlist). myMPD creates from this definitions normal MPD playlists on startup or database changes (you can enforce updating the playlists in the maintenance dialog in the gui).

| KEY | SMART PLAYLIST TYPE | DESCRIPTION |
| --- | ------------------- | ----------- |
| type | all | Type of smart playlist: `sticker`, `newest` or `search` |
| sticker | sticker | stickername, see [Sticker]({{ site.baseurl }}/references/sticker) |
| maxentries | sticker | Maximum entries for the playlist |
| minvalue | sticker | Minimum sticker value |
| timerange | newest | timerange since last database update in seconds |
| expression | search | MPD filter expression |
| sort | all | tag to sort (e.g. `Artist`), an empty string or `shuffle` |
| sortdesc | all | `false` = sort ascending, `true` = sort descending |
{: .table .table-sm }

### Sticker based

- myMPDsmart-bestRated: `{"type": "sticker", "sticker": "like", "maxentries": 200, "minvalue": 2, "sort": "", "sortdesc": false}`
- myMPDsmart-mostPlayed: `{"type": "sticker", "sticker": "playCount", "maxentries": 200, "minvalue": 10,"sort": "", "sortdesc": false}`

### Newest songs

- myMPDsmart-newestSongs: `{"type": "newest", "timerange": 604800, "sort":"", "sortdesc": false}`

### Saved search

- savedSearch: `{"type":"search", "expression":"((Artist contains 'test'))", "sort":"Album", "sortdesc": false}`
