---
layout: page
permalink: /configuration/ssl-trust
title: SSL trust
---

To trust the myMPD CA you must add the `/var/lib/mympd/ssl/ca.pem` to the trust store of your client or server.

## Alpine Linux
```
cp /var/lib/mympd/ssl/ca.pem /usr/local/share/ca-certificates/mympd-ca.pem
update-ca-certificates
```

## Debian / Ubuntu

Run the following commands and select the `extra/mympd-ca.pem` to include.

```
mkdir /usr/share/ca-certificates/extra
cp /var/lib/mympd/ssl/ca.pem /usr/share/ca-certificates/extra/mympd-ca.pem
dpkg-reconfigure ca-certificates
```
