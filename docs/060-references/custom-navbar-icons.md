---
title: Custom navbar icons
---

The navbar icons can be customized. You must edit the file `/var/lib/mympd/state/navbar_icons` and restart myMPD. It must be a valid JSON array.

| FIELD | DESCRIPTION |
| ----- | ----------- |
| ligature | The same as for home icons from [Material Icons](https://fonts.google.com/icons?selected=Material+Icons&icon.style=Filled) |
| title | Title of the icon (will be translated) |
| options | Array of the view to open. |

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

## Available views

- Playback
- Queue
    - Current
    - LastPlayed
    - Jukebox
- Browse
    - Database
    - Filesystem
    - Playlist
    - Radio
        - Favorites
        - Webradiodb
- Search

### Some examples

```json
{
  "ligature": "library_music",
  "title":"Browse",
  "options": ["Browse", "Filesystem"]
},
{
  "ligature": "radio",
  "title":"Radio Favorites",
  "options": ["Browse", "Radio", "Favorites"]
}
```
