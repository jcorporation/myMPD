---
layout: page
permalink: /installation/compiling/build-sh
title: Compiling with build.sh
---

With the `build.sh` script you can easily build myMPD and create your own packages. Downstream packagers should use [cmake]({{ site.baseurl }}/installation/compiling/cmake) directly.

## Compiling and installing

The `build.sh` script provides three compile targets for myMPD.

_If compilation fails and you are building on top of an old version, try to run `./build.sh cleanup` before._

### Release 

- `./build.sh release`
  - Builds release binaries
  - Directory: release
  - Assets embedded in binary
  - Binary is stripped
  - Install prefix is `/usr`
- `./build.sh install` installs the release binaries (run as root)

You can use `./build.sh releaseinstall` to compile and install in one step.

### Debug

- `./build.sh debug`
  - Builds debug binaries
  - Directory: debug
  - Plain assets in htdocs directory
  - Use this to debug mympd with valgrind or gdb

### Memcheck

- `./build.sh memcheck`
  - Builds debug binaries linked with libasan
  - Directory: debug
  - Plain assets in htdocs directory
  - You must preload libasan, e.g. `LD_PRELOAD=libasan.so.6 debug/bin/mympd`

## Removing

- `./build.sh uninstall` to remove only binaries
- `./build.sh purge` to remove all

## Create distribution specific packages

You can self create packages for your distribution:

- `./build.sh pkgalpine` for Alpine Linux
- `./build.sh pkgarch` for Arch based distributions (e.g. Manjaro)
- `./build.sh pkgdebian` for Debian based distributions (e.g. Ubuntu. Raspbian)
- `./build.sh pkgrpm` for RPM based distributions (e.g. openSUSE, Fedora)
- `./build.sh pkgdocker` to create a Docker image based on Alpine Linux
- For gentoo you have to create a local overlay: https://wiki.gentoo.org/wiki/Custom_repository, the ebuild file is in the directory `contrib/packaging/gentoo`
- Build a [OpenWrt package]({{ site.baseurl }}/installation/openwrt)

### Cross compiling debian packages

The build script can use sbuild and qemu to cross compile debian packages, thanks to #264 @tsunulukai.

1. Set target distributions: `export DISTROS="bullseye buster"`
2. Set target architectures: `export TARGETS="armhf armel"`
3. `sudo -E ./build.sh sbuild_chroots` to create chroot environments for build
4. `sudo -E ./build.sh sbuild_build` to build the packages

The successfully build packages can be found in the `packager/builds` directory.
