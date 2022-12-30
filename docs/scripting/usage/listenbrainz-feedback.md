---
layout: page
permalink: /scripting/usage/listenbrainz-feedback
title: ListenBrainz Feedback
---

Through the scripting and trigger features you can send hate/love to ListenBrainz.

An example script is in the `docs/scripting/scripts` directory named `ListenBrainz-Feedback.lua`

- Add your ListenBrainz token (Settings -> Cloud)
- Create a new script
  - Import the `ListenBrainz-Feedback.lua` script
- Create a new trigger
  - Event: `mympd_feedback`
  - Action: above script
  - Leave the arguments empty

You must enable the MusicBrainz tag `MUSICBRAINZ_TRACKID` in mpd and myMPD.
