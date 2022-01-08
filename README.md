![image](https://jcorporation.github.io/assets/mympd-logo-schriftzug.svg)

[![release](https://github.com/jcorporation/myMPD/actions/workflows/build_release.yml/badge.svg)](https://github.com/jcorporation/myMPD/actions/workflows/build_release.yml)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

myMPD is a standalone and lightweight web-based MPD client. It's tuned for minimal resource usage and requires only very few dependencies. Therefore myMPD is ideal for raspberry pis and similar devices.

The backend is written in C and has no dependencies to external databases or webservers. The configuration is stored in plain text files and all the data is pulled on demand from MPD. The MPD database is the only source of truth for myMPD.

The frontend is mobile friendly, written as a PWA and offers on all devices the same functionality. It communicates over AJAX and websockets using the json-rpc 2 protocol.

myMPD also integrates extended features like an advanced jukebox mode, timers, triggers and smart playlists. With the integrated lua interpreter myMPD functions can also be scripted.

## Screenshots

![image](https://jcorporation.github.io/assets/myMDPv6.8.3.gif)

## Features

- Control mpd functions (play, pause, etc.)
- Set mpd options (repeat, random, etc.)
- MPD mount and neighbors support
- MPD partition support
- MPD output attributes
- Browse mpd database by tags (gridview)
- Browse filesystem
- Webradios: [Webradiodb](https://jcorporation.github.io/webradiodb/) and [RadioBrowser](https://www.radio-browser.info/)
- Queue management
- Playlist management
- Advanced search
- Jukebox mode
- Customizable home screen and navigation bar
- [Smart playlists and saved searches](https://jcorporation.github.io/myMPD/references/smart-playlists)
- Playback statistics and song voting
- [Albumart: embedded and image per folder](https://jcorporation.github.io/myMPD/references/pictures)
- [Unsynchronized and synchronized Lyrics](https://jcorporation.github.io/myMPD/references/tags)
- Local booklet support (per album folder)
- HTTP stream support
- [Local playback of mpd http stream](https://jcorporation.github.io/myMPD/references/local-playback)
- Timers and Triggers
- [Lua scripting](https://jcorporation.github.io/myMPD/scripting/)
- Embedded Webserver (mongoose)
- [Localized user interface](https://jcorporation.github.io/myMPD/references/translating)
- Themeing
- [Publishing of mpd and myMPD directories via http](https://jcorporation.github.io/myMPD/references/published-directories)
- Progressiv Web App enabled
- Support of Media Session API

To use all myMPD functions you should use the latest stable MPD version.

## Contribution

myMPD is in active development. If you like myMPD, you can help to improve it (no programming skills are required).

- Star this repository to make it more popular.
- [Help to improve myMPD](https://github.com/jcorporation/myMPD/issues/167).
- Use [issues](https://github.com/jcorporation/myMPD/issues) for bug reports only.
- Use [discussion](https://github.com/jcorporation/myMPD/discussions) for feature requests, questions and general feedback.
- If I accept a feature request, I will open an issue and assign a [milestone](https://github.com/jcorporation/myMPD/milestones) to it.
- Consider [donating](https://jcorporation.github.io/donate) a coffee to this project.

## Installation

myMPD should run on all current linux distributions. There are four ways to install myMPD:

1. Use a linux distribution that delivers a myMPD package - the easiest way: [Distributions with myMPD](https://jcorporation.github.io/myMPD/installation/distributions)
2. Use a prebuild package: [Prebuild-Packages](https://jcorporation.github.io/myMPD/installation/prebuild-packages)
3. Use the docker image: [Docker](https://jcorporation.github.io/myMPD/installation/docker)
4. Use the ``build.sh`` script to compile myMPD: [Building myMPD](https://jcorporation.github.io/myMPD/installation/compiling)

## Run

On the first startup myMPD reads some environments variables and tries to autodetect the MPD connection configuration.

- [Configuration](https://jcorporation.github.io/myMPD/configuration/)

``
Usage: ./mympd
``

## Documentation

For further information on installation and configuration, see the [myMPD documentation](https://jcorporation.github.io/myMPD/)

## Copyright

2018-2022 Juergen Mang <mail@jcgames.de>

myMPD was originally a fork of [ympd](https://github.com/notandy/ympd), but it has evolved into a much more featurefull MPD client.
