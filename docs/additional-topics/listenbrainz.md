---
layout: page
permalink: /additional-topics/listenbrainz
title: ListenBrainz integration
---

myMPD proudly supports MusicBrainz and ListenBrainz. MusicBrainz tags are automatically linked to the MusicBrainz website and you can use scripts for scrobbling.

- Set your ListenBrainz token: Settings -> Cloud
- Enable tags in MPD and myMPD:
  - MUSICBRAINZ_ALBUMARTISTID
  - MUSICBRAINZ_ARTISTID
  - MUSICBRAINZ_RELEASETRACKID
  - MUSICBRAINZ_TRACKID

## Scrobbling

You can send your listening habits to ListenBrainz.

- Create a new script
  - Import the `ListenBrainz-Scrobbler.lua` script
- Create a new trigger
  - Event: `mympd_scrobble`
  - Action: above script

## Feedback

You can send hate/love feedback to ListenBrainz with the thumbs up and down buttons in the playback view.

- Create a new script
  - Import the `ListenBrainz-Feedback.lua` script
- Create a new trigger
  - Event: `mympd_feedback`
  - Action: above script
  - Leave the arguments empty
