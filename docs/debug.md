---
layout: page
permalink: /debug
title: Debug
---

Tips to debug problems with myMPD. Try these steps and reproduce the error.

## myMPD logging

- Stop myMPD
- Set loglevel to debug: `export MYMPD_LOGLEVEL=7`
- Start it in the console `mympd`
- Debug output is printed to the console
- Press Ctrl + C to abort
- Reset loglevel: `unset MYMPD_LOGLEVEL`

## Webbrowser logging

- Open the javascript console or webconsole
- Clear the browsercache and the application cache
- Reload the webpage

## MPD logging

- Set `log_level "verbose"` in mpd.conf and restart mpd
- Look through the mpd log file for any errors

## myMPD debug build

- Build: `./build.sh debug`

### If myMPD aborts with a segmentation fault

- Run: `catchsegv debug/mympd`

### Other errors - get a calltrace

- Run: `valgrind --tool=callgrind debug/mympd`
