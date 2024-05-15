---
layout: page
permalink: /scripting/usage/yt-dlp
title: Play YouTube links
---

This script extracts the audio links and the associated metadata from an YouTube link and appends the result to the MPD queue. On playback MPD calls the script again to get the real streaming uri.

The script was written by [sevmonster](https://github.com/sevmonster) - [Discussion](https://github.com/jcorporation/myMPD/discussions/1276).

## Usage

1. Install `yt-dlp`
2. Import the script `yt-dlp`
3. Start it from the main menu and copy the YouTube link in URI field.
