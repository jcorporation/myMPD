---
layout: page
permalink: /additional-topics/scrobble-to-listenbrainz
title: Scrobble to ListenBrainz
---

Through the scripting and trigger features you can add a simple scrobbler to myMPD.

An example script is in the `contrib/scripts` directory named `ListenBrainz-Scrobbler.lua`

After copy & paste the file content in a new script (ommit the first comment line) you must add your ListenBraniz token (`token = "<musicbrainz token>"`).

To automatically call this script add trigger to the event `Scrobble` with this script as action.
