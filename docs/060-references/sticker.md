---
title: Sticker
---

myMPD uses the MPD stickers to save song states, playback statistics and votes of songs.

| STICKER | DESCRIPTION |
| ------- | ----------- |
| elapsed | recent song position |
| lastPlayed | last played time of song (unix timestamp) |
| lastSkipped | last skipped time of songs (unix timestamp) |
| like | 0 - dislike, 1 - neutral, 2 - like |
| playCount | How often the song was played |
| rating | 0 - 10 stars rating |
| skipCount | How often the song was skipped |

**playCount is updated:**

- The song must be longer than 10 seconds
- And the song has been played for at least half its duration, or for 4 minutes (whichever occurs earlier)

**skipCount is updated:**

- The song has been played for at least 10 seconds

## Padding of sticker values

You can enable the padding of sticker values that should be treated as integers. Stickers are padded to 12 digits. Padding is useful, because MPD saves all sticker values as strings.

Run `mympd-config MYMPD_STICKERS_PAD_INT true` and restart myMPD.

## Central sticker database

myMPD uses a distinct mpd connection to read and write stickers. You can configure this connection to use an other mpd server to maintain song statistics across several myMPD instances.

- [MPD satellite setup](../070-additional-topics/mpd-satellite-setup.md)
