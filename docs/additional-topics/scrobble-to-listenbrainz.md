---
layout: page
permalink: /additional-topics/scrobble-to-listenbrainz
title: Scrobble to ListenBrainz
---

Through the scripting and trigger features you can add a simple scrobbler to myMPD.

An example script is in the `docs/scripting/scripts` directory named `ListenBrainz-Scrobbler.lua`

- Add your ListenBrainz token (Settings -> Cloud).
- Create a new script
  - Import the `ListenBrainz-Scrobbler.lua` script
- Create a new trigger
  - Event: `mympd_scrobble`
  - Action: above script

You should enable the MusicBrainz related [tags]({{site.baseurl}}/references/tags) in mpd and myMPD to scrobble MBIDs.
