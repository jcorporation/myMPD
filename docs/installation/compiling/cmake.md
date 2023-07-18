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
| MYMPD_BUILD_TESTING | OFF | Enables building of unit tests |
| MYMPD_DEBUG | OFF | Enables myMPD debug mode, default OFF, ON for Debug |
| MYMPD_EMBEDDED_ASSETS | ON | Embed assets in binary, default ON, OFF for Debug |
| MYMPD_ENABLE_FLAC | ON | Enables flac support |
| MYMPD_ENABLE_IPV6 | ON | Enables IPv6 |
| MYMPD_ENABLE_LIBASAN | OFF | Enables build with libasan |
| MYMPD_ENABLE_LIBID3TAG | ON | Enables libid3tag support |
| MYMPD_ENABLE_LUA | ON | Enables lua support |
| MYMPD_MANPAGES | ON | Creates and installs manpages |
| MYMPD_MINIMAL | OFF | Enables minimal myMPD build, disables all MYMPD_ENABLE_* flags |
| MYMPD_STRIP_BINARY | ON | Enables stripping the binaries for Release |
{: .table .table-sm}

## cmake build types

1. Release
  - Uses predefined compile and link options for a release build
  - Embed assets in binary
  - Strips binary
2. Debug
  - Uses predefined compile and link options for a debug build
  - Assets are served from the `htdocs` folder in the source directory
3. None:
  - Use this option to set your own compile and link options

## Unit tests

- Set `MYMPD_BUILD_TESTING=ON`
- Build as normal
- Run `make test`
