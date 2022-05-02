---
layout: page
permalink: /additional-topics/scrobble-to-listenbrainz
title: Scrobble to ListenBrainz
---

Through the scripting and trigger features you can add a simple scrobbler to myMPD.

An example script is in the `contrib/scripts` directory named `ListenBrainz-Scrobbler.lua`

- Add your ListenBrainz token (Settings -> Cloud).
- Copy & paste the file content in a new script (ommit the first comment line)
- Create a new trigger
  - Event: `mympd_scrobble`
  - Action: above script

You should enable the MusicBrainz related tags in mpd and myMPD to scrobble MBIDs.
