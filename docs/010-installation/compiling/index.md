---
title: Compiling
---

## Releases

| NAME | STATE | DESCRIPTION |
| ---- | ----- | ----------- |
| [latest release](https://github.com/jcorporation/myMPD/releases/latest) | stable | The latest stable release, this is the preferred image for daily, hassle-free usage |
| [master](https://github.com/jcorporation/myMPD/tree/master) | stable | the latest releas is created from the master branch |
| [devel](https://github.com/jcorporation/myMPD/tree/devel) | unstable | this branch is for the next bugfix release |
| other branches | unstable | development branches for new major and minor releases |

Get the appropriated tarball or clone the git repository and checkout the wanted branch.

**Example: Clone and use devel branch:**

```sh
git clone https://github.com/jcorporation/myMPD.git
git checkout devel
```

## Build Dependencies

myMPD has only a few dependencies beside the standard c libraries. Not installing the optional dependencies leads only to a smaller subset of myMPD functions.

- cmake >= 3.13
- libasan3 - for memcheck builds only
- Perl - to create translation files
- gzip - to precompress assets
- jq - json parsing
- lua - to precompile embedded lua libraries
- whiptail - for mympd-config
- Devel packages:
    - pcre2 - for pcre support
    - OpenSSL >= 1.1.0 - for https support
    - Optional:
        - libid3tag - to extract embedded coverimages
        - flac - to extract embedded coverimages
        - liblua >= 5.3.0 - for myMPD scripting
        - libmygpio - for GPIO scripting functions
- Documentation:
    - Doxygen
    - JSDoc
    - LuaDoc
    - mkdocs

You can type `./build.sh installdeps` as root to install the dependencies (works only for supported distributions). For all other distributions you must install the packages manually.

## Building myMPD

- [Easy build with the build.sh script](build-sh.md)
- [Advanced build with cmake](cmake.md)
- [Build it in Termux](termux.md)
- [Build it for OpenWrt](openwrt.md)
- [Build it for FreeBSD](freebsd.md)
