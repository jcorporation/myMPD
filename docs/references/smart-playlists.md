---
layout: page
permalink: /references/smart-playlists
title: Smart playlists
---

In the default configuration there are no smart playlists. You can add smart playlists in the playlist view or through a saved song search.

## Playlist rule

myMPD can generate smart playlists per tag value, e.g. one playlist for each genre. You can configure the rule in the settings dialog under `General -> Smart playlists`.

## Smart playlists

Smart playlists are saved in the folder `/var/lib/mympd/smartpls` (one JSON file per smart playlist). myMPD creates from this definitions normal MPD playlists on startup, after database changes and a specified interval. You can also enforce an update of all smart playlists in the maintenance dialog in the gui.

| KEY | TYPE | DESCRIPTION |
| --- | ---- | ----------- |
| type | all | Type of smart playlist: `sticker`, `newest` or `search` |
| sticker | sticker | Stickername, see [Sticker]({{ site.baseurl }}/references/sticker) |
| value | sticker | Sticker value |
| op | sticker | Sticker compare operator: `=`, `<`, `>`, `gt` (MPD 0.24), `lt` (MPD 0.24) |
| timerange | newest | Timerange since last database update in seconds |
| expression | search | MPD filter expression |
| sort | newest, expression | Tag to sort (e.g. `Artist`), `shuffle` or empty string |
| sort | sticker | Tag to sort `uri`, `value`, `value_int` (MPD 0.24) |
| sortdesc | all | `false` = sort ascending, `true` = sort descending |
| maxentries | all | Maximum entries for the playlist |
{: .table .table-sm }

### Sticker based

```
{"type": "sticker", "sticker": "like", "value": "2", "op": "=" "sort": "", "sortdesc": false, "maxentries": 200}
```

### Newest songs

```
{"type": "newest", "timerange": 604800, "sort":"", "sortdesc": false, "maxentries": 0}
```

### Saved search

```
{"type":"search", "expression":"((Artist contains 'test'))", "sort":"Album", "sortdesc": false, "maxentries": 0}
```
