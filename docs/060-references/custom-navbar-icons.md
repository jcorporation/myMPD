---
title: Custom navbar icons
---

The navbar icons can be customized. You must edit the file `/var/lib/mympd/state/navbar_icons` and restart myMPD. It must be a valid JSON array.

| FIELD | DESCRIPTION |
| ----- | ----------- |
| ligature | the same as for home icons from [Material Icons](https://fonts.google.com/icons?selected=Material+Icons&icon.style=Filled) |
| title | title of the icon (would be translated) |
| options | array of the view to open (take the part before the exclamation mark and split it by the `/` character), ending hyphens can be omitted |

Default navbar definition:

```json
[
  {
    "ligature": "home",
    "title": "Home",
    "options": ["Home"]
  },
  {
    "ligature": "play_arrow",
    "title": "Playback",
    "options": ["Playback"]
  },
  { 
    "ligature": "queue_music",
    "title":"Queue",
    "options": ["Queue"]
  },
  {
    "ligature": "library_music",
    "title":"Browse",
    "options": ["Browse"]
  },
  {
    "ligature": "search",
    "title": "Search",
    "options": ["Search"]
  }
]
```

### Other Example

URI: /Browse/Filesystem!0/-/-/-/

```json
  {
    "ligature": "library_music",
    "title":"Browse",
    "options": ["Browse", "Filesystem"]
  },
```
