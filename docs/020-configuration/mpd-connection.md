---

title: MPD connection
---

myMPD uses libmpdclient to connect to MPD. The fastet and easiest setup is to run myMPD on the same host as MPD, but myMPD can also connect to a remote MPD server.

The mpd connection settings can be configured in the web-ui (Main menu -> MPD connection).

| SETTING | DEFAULT |
| ------- | ------- |
| MPD host | `/run/mpd/socket` |
| MPD port | 6600 |
| MPD timeout | 30 |


### MPD autodetection

myMPD tries to autodetect the mpd connection only at first start (if no mpd_host file is found in the `state` directory).

1. Uses the default MPD environment variables
2. Searches for a mpd socket
  - `$XDG_RUNTIME_DIR/mpd/socket`
  - `/run/mpd/socket`
  - `/var/run/mpd/socket`
  - `/var/lib/mpd/socket`
3. Falls back to `localhost:6600`

| ENVIRONMENT | DESCRIPTION |
| ----------- | ----------- |
| MPD_HOST | MPD host or path to mpd socket |
| MPD_PORT | MPD port |
| MPD_TIMEOUT | MPD timeout in seconds |


This is done after dropping privileges to the mympd user.
