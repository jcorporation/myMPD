---
layout: page
permalink: /security
title: Security
---

## General

- Update myMPD and the systems it runs on regulary
- Do not add files from untrusted sources to your music library

## Restrict access

myMPD should not be directly accessible from the internet. It is designed to run inside a relatively secure intranet.

- myMPD publishes several [directories]({{ site.baseurl }}/references/published-directories)
- the pin setting only secures some [api calls]({{ site.baseurl }}/references/api/methods)

If you want to access myMPD from the internet, you shoud add a reverse proxy with authentication and ssl encryption in front of it.

- [Behind a reverse proxy]({{ site.baseurl }}/additional-topics/behind-a-reverse-proxy)

## Security measures

Nevertheless myMPD is designed with security in mind.

- All input data is validated and size limited
- The webserver limits the number of connections and request sizes
- The C backend is compiled with hardening flags and is regulary checked with static code analyzers
- myMPD uses a [https://github.com/antirez/sds](dynamic string library) to prevent buffer-overflows
- Files are served with a strict [content security policy](https://developer.mozilla.org/en-US/docs/Web/HTTP/CSP) to prevent XSS attacks
- The javascript completely avoids parsing of strings to dom nodes

## Reporting

If you find a security bug in myMPD please report it and I will fix it as soon as possible.

Write a mail to [mail@jcgames.de](mail@jcgames.de).
