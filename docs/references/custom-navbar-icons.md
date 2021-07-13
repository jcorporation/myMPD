---
layout: page
permalink: /references/custom-navbar-icons
title: Custom navbar icons
---

The navbar icons can be customized. You must edit the file `/var/lib/mympd/state/navbar_icons` and restart myMPD. It must be a valid JSON array.

| FIELD | DESCRIPTION |
| ----- | ----------- |
| ligature | the same as for home icons from [material.io](https://material.io/resources/icons/?style=baseline) |
| title | title of the icon (would be translated) |
| options | array of the view to open (take the part before the exclamation mark and split it by the `/` character), ending hyphens can be ommited |
| badge | extra html markup inserted after the icon |
{: .table .table-sm }

Default navbar definition:
```
[
  {
    "ligature": "home",
    "title": "Home",
    "options": ["Home"],
    "badge":""
  },
  {
    "ligature": "play_arrow",
    "title": "Playback",
    "options": ["Playback"],
    "badge": ""
  },
  { 
    "ligature": "queue_music",
    "title":"Queue",
    "options": ["Queue"],
    "badge":"<span id=\"badgeQueueItems\" class=\"badge badge-secondary\"></span>"
  },
  {
    "ligature": "library_music",
    "title":"Browse",
    "options": ["Browse"],
    "badge":""
  },
  {
    "ligature": "search",
    "title": "Search",
    "options": ["Search"],
    "badge": ""
  }
]
```

### Other Example:

URI: /Browse/Filesystem!0/-/-/-/
```
  {
    "ligature": "library_music",
    "title":"Browse",
    "options": ["Browse", "Filesystem"],
    "badge":""
  },
```