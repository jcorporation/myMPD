---
layout: page
permalink: /installation/compiling/freebsd
title: FreeBSD
---

This is a general how-to for building myMPD on FreeBSD. It is highly recommended to check the FreeBSD handbook if you are unsure how software installation and building works.

- [https://docs.freebsd.org/en/books/handbook/ports/](https://docs.freebsd.org/en/books/handbook/ports/)

FreeBSD is supported by [https://github.com/jiixyj/epoll-shim](https://github.com/jiixyj/epoll-shim).

## Building directly from the ports tree

This is the basic way to install myMPD on FreeBSD system and it is only advised
for those using ports tree to install software directly.

It is expected you have /usr/ports populated.

1. Copy the contents of `contrib/packaging/freebsd` to `/usr/ports`
2. Generate the checksums:
    ```
    cd /usr/ports/multimedia/mympd/
    make makesum
    ```
3. Install myMPD:
    ```
    make install
    make clean
    ```

## Building with poudriere

This is more advanced way to build packages for pkg(1) package manager.
Consult the handbook how to setup the poudriere and its building jails.

I would use the poudriere's ports tree installed in `/usr/local/poudriere/ports/local`, change this to your local setup.

1. Copy the contents of `contrib/packaging/freebsd` to `/usr/local/poudriere/ports/local`
2. Generate the checksums:
    ```
    cd /usr/local/poudriere/ports/local/multimedia/mympd
    make makesum
    ```
3. If the previous step fails, you may need to create temporary symlink from `/usr/ports` to this local tree (revert it back afterwards):
    ```
    mv /usr/ports /usr/ports.bu
    ln -s /usr/local/poudriere/ports/local /usr/ports
    ```
4. Build the myMPD package (consult the options with the handbook):
    ```
    poudriere bulk -j 13amd64 -p local multimedia/mympd
    ```
5. Install the package. You may need to add poudriere's repository if you haven't done that earlier.
    ```
    pkg install myMPD
    ```
