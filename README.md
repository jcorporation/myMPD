![image](https://jcorporation.github.io/assets/mympd-logo-schriftzug.svg)

myMPD is a standalone and lightweight web-based MPD client. It's tuned for minimal resource usage and requires only very few dependencies. Therefore myMPD is ideal for raspberry pis and similar devices.

The backend is written in C and has no dependencies to external databases or webservers. The configuration is stored in plain text files and all the data is pulled on demand from MPD. The MPD database is the only source of truth for myMPD.

The frontend is mobile friendly, written as a PWA and offers on all devices the same functionality. It communicates over AJAX and websockets using the json-rpc 2 protocol.

myMPD also integrates extended features like an advanced jukebox mode, timers, triggers and smart playlists. With the integrated lua interpreter myMPD functions can also be scripted.

## Features

- Control mpd functions (play, pause, etc.)
- Set mpd options (repeat, random, etc.)
- MPD mount and neighbors support
- MPD partition support
- MPD output attributes
- Browse mpd database by tags (gridview)
- Browse filesystem
- Queue management
- Playlist management
- Advanced search
- Jukebox mode
- Customizable home screen and navigation bar
- [Smart playlists and saved searches](https://github.com/jcorporation/myMPD/wiki/Smart-playlists)
- Play statistics and song voting
- [Local albumart support: embedded and image per folder](https://github.com/jcorporation/myMPD/wiki/Albumart)
- [Unsynchronized and synchronized Lyrics](https://github.com/jcorporation/myMPD/wiki/Lyrics)
- Local booklet support (per album folder)
- HTTP stream support
- [Local playback of mpd http stream](https://github.com/jcorporation/myMPD/wiki/Local-playback)
- Timers and Triggers
- [System commands](https://github.com/jcorporation/myMPD/wiki/System-Commands)
- [Lua scripting](https://github.com/jcorporation/myMPD/wiki/Scripting)
- Scrobbler integration
- Embedded Webserver (mongoose)
- [Localized user interface](https://github.com/jcorporation/myMPD/wiki/Translating)
- Themeing
- [Publishing of mpd and myMPD directories via http and webdav](https://github.com/jcorporation/myMPD/wiki/Publishing-directories)
- Progressiv Web App enabled
- Support of Media Session API

To use all myMPD functions you should use the latest stable MPD version (0.22.x).

## Contribution

myMPD is in active development. If you like myMPD, you can help to improve it (no programming skills are required).

- [Help to improve myMPD](https://github.com/jcorporation/myMPD/issues/167).
- Use [issues](https://github.com/jcorporation/myMPD/issues) for bug reports only.
- Use [discussion](https://github.com/jcorporation/myMPD/discussions) for feature requests, questions and general feedback.
- If I accept a feature request, I will open an issue and assign a milestone to it.

## Screenshots

![image](https://jcorporation.github.io/assets/myMDPv6.8.3.gif)

## Installation

myMPD should run on all current linux distributions. There are four ways to install myMPD:

1. Use a linux distribution that delivers a myMPD package - the easiest way: [Distributions with myMPD](https://github.com/jcorporation/myMPD/wiki/Distributions-with-myMPD)
2. Use a prebuild package: [Prebuild-Packages](https://github.com/jcorporation/myMPD/wiki/Prebuild-Packages)
3. Use the docker image: [Docker](https://github.com/jcorporation/myMPD/wiki/Docker)
4. Use the ``build.sh`` script to compile myMPD: [Building myMPD](https://github.com/jcorporation/myMPD/wiki/Building-myMPD)

## Run

Adapt the configuration file `/etc/mympd.conf` to your needs (`/etc/webapps/mympd/mympd.conf` for Archlinux) or use the [mympd-config](https://github.com/jcorporation/myMPD/wiki/mympd-config) tool to generate automatically a valid `mympd.conf`. myMPD can be customized in many aspects, see the [configuration page](https://github.com/jcorporation/myMPD/wiki/Configuration) in the wiki for reference.


``
Usage: ./mympd [/etc/mympd.conf]
``

## Wiki

For further information on installation and configuration, see the [myMPD wiki](https://github.com/jcorporation/myMPD/wiki)

## Copyright

2018-2021 Juergen Mang <mail@jcgames.de>

myMPD is a fork of [ympd](https://github.com/notandy/ympd).
