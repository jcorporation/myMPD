![image](https://jcorporation.github.io/assets/mympd-logo-schriftzug.svg)

[![release](https://github.com/jcorporation/myMPD/actions/workflows/build_release.yml/badge.svg)](https://github.com/jcorporation/myMPD/actions/workflows/build_release.yml)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

myMPD is a standalone and lightweight web-based MPD client. It's tuned for minimal resource usage and requires only very few dependencies. Therefore myMPD is ideal for raspberry pis and similar devices.

The backend is written in C and has no dependencies to external databases or webservers. The configuration is stored in plain text files and all the data is pulled on demand from MPD. The MPD database is the only source of truth for myMPD.

The frontend is mobile friendly, written as a PWA and offers on all devices the same functionality. It communicates over AJAX and websockets using the json-rpc 2 protocol.

myMPD also integrates extended features like an advanced jukebox mode, timers, triggers and smart playlists. With the integrated lua interpreter myMPD functions can also be scripted.

## Screenshots

![image](https://jcorporation.github.io/myMPD/assets/myMDPv9.3.0.gif)

## Features

- Concurrent MPD partition support
- Control mpd functions and options
- MPD option presets
- Queue and playlist management
- Browse mpd database by tags or filesystem
- Webradios: [Webradiodb](https://jcorporation.github.io/webradiodb/) and [RadioBrowser](https://www.radio-browser.info/)
- Advanced search
- Jukebox mode
- MPD mount and neighbors support
- Customizable home screen and navigation bar
- [Smart playlists and saved searches](https://jcorporation.github.io/myMPD/references/smart-playlists)
- Playback statistics and song voting
- [Albumart: embedded and image per folder](https://jcorporation.github.io/myMPD/references/pictures)
- [Unsynchronized and synchronized Lyrics](https://jcorporation.github.io/myMPD/references/lyrics)
- Local booklet support (per album folder)
- [Local playback of mpd http stream](https://jcorporation.github.io/myMPD/references/local-playback)
- Timers and Triggers
- [Lua scripting](https://jcorporation.github.io/myMPD/scripting/)
- [Localized user interface](https://jcorporation.github.io/myMPD/references/translating)
- Dark and Light theme
- [Publishing of mpd and myMPD directories via http](https://jcorporation.github.io/myMPD/references/published-directories)
- Progressive Web App enabled
- Support of Media Session API
- ListenBrainz and MusicBrainz integration

To use all myMPD functions you should use the latest stable MPD version.

## Documentation

For information on installation and configuration, see the [myMPD documentation](https://jcorporation.github.io/myMPD/)

- [Installation](https://jcorporation.github.io/myMPD/installation/)
- [Configuration](https://jcorporation.github.io/myMPD/configuration/)
- [Running](https://jcorporation.github.io/myMPD/running)

## Support

Please read the [documentation](https://jcorporation.github.io/myMPD/) before asking for help. Bugs should be reported through [issues](https://github.com/jcorporation/myMPD/issues). For all other question and general feedback use the [discussions](https://github.com/jcorporation/myMPD/discussions).

## Contribution

myMPD is in active development. If you like myMPD, you can help to improve it (no programming skills are required).

- Star this repository to make it more popular.
- [Help to improve myMPD](https://github.com/jcorporation/myMPD/issues/167).
- Consider [donating](https://jcorporation.github.io/donate) a coffee to this project.

## Copyright

2018-2022 Juergen Mang <mail@jcgames.de>

myMPD was originally a fork of [ympd](https://github.com/notandy/ympd), but it has evolved into a much more comprehensive MPD client.
