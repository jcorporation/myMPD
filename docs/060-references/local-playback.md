---
title: Local playback
---

With the local playback feature myMPD can play the http stream from MPD directly in your browser.

If myMPD is served over https the stream must be also served over https. Mixed content is prohibited by modern browsers.
MPD can not serve a https stream, therefore myMPD integrates a reverse proxy. The reverse proxy publishes the mpd stream through the `/stream/<partition>` uri.

You must set the correct stream port in the settings dialog (Features tab).

Alternatively you can specify an uri, if the mpd stream is published through an other method.

MPD configuration:

```text
audio_output {
    type        "httpd"
    name        "HTTP Stream"
    encoder     "lame" #to support safari on ios
    port        "8000"
    bitrate     "128"
    format      "44100:16:1"
    always_on   "yes"
    tags        "yes"
}
```

!!! note
    Playback does not start instantinously, buffering can take up to 30 seconds.
