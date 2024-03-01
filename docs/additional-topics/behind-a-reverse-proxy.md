---
layout: page
permalink: /additional-topics/behind-a-reverse-proxy
title: Behind a reverse proxy
---

myMPD could be hosted behind a reverse proxy to add features like http2 and authentication.

Configure myMPD to disable ssl and listen on localhost on port 8080.

```sh
printf "false" > /var/lib/mympd/config/ssl
printf "8080" > /var/lib/mympd/config/http_port
printf "127.0.0.1" > /var/lib/mympd/config/http_host
```

The reverse proxy should be configured to:

- remove the subdirectory
- rewrite location headers
- support websockets for the `/ws/` uri

In this examples myMPD is proxied behind the path `/mympd`.

## Nginx

```
location /mympd/ {
  proxy_pass http://127.0.0.1:8080/;
  proxy_redirect / /mympd/;
}
```

## Apache

### Basic setup with proxypass

To access mympd behind apache2 enable following modules, e.g. with `a2enmod`:

- proxy
- proxy_wstunnel

Add the following to a existing or a new virtual host:

```
ProxyRequests Off 
ProxyPreserveHost on
ProxyVia On

ProxyPass /mympd/ws/ ws://127.0.0.1:8080/ws/

Redirect /mympd /mympd/
ProxyPass /mympd/ http://127.0.0.1:8080/
ProxyPassReverse /mympd/ http://127.0.0.1:8080/
```

### Basic Password Authentication

To add basic password authentication create a new htpasswd file 
`sudo htpasswd -c <path/to/htpasswd/file> <username to access mympd>`

Or append an existing htpasswd file
`sudo htpasswd <path/to/htpasswd/file> <username to access mympd>`

for example:
`sudo htpasswd -c /etc/apache2/.htpasswds mympd`

Then add the following to your mympd Location in your apache virtualhost configuration file

```
AuthType Basic
AuthName "Authentication Required" # or add any type of identifier you prefer
AuthUserFile <path/to/htpasswd/file> # for example /etc/apache2/.htpasswds
Require valid-user
```

for example:

```
<Location "/mympd/">
  AuthType Basic
  AuthName "Authentication Required" # or add any type of identifier you prefer
  AuthUserFile <path/to/htpasswd/file> # for example /etc/apache2/.htpasswds
  Require valid-user
</Location>
```
See https://wiki.apache.org/httpd/PasswordBasicAuth for more information

### Simple Access Control

To limit acces to mympd to the local network, add the following to the location directive adjusting for your own network:

```
Require host localhost 
Require ip <ipaddress>
Require ip <ip range/netmask> # ex 192.168.1.0/24
```

See https://httpd.apache.org/docs/2.4/howto/access.html for more information

### Full Example Config

```
MergeSlashes OFF

<VirtualHost *:80>
  ServerName yourserver.example.com

  ProxyRequests Off
  ProxyPreserveHost on
  ProxyVia On

  <Location "/mympd/">
    Require host localhost
    Require ip 192.168.1.0/24

    AuthType Basic
    AuthName "Authentication Required"
    AuthUserFile /etc/apache2/.passwds/mympd
    Require valid-user
  </Location>

  ProxyPass /mympd/ws/ ws://127.0.0.1:8080/ws/

  Redirect /mympd /mympd/
  ProxyPass /mympd/ http://127.0.0.1:8080/
  ProxyPassReverse /mympd/ http://127.0.0.1:8080/
</VirtualHost>
```
