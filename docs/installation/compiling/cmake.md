---
layout: page
permalink: /installation/compiling/cmake
title: Compiling with cmake
---

myMPD uses cmake as build system.

Example: run cmake, build myMPD in the directory `build` and install the binaries in /usr/bin.

```sh
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
| MYMPD_DOC | ON | Installs documentation |
| MYMPD_DOC_HTML | OFF | Creates and installs the html documentation |
| MYMPD_EMBEDDED_ASSETS | ON | Embed assets in binary, default ON, OFF for Debug |
| MYMPD_ENABLE_EXPERIMENTAL | OFF | Enable experimental features |
| MYMPD_ENABLE_FLAC | ON | Enables flac support |
| MYMPD_ENABLE_IPV6 | ON | Enables IPv6 |
| MYMPD_ENABLE_ASAN | OFF | Enables build with address sanitizer |
| MYMPD_ENABLE_LIBID3TAG | ON | Enables libid3tag support |
| MYMPD_ENABLE_MYGPIOD | ON | Enables myGPIOd support |
| MYMPD_ENABLE_LUA | ON | Enables lua support |
| MYMPD_ENABLE_TSAN | OFF | Enables build with thread san |
| MYMPD_ENABLE_UBSAN | OFF | Enables build with undefined behavior sanitizer |
| MYMPD_MANPAGES | ON | Creates and installs manpages |
| MYMPD_MINIMAL | OFF | Enables minimal myMPD build, disables all MYMPD_ENABLE_* flags |
| MYMPD_STARTUP_SCRIPT | ON | Installs the startup script, valid values: ON, OFF, SYSTEMD, OPENRC, SYSVINIT, FREEBSD |
{: .table .table-sm}

## cmake build types

- **Release, MinSizeRel**
  - Uses predefined compile and link options for a release build
  - Embed assets in binary
  - No debug output
  - Strips binary
- **RelWithDebInfo**
  - Uses predefined compile and link options for a release build
  - Embed assets in binary
  - No debug output
  - Included Debug info
- **Debug**
  - Uses predefined compile and link options for a debug build
  - Assets are served from the `htdocs` folder in the source directory
  - Debug output
  - Included Debug info
- **None**
  - Use this option to set your own compile and link options

## Unit tests

- Set `MYMPD_BUILD_TESTING=ON`
- Build as normal
- Run `make test`
