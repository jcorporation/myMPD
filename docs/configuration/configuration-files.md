---
layout: page
permalink: /configuration/configuration-files
title: Configuration files
---

At the first start (if there is no config folder in the working folder) myMPD reads some environment variables and writes the configuration files.

<div class="alert alert-warning">
After the first start all environment variables are ignored, except loglevel.
</div>

To change these settings afterwards, you must edit the files in the folder `/var/lib/mympd/config/` and restart myMPD.

You can use `mympd -c` to create the initial configuration in the `/var/lib/mympd/config/` directory.

**Note:**

Use [systemd-run]({{ site.baseurl }}/running#manual-startup), if you use a distribution with systemd, e.g.:

```
systemd-run -p DynamicUser=yes -p StateDirectory=mympd -p CacheDirectory=mympd -E MYMPD_LOGLEVEL=4 -E MYMPD_HTTP_PORT=0 -E MYMPD_SSL_PORT=1333 mympd -c
```

## General options

| FILE | TYPE | ENVIRONMENT | DEFAULT | DESCRIPTION |
| ---- | ---- | ----------- | ------- | ----------- |
| acl | string | MYMPD_ACL | | ACL to access the myMPD webserver: [ACL]({{ site.baseurl }}/configuration/acl), allows all hosts in the default configuration |
| covercache_keep_days | number | MYMPD_COVERCACHE_KEEP_DAYS | 31 | How long to keep images in the covercache, 0 to disable the cache |
| http_host | string | MYMPD_HTTP_HOST | 0.0.0.0 | IP address to listen on, use [::] to listen on IPv6 |
| http_port | number | MYMPD_HTTP_PORT | 80 | Port to listen for plain http requests. Redirects to `ssl_port` if `ssl` is set to `true`. Set to `0` to disable it. *1 |
| loglevel | number | MYMPD_LOGLEVEL | 5 | [Logging]({{ site.baseurl }}/configuration/logging) - this environment variable is always used |
| lualibs | string | MYMPD_LUALIBS | all | Comma separated list of lua libraries to load, look at [Scripting - LUA standard libraries]({{ site.baseurl }}/scripting#lua-standard-libraries) |
| mympd_uri | string | MYMPD_URI | auto | `auto` or uri to myMPD listening port, e.g. `https://192.168.1.1/mympd` |
| pin_hash | string | N/A | | SHA256 hash of pin, create it with `mympd -p` *2 |
| save_caches | boolean | MYMPD_SAVE_CACHES | true | `true` = saves caches between restart, `false` = create caches on startup |
| scriptacl | string | MYMPD_SCRIPTACL | +127.0.0.1 | ACL to access the myMPD script backend: [ACL]({{ site.baseurl }}/configuration/acl), allows only local connections in the default configuration. The acl above must also grant access. |
{: .table .table-sm }

1. If http_port is disabled: The MPD curl plugin must trust the myMPD CA or certificate checking must be disabled. MPD fetches webradio playlist with http(s) from myMPD webserver.
2. Only supported if myMPD is compiled with SSL support.

## SSL options

| FILE | TYPE | ENVIRONMENT | DEFAULT | DESCRIPTION |
| ---- | ---- | ----------- | ------- | ----------- |
| ssl | boolean | MYMPD_SSL | true | `true` = enables https, `false` = disables https |
| ssl_port | number | MYMPD_SSL_PORT | 443 | Port to listen for https requests |
| ssl_san | string | MYMPD_SSL_SAN | | Additional SAN for certificate creation |
| custom_cert | boolean | MYMPD_CUSTOM_CERT | false | `true` = use custom ssl key and certificate |
| ssl_cert | string | MYMPD_SSL_CERT | | Path to custom ssl certificate file |
| ssl_key | string | MYMPD_SSL_KEY | | Path to custom ssl key file |
{: .table .table-sm }

- More details on [SSL]({{ site.baseurl }}/configuration/ssl)
