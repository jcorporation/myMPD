---

title: Known issues
---

### myMPD startup is slow and MPD runs with 100% cpu usage

#### Cause

myMPD creates on first startup and on each database refresh the album cache. It searches through the whole MPD database and this could cause heavy cpu usage on large databases.

#### Workaround

You can disable the creation of the caches, but you loose the album browse feature of myMPD
or you can change the album cache mode to simple.

- [Configuration](020-configuration/configuration-files.md)

### Accept-Encoding not honored

- myMPD issue: [#738](https://github.com/jcorporation/myMPD/issues/738)

#### Cause

Parsing this header and inflating the embedded compressed assets is not implemented.

#### Workaround

None

***

### Response line too large

- myMPD issue: [#524](https://github.com/jcorporation/myMPD/issues/524)
- libmpdclient issue: [#69](https://github.com/MusicPlayerDaemon/libmpdclient/issues/69)

#### Cause

The mpd client library libmpclient uses a fixed buffer of 4096 bytes to get
response lines from MPD. If a response line is larger than this limit, this
error occurs. Most of the time the response line is to large, because of a tag
that length exceeds this limit.

libmympdclient uses a fixed buffer size of 8192 bytes.

#### Workaround

- Disable the tag in mpd.conf or in myMPD if you do not use it
- Crop the tag value

***

### Output buffer is full

- myMPD issue: [#528](https://github.com/jcorporation/myMPD/issues/528)

### Cause

MPD has an output buffer with a default max size of 8 MB. If a response is
larger than this limit, this error occurs.

#### Workaround

- Increase the output buffer size in mpd.conf - can cause further issues
- Limit the response size - the better workaround
  - Decrease the number of enabled tags to use
  - Decrease the number of elements per page
