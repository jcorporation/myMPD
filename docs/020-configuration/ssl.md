---
layout: page
permalink: /configuration/ssl
title: SSL
---

In the default configuration myMPD encrypts traffic on port 443, set `/var/lib/mympd/config/ssl` to `false` to disable encryption. If ssl is enabled, myMPD redirects port 80 to 443. If you want to install myMPD as an app in chrome you need ssl.

## Default certificate

Certificates are checked at startup and if necessary created or renewed. myMPD maintains a CA certificate and a server certificate signed with this ca.

- Lifetime of the CA certificate: approximately 10 years
- Lifetime of the server certificate: 1 year

The validity of the server certificate is so short because browsers no longer trust certificates with long durations, even if they are self-signed. On startup myMPD checks the expiration date of the certificate and renews it if necessary.

The default certificates are saved in the directory `/var/lib/mympd/ssl/`.

| FILE | DESCRIPTION |
| ---- | ----------- |
| ca.pem | Self signed CA certificate |
| ca.key | CA private key |
| server.pem | Server certificate |
| server.key | Server private key |
{: .table .table-sm}

The server certificates SAN is:

- DNS: localhost
- DNS: ip6-localhost
- DNS: ip6-loopback
- IP: 127.0.0.1
- IP: ::1
- DNS:`<hostname>`
- DNS:`<full qualified hostname>`
- IP:`<ip of resolved hostname>`
- IP:`all interface ips`

You can set the environment variable `MYMPD_SSL_SAN` before starting myMPD to add additional names or ip addresses to the certificate, e.g. `export MYMPD_SSL_SAN="DNS:jukebox.local"`.

To regenerate the server certificate stop myMPD, and remove the `/var/lib/mympd/ssl/server.crt` and `/var/lib/mympd/ssl/server.key` files.

## Default CA

You can download the CA certificate in the About dialog and import it in your operatingsystem or browser to avoid ugly ssl warnings and trust this ca. Do not import the server certificate.

## Custom certificate

You can of course use your own certificate, e.g. from [Let's Encrypt](https://letsencrypt.org/).

| FILE | CONTENT |
| ---- | ------- |
| ssl | true |
| custom_cert | true |
| ssl_key | path to custom ssl key |
| ssl_cert | path to custom ssl certificate |
{: .table .table-sm}
