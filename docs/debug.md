---
layout: page
permalink: /debug
title: Debug
---

Tips to debug problems with myMPD. Try these steps and reproduce the error.

## myMPD logging

### Start myMPD manually

- Stop myMPD
- Set loglevel to debug: `export MYMPD_LOGLEVEL=7`
- Start it in the console `mympd` (same user as the service runs or as root)
- Debug output is printed to the console
- Press Ctrl + C to abort
- Reset loglevel: `unset MYMPD_LOGLEVEL`

- **Note:** Use [systemd-run]({{ site.baseurl }}/running#manual-startup), if you use a distribution with systemd

### Get logs from running myMPD

You can set the log level in the Maintenance dialog to `debug` and get the output from your logging service, for systemd it is: `journalctl -fu mympd`.

## Webbrowser logging

- Open the javascript console or webconsole
- Clear the browsercache and the application cache
- Reload the webpage

## MPD logging

- Set `log_level "verbose"` in mpd.conf and restart mpd
- Look through the mpd log file for any errors

## Recording traffic between MPD and myMPD

### Local socket connection

MPD should listen on `/run/mpd/socket`. Point myMPD to `/run/mpd/socket-debug`.

```
socat -t100 -v UNIX-LISTEN:/run/mpd/socket-debug,mode=777,reuseaddr,fork UNIX-CONNECT:/run/mpd/socket
```

### TCP connection

MPD should listen on `/run/mpd/socket`. Point myMPD to `/run/mpd/socket-debug`.

```
tcpdump -nni any -vvv -x any host <mpd host>
```

## myMPD debug build

- Build: `./build.sh debug`

### If myMPD aborts with a segmentation fault

- Run: `catchsegv debug/bin/mympd`

### Memory leaks

- Run: `valgrind --leakcheck=full debug/bin/mympd`

### Other errors - get a calltrace

- Run: `valgrind --tool=callgrind debug/bin/mympd`
