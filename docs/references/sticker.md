---
layout: page
permalink: /references/sticker
title: Sticker
---

myMPD uses stickers to save statistics and votes of songs.

| STICKER | DESCRIPTION |
| ------- | ----------- |
| elapsed | recent song position |
| lastPlayed | last played time of song (unix timestamp) |
| lastSkipped | last skipped time of songs (unix timestamp) |
| like | 0 - dislike, 1 - neutral, 2 - like |
| playCount | How often the song was played |
| skipCount | How often the song was skipped |
{: .table .table-sm }

**playCount is updated:**

- The song must be longer than 10 seconds
- And the song has been played for at least half its duration, or for 4 minutes (whichever occurs earlier.)

**skipCount is updated:**

- The song has been played for at least 10 seconds
