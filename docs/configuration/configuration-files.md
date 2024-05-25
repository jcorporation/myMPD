---
layout: page
permalink: /configuration/configuration-files
title: Configuration files
---

At the first start (if there is no config folder in the working folder) myMPD reads some environment variables and writes the configuration files.

<div class="alert alert-warning">
After the first start all environment variables are ignored, except loglevel.
</div>

To change these settings afterwards, you can use the `mympd-config` utility and restart myMPD. As an alternative you can edit the files in the folder `/var/lib/mympd/config/`.

You can use `mympd -c` to create the initial configuration in the `/var/lib/mympd/config/` directory.

**Note:**

Use [systemd-run]({{ site.baseurl }}/running#manual-startup), if you use a distribution with systemd, e.g.:

```sh
systemd-run -p DynamicUser=yes -p StateDirectory=mympd -p CacheDirectory=mympd -E MYMPD_LOGLEVEL=4 -E MYMPD_HTTP=false -E MYMPD_SSL_PORT=1333 mympd -c
```

## General options

| FILE | TYPE | ENVIRONMENT | DEFAULT | DESCRIPTION |
| ---- | ---- | ----------- | ------- | ----------- |
| acl | string | MYMPD_ACL | | ACL to access the myMPD webserver: [ACL]({{ site.baseurl }}/configuration/acl), allows all hosts in the default configuration |
| album_group_tag | string | MYMPD_ALBUM_GROUP_TAG | Date | Additional tag to group albums |
| album_mode | string | MYMPD_ALBUM_MODE | adv | Set the album mode: `adv` or `simple` |
| covercache_keep_days | number | MYMPD_COVERCACHE_KEEP_DAYS | 31 | How long to keep images in the covercache; 0 to disable the cache; -1 to disable pruning of the cache. |
| http | boolean | MYMPD_HTTP | true | `true` = Enable listening on http_port |
| http_host | string | MYMPD_HTTP_HOST | `[::]` | IP address to listen on, use `[::]` to listen on IPv6 and IPv4 |
| http_port | number | MYMPD_HTTP_PORT | 80 | Port to listen for plain http requests. Redirects to `ssl_port` if `ssl` is set to `true`. *1 |
| loglevel | number | MYMPD_LOGLEVEL | 5 | [Logging]({{ site.baseurl }}/configuration/logging) - this environment variable is always used |
| mympd_uri | string | MYMPD_URI | auto | `auto` or uri to myMPD listening port, e.g. `https://192.168.1.1/mympd` |
| pin_hash | string | N/A | | SHA256 hash of pin, create it with `mympd -p` |
| save_caches | boolean | MYMPD_SAVE_CACHES | true | `true` = saves caches between restart, `false` = create caches on startup |
| scriptacl | string | MYMPD_SCRIPTACL | +127.0.0.1 | ACL to access the myMPD script backend: [ACL]({{ site.baseurl }}/configuration/acl), allows only local connections in the default configuration. The acl above must also grant access. |
| stickers | boolean | MYMPD_STICKERS | true | Enables the support for MPD stickers. |
| stickers_pad_int | boolean | MYMPD_STICKERS_PAD_INT | false | Enables the padding of integer sticker values (12 digits). |
{: .table .table-sm }

1. If http_port is disabled: The MPD curl plugin must trust the myMPD CA or certificate checking must be disabled. MPD fetches webradio playlists with http(s) from myMPD webserver.

## SSL options

| FILE | TYPE | ENVIRONMENT | DEFAULT | DESCRIPTION |
| ---- | ---- | ----------- | ------- | ----------- |
| ssl | boolean | MYMPD_SSL | true | `true` = enable listening on ssl_port, enables also the redirection from http_port to ssl_port |
| ssl_port | number | MYMPD_SSL_PORT | 443 | Port to listen for https requests |
| ssl_san | string | MYMPD_SSL_SAN | | Additional SAN for certificate creation |
| custom_cert | boolean | MYMPD_CUSTOM_CERT | false | `true` = use custom ssl key and certificate |
| ssl_cert | string | MYMPD_SSL_CERT | | Path to custom ssl certificate file |
| ssl_key | string | MYMPD_SSL_KEY | | Path to custom ssl key file |
{: .table .table-sm }

- More details on [SSL]({{ site.baseurl }}/configuration/ssl)
