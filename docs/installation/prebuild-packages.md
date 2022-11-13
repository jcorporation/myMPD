---
layout: page
permalink: /installation/prebuild-packages
title: Prebuild packages
---

myMPD uses the openSUSE Build Service to build packages for releases.

- [Project homepage](https://build.opensuse.org/package/show/home:jcorporation/myMPD)
- [Repositories](https://download.opensuse.org/repositories/home:/jcorporation/)

Download the appropriated package for your distribution and install it with the package manager.

Following distributions are supported:

- Arch
- [Debian]({{ site.baseurl }}/installation/prebuild-packages-debian)
- Fedora
- Raspian
- openSUSE
- Ubuntu

## Dependencies

- libpcre2: for pcre support (8 bit runtime files)
- OpenSSL >= 1.1.0 (optional): for https support
- libid3tag (optional): to extract embedded albumart
- flac (optional): to extract embedded albumart
- liblua >= 5.3.0 (optional): for scripting myMPD
