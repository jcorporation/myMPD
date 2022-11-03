---
layout: page
permalink: /installation/compiling/cmake
title: Compiling with cmake
---

myMPD uses cmake as build system.

Example: run cmake, build myMPD in the directory `build` and install the binaries in /usr/bin.

```
# Build as user
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr .
# binaries are placed in build/bin
make -C build
# Run install as root
sudo make -C build install
```

## myMPD specific cmake options

| OPTION | DEFAULT | DESCRIPTION |
| ------ | ------- | ----------- |
| MYMPD_DEBUG | OFF | Enables myMPD debug mode |
| MYMPD_EMBEDDED_ASSETS | - | ON = Embeds assets in binary, default ON for release, else OFF |
| MYMPD_ENABLE_FLAC | ON | ON = Enables flac usage for extracting coverimages |
| MYMPD_ENABLE_IPV6 | ON | ON = Enables IPv6 |
| MYMPD_ENABLE_LIBASAN | - | ON = compile with libasan, default ON for memcheck, else OFF |
| MYMPD_ENABLE_LIBID3TAG | ON | ON = Enables libid3tag usage for extracting coverimages |
| MYMPD_ENABLE_LUA | ON | ON = Enables scripting support with lua |
| MYMPD_ENABLE_SSL | ON | ON = Enables SSL, requires OpenSSL >= 1.1.0 |
| MYMPD_MANPAGES | ON | ON = build manpages |
| MYMPD_MINIMAL | OFF | ON = disables all MYMPD_ENABLE_* flags |
| MYMPD_STRIP_BINARY | ON | for release ON, else OFF |
{: .table .table-sm}

## cmake targets

1. Release: Uses predefined compile and link options for a release build
2. Debug: Uses predefined compile and link options for a debug build
3. None: Use this option to set your own compile and link options
