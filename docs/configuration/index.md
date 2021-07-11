---
layout: page
permalink: /configuration/
title: Configuration
---

myMPD has no single configuration file. Most of the options are configureable through the settings dialog in the web ui.

You can set some basic options with command line options. All these options have sane default values and should not be changed for default usage.

The `workdir` option is useful if you want to run more then one instance of myMPD on the same host.

| OPTION | DESCRIPTION |
| ------ | ----------- |
| -c, --config | creates config and exits (default directory: /var/lib/mympd/config/) |
| -h, --help | displays this help |
| -u, --user `<username>`| username to drop privileges to (default: mympd) |
| -s, --syslog | enable syslog logging (facility: daemon) |
| -w, --workdir `<path>` | working directory (default: /var/lib/mympd) |
{: .table }

At first startup (if there is no ẁorking directory) myMPD tries to autodetect the MPD connection and reads some environment variables.

After first startup all environment variables are ignored and the file in the directory `/var/lib/mympd/config/` should be edited.

| FILE | ENVIRONMENT | DEFAULT | DESCRIPTION |
| ---- | ----------- | ----------- |
| acl | MYMPD_ACL | | ACL to access the myMPD webserver: [ACL]({{ site.baseurl }}/configuration/acl), allows all hosts in the default configuration |
| http_host | MYMPD_HTTP_HOST | 0.0.0.0 | IP address to listen on |
| http_port | MYMPD_HTTP_PORT | 80 | Port to listen on. Redirect to `ssl_port` if `ssl` is set to `true` |
| loglevel | MYMPD_LOGLEVEL | 5 | ({{ site.baseurl }}/configuration/logging) |
| lualibs | MYMPD_LUALIBS | all | ({{ site.baseurl }}/references/scripting) |
| scriptacl | MYMPD_SCRIPTACL | +127.0.0.1 | ACL to access the myMPD script backend: [ACL]({{ site.baseurl }}/configuration/acl), allows only local connections in the default configuration |
| ssl | MYMPD_SSL | true | `true` = enables ssl |
| ssl_port | MYMPD_SSL_PORT | 443 | Port to listen to https traffic |
| ssl_san | MYMPD_SSL_SAN | | Additional SAN |
| custom_cert | MYMPD_CUSTOM_CERT | false | `true` = use custom ssl key and certificate |
| ssl_cert | MYMPD_SSL_CERT | | Path to custom ssl certificate file |
| ssl_key | MYMPD_SSL_KEY | | Path to custom ssl key file |
{: .table }

- More details on [SSL]({{ site.baseurl }}/configuration/ssl)

## MPD autodetection

myMPD tries to autodetect the mpd connection at first startup.

1. Searches for a valid `mpd.con` file and reads all interesting settings
2. Uses environment variables

| ENVIRONMENT | DEFAULT | DESCRIPTION |
| ----------- | ----------- |
| MPD_HOST | `/run/mpd/socket` | MPD host or path to mpd socket |
| MPD_PORT | 0.0.0.0 | MPD port |
{: .table }
