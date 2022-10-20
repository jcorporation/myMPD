---
layout: page
permalink: /security
title: Security
---

## General

- Update myMPD and the systems it runs on regularly
- Do not add files from untrusted sources to your music library

## Restrict access

myMPD should not be directly accessible from the internet. It is designed to run inside a relatively secure intranet.

- myMPD publishes several [directories]({{ site.baseurl }}/references/published-directories)
- the pin setting only secures some [api calls]({{ site.baseurl }}/references/api/methods)

If you want to access myMPD from the internet, you should add a reverse proxy with authentication and ssl encryption in front of it.

- [Behind a reverse proxy]({{ site.baseurl }}/additional-topics/behind-a-reverse-proxy)

## Security measures

Nevertheless myMPD is designed with security in mind.

- All input data is validated and size limited
- The webserver limits the number of connections and request sizes
- The C backend is compiled with hardening flags and is regularly checked with static code analyzers
- The debug and development builds are linked with libasan to detect memory errors
- myMPD uses a fork of [Simple Dynamic Strings](https://github.com/jcorporation/sds) to prevent buffer-overflows
- Files are served with a strict [Content Security](https://developer.mozilla.org/en-US/docs/Web/HTTP/CSP) and [Trusted Types](https://developer.mozilla.org/en-US/docs/Web/API/Trusted_Types_API) Policy to prevent XSS attacks
- The javascript frontend avoids parsing of strings to dom nodes
- All included dependencies are updated regularly

## Reporting

If you find a security bug in myMPD please report it and I will fix it as soon as possible.

Write a mail to [mail@jcgames.de](mailto:mail@jcgames.de).
