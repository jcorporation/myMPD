---
layout: page
permalink: /references/feature-matrix
title: Feature matrix
---

Certain myMPD features are only available with new MPD versions. To use all myMPD functions you should use the latest stable MPD version.

myMPD support only MPD 0.21.0 and above.

| MYMPD FEATURE | MPD FEATURE | MPD VERSION | DESCRIPTION |
| ------------- | ----------- | ----------- | ----------- |
| Advanced queue search |  | 0.24.0 | sort/window arg and prio filter for playlistsearch |
| Add after current song | position/whence arg for load/searchadd | 0.23.5 | |
| Insert into playlist | position arg for playlistadd | 0.23.5 | |
| Remove range in playlist | range arg for playlistdelete | 0.23.3 | |
| Partitions | Partition | 0.22.0 | |
| Embedded albumart| Readpicture | 0.22.0 | myMPD can read pictures tags directly if it has access to the musicdirectory |
{: .table .table-sm }
