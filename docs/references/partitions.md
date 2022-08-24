---
layout: page
permalink: /references/partitions
title: Feature matrix
---

myMPD supports multiple mpd partitions. myMPD connects to the `default` partition, gets a list of all partitions and creates separate connections for each partition.

You can select and manage partitions in the web interface. The selected partition for the web interface is saved in the browsers localStorage.

- The `default` partition can not be deleted.
- Do not use a partition with the name `!all!`. It is used internally by myMPD.
