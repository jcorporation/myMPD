---

title: Multiple myMPD instances on same host
---

You can run multiple myMPD instances on the same host connecting to different MPD instances.

You must define for each myMPD instance:
- different working directory
- different cache directory
- different ports to listen on
- different mpd servers

If the working directory is not present, you can create it:

First myMPD instance:
- connect to mpd listening on socket `/run/mpd1/socket`
- myMPD will listen on port 8001
- myMPD will not use ssl

```
export MPD_HOST=/run/mpd1/socket
export MYMPD_HTTP_PORT=8001
export MYMPD_SSL=false
mympd -w /var/lib/mympd1 -a /var/cache/mympd1
```

Second myMPD instance:
- connect to mpd listening on socket `/run/mpd2/socket`
- myMPD will listen on port 8002 for http
- myMPD will listen on port 4432 for ssl traffic

```
export MPD_HOST=/run/mpd2/socket
export MYMPD_HTTP_PORT=8002
export MYMPD_SSL_PORT=4432
export MYMPD_SSL=true
mympd -w /var/lib/mympd2 -a /var/cache/mympd2
```

If the working directory is already present you should edit the files in the config directory. The MPD instance can be set with the webgui or with the corresponding files in the state directory.

If all is running fine, you should copy the startup file for each myMPD instance and add the workdir and cachedir option.
