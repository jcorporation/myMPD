---
layout: page
permalink: /references/partitions
title: Partitions
---

myMPD supports multiple mpd partitions. myMPD connects to the `default` partition, gets a list of all partitions and creates separate connections for each partition.

Following items are partition specific:

- Highlight color
- Highlight contrast color
- Jukebox
- Last played
- Local player
- MPD options
- Triggers
- Timers

You can select and manage partitions in the web interface. The selected partition for the web interface is saved in the browsers localStorage.

## Notes

- The `default` partition can not be deleted.
- Do not use a partition with the name `!all!`. It is used internally by myMPD.
