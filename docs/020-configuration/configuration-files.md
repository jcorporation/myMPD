---
title: Configuration files
---

You can configure myMPD in different ways:

1. With environment variables
    1. Set environment variables as described below
    2. Start myMPD. myMPD grabs the environment variables and writes it's configuration files accordingly.

2. Use `mymmpd-config`
    1. Type `mympd-config` to configure myMPD with a ncurses based interface.
    2. Start myMPD.

3. Use `mympd -c`
    1. Type `mympd -c` to create the initial configuration in the `/var/lib/mympd/config/` directory.
    2. Edit the files and start myMPD.

!!! note
    Use [systemd-run](../030-running.md#manual-startup), if you use a distribution with systemd, e.g.:

    ```sh
    systemd-run -p DynamicUser=yes -p StateDirectory=mympd -p CacheDirectory=mympd -E MYMPD_LOGLEVEL=4 -E MYMPD_HTTP=false -E MYMPD_SSL_PORT=1333 mympd -c
    ```

## General options

| FILE | TYPE | ENVIRONMENT | DEFAULT | DESCRIPTION |
| ---- | ---- | ----------- | ------- | ----------- |
| acl | string | MYMPD_ACL | | ACL to access the myMPD webserver: [ACL](acl.md), allows all hosts in the default configuration |
| album_group_tag | string | MYMPD_ALBUM_GROUP_TAG | `Date` | Additional tag to group albums |
| album_mode | string | MYMPD_ALBUM_MODE | `adv` | Set the album mode: `adv` or `simple` |
| cache_cover_keep_days | number | MYMPD_CACHE_COVER_KEEP_DAYS | `31` | How long to keep images in the cover cache; 0 to disable the cache; -1 to disable pruning of the cache. |
| cache_http_keep_days | number | MYMPD_CACHE_HTTP_KEEP_DAYS | `31` | How long to keep successful responses in the http client cache; 0 to disable the cache; -1 to disable pruning of the cache. |
| cache_lyrics_keep_days | number | MYMPD_CACHE_LYRICS_KEEP_DAYS | `31` | How long to keep lyrics in the lyrics cache; 0 to disable the cache; -1 to disable pruning of the cache. |
| cache_misc_keep_days | number | MYMPD_CACHE_MISC_KEEP_DAYS | `1` | How long to keep files in the misc cache. |
| cache_thumbs_keep_days | number | MYMPD_CACHE_THUMBS_KEEP_DAYS | `31` | How long to keep images in the thumbnail cache; 0 to disable the cache; -1 to disable pruning of the cache. |
| ca_cert_store | string | MYMPD_CA_CERT_STORE | [2] | Path to the system CA certificate store. |
| cert_check | boolean | MYMPD_CERT_CHECK | `true` | Enable certificate checking for outgoing https connections. |
| http | boolean | MYMPD_HTTP | true | `true` = Enable listening on http_port |
| http_host | string | MYMPD_HTTP_HOST | `[::]` | IP address to listen on, use `[::]` to listen on IPv6 and IPv4 |
| http_port | number | MYMPD_HTTP_PORT | `8080` | Port to listen for plain http requests. Redirects to `ssl_port` if `ssl` is set to `true`. [1] |
| loglevel | number | MYMPD_LOGLEVEL | `5` | [Logging](logging.md) - this environment variable is always used |
| mympd_uri | string | MYMPD_URI | `auto` | `auto` or uri to myMPD listening port, e.g. `https://192.168.1.1/mympd` |
| pin_hash | string | N/A | | SHA256 hash of pin, create it with `mympd -p` |
| save_caches | boolean | MYMPD_SAVE_CACHES | `true` | `true` = saves caches between restart, `false` = create caches on startup |
| scriptacl | string | MYMPD_SCRIPTACL | `+127.0.0.1` | ACL to access the myMPD script backend: [ACL](acl.md), allows only local connections in the default configuration. The acl above must also grant access. |
| scripts_external | boolean | MYMPD_SCRIPTS_EXTERNAL | `false` | Allow myMPD to execute external scripts vie the `/script-api`-Endpoint. |
| stickers | boolean | MYMPD_STICKERS | `true` | Enables the support for MPD stickers. |
| stickers_pad_int | boolean | MYMPD_STICKERS_PAD_INT | `false` | Enables the padding of integer sticker values (12 digits). |
| webradiodb | boolean | MYMPD_WEBRADIODB | `true` | Enables the WebradioDB integration. |

1. If http_port is disabled: The MPD curl plugin must trust the myMPD CA or certificate checking must be disabled. MPD fetches webradio playlists with http(s) from myMPD webserver.
2. myMPD checks following locations for the ca cert store file:

    - `/etc/ssl/certs/ca-certificates.crt`
    - `/etc/ssl/certs/ca-bundle.crt`
    - `/etc/pki/tls/certs/ca-bundle.crt`
    - `/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem`

## SSL options

| FILE | TYPE | ENVIRONMENT | DEFAULT | DESCRIPTION |
| ---- | ---- | ----------- | ------- | ----------- |
| ssl | boolean | MYMPD_SSL | `true` | `true` = enable listening on ssl_port, enables also the redirection from http_port to ssl_port |
| ssl_port | number | MYMPD_SSL_PORT | `8443` | Port to listen for https requests |
| ssl_san | string | MYMPD_SSL_SAN | | Additional SAN for certificate creation |
| custom_cert | boolean | MYMPD_CUSTOM_CERT | `false` | `true` = use custom ssl key and certificate |
| ssl_cert | string | MYMPD_SSL_CERT | | Path to custom ssl certificate file |
| ssl_key | string | MYMPD_SSL_KEY | | Path to custom ssl key file |

- More details on [SSL](ssl.md)
