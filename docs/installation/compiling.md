---
layout: page
permalink: /installation/compiling
title: Compiling
---

## Releases

| NAME | STATE | DESCRIPTION |
|-|-|-|
| [latest release](https://github.com/jcorporation/myMPD/releases/latest) | stable | The latest stable release, this is the preferred image for daily, hassle-free usage |
| [master](https://github.com/jcorporation/myMPD/tree/master) | stable | the latest releases are created from the master branch |
| [devel](https://github.com/jcorporation/myMPD/tree/devel) | unstable | this branch is for stabilizing and testing the upcoming new myMPD release |
| other branches | unstable | very unstable development branches with breaking changes |
{: .table .table-sm }

Get the appropriated tarball or clone the git repository and switch to the wanted branch.

**Example: Clone and use devel branch:**
```
git clone https://github.com/jcorporation/myMPD.git
git checkout devel
```

## Building

The `build.sh` script is the one stop shop for building and packaging myMPD.

### Build Dependencies

myMPD has only a few dependencies beside the standard c libraries. Not installing the optional dependencies leads only to a smaller subset of myMPD functions.

- cmake >= 3.4
- libasan3 - for memcheck builds only
- Perl - to create translation files
- jq - to show translation statistics
- Devel packages:
  - pcre2 - for pcre support
  - Optional: 
    - OpenSSL >= 1.1.0 - for https support
    - libid3tag - to extract embedded coverimages
    - flac - to extract embedded coverimages
    - liblua >= 5.3.0 - for scripting myMPD

You can type `./build.sh installdeps` as root to install the dependencies (works only for supported distributions). For all other distributions you must install the packages manually.

- Build it in [Termux]({{ site.baseurl }}/installation/termux)

## Packaging

You can self create packages for your distribution:

- `./build.sh pkgalpine` for Alpine Linux
- `./build.sh pkgarch` for Arch based distributions (e.g. Manjaro)
- `./build.sh pkgdebian` for Debian based distributions (e.g. Ubuntu. Raspbian)
- `./build.sh pkgrpm` for RPM based distributions (e.g. openSUSE, Fedora)
- `./build.sh pkgdocker` to create a Docker image based on Alpine Linux
- For gentoo you have to create a local overlay: https://wiki.gentoo.org/wiki/Custom_repository, the ebuild file is in the directory `contrib/packaging/gentoo`
- Build a [OpenWrt package]({{ site.baseurl }}/installation/openwrt)

## Compiling and installing

### Compile time options

Compile time options are set through environment variables.

| ENVIRONMENT | DEFAULT | DESCRIPTION |
|-|-|-|
| EMBEDDED_ASSETS | - | ON = Embeds assets in binary, default ON for release else OFF |
| ENABLE_FLAC | ON | ON = Enables flac usage for extracting coverimages |
| ENABLE_IPV6 | OFF | ON = Enables IPv6 |
| ENABLE_LIBASAN | - | ON = compile with libasan, default ON for memcheck else OFF |
| ENABLE_LIBID3TAG | ON | ON = Enables libid3tag usage for extracting coverimages |
| ENABLE_LUA | ON | ON = Enables scripting support with lua |
| ENABLE_SSL | ON | ON = Enables SSL, requires OpenSSL >= 1.1.0 |
| EXTRA_CMAKE_OPTIONS | | Extra options for cmake |
| MANPAGES | ON | ON = build manpages |
| MYMPD_INSTALL_PREFIX | /usr | Installation prefix for myMPD |
{: .table .table-sm}

There are three compile targets for myMPD.

_If compilation fails and you are building on top of an old version, try to run `./build.sh cleanup` before._

### Release 

- `./build.sh release` builds the release binaries
  - Directory: release
  - Assets embedded in binary
  - Binary is stripped
- `./build.sh install` installs the release binaries (run as root)

You can use `./build.sh releaseinstall` to compile and install in one step.

### Debug

- `./build.sh debug` builds the debug binaries
  - Directory: debug
  - Plain assets in htdocs directory
  - Use this to debug mympd with valgrind or gdb

- `./build.sh memcheck` builds the debug binaries
  - Directory: debug
  - Plain assets in htdocs directory
  - Binary is statically linked with libasan

### Test

To run the unit tests:

- `./build.sh test`

### Removing

- `./build.sh uninstall` to remove only binaries
- `./build.sh purge` to remove all

## Advanced Options

For advanced options type ``./build.sh help``.

## Cross compiling with Debian

The build script can use sbuild and qemu to cross compile debian packages, thanks to #264 @tsunulukai.

1. Set target distributions: `export DISTROS="buster stretch"`
2. Set target architectures: `export TARGETS="armhf armel"`
3. `sudo -E ./build.sh sbuild_chroots` to create chroot environments for build
4. `sudo -E ./build.sh sbuild_build` to build the packages

The successfully build packages can be found in the `packager/builds` directory.

***

**Note:** Since v6.12.0 the source does not include prebuild assets. If you do not use the provided build scripts, you must build the assets before, e. g.:

```
export MYMPD_BUILDIR="build"
./build.sh createassets
```
