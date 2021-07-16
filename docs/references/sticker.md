---
layout: page
permalink: /references/sticker
title: Sticker
---

myMPD uses stickers to save statistics and votes of songs.

| STICKER | DESCRIPTION |
| ------- | ----------- |
| playCount | How often the song was played (1) |
| skipCount | How often the song was skipped |
| like | 0 - dislike, 1 - neutral, 2 - like |
| lastPlayed | last played time of song (unix timestamp) |
| lastSkipped | last skipped time of songs (unix timestamp) |
{: .table .table-sm }

**playCount is updated:**

- The song must be longer than 10 seconds
- And the song has been played for at least half its duration, or for 4 minutes (whichever occurs earlier.)

**skipCount is updated:**

- The song has been played for at least 10 seconds
