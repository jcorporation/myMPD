# myMPD Changelog

https://github.com/jcorporation/myMPD/

***

## myMPD v10.1.0 (not yet released)

This minor release adds support of new MPD 0.24 features. The javascript frontend is now documented with jsDoc and linted with the typescript compiler.

### Changelog

- Feat: autoconfiguration for playlist_directory (MPD 0.24) #836
- Feat: support consume oneshot (MPD 0.24) #837
- Feat: autoconfiguration for pcre support (MPD 0.24) #843
- Feat: support starts_with filter expression (MPD 0.24) #843
- Feat: support queue save modes (MPD 0.24) #848
- Feat: add elapsed sticker to save recent position from songs #781
- Feat: jsDoc compatible API documentation and linting with typescript compiler
- Upd: libmympdclient 1.0.15
- Upd: refactor translation framework
- Upd: Bootstrap 5.2.2

***

## myMPD v10.0.3 (not yet released)

This is a small bug fix release.

### Changelog

- Fix: Segmentation fault when clicking on last played list #850

***

## myMPD v10.0.2 (2022-10-03)

This is a small bug fix release that also updates some translations.

Many thanks to all translators!

### Changelog

- Upd: zh-CN, it-IT translations
- Fix: contextmenu for discs
- Fix: flashing progress bar
- Fix: translate locale select
- Fix: run feature detection only in main thread #845
- Fix: pass script_arguments in feedback trigger #846
- Fix: Gentoo Linux ebuild, thanks @itspec-ru #847
- Fix: add missing translation phrases attributes #849
- Fix: API usage in some example scripts #846

***

## myMPD v10.0.1 (2022-09-25)

This is a small bug fix release.

### Changelog

- Upd: fr-FR, nl-NL translations
- Fix: js error clearing current title
- Fix: translate more elements

***

## myMPD v10.0.0 (2022-09-22)

This major release adds concurrent MPD partition support to myMPD. myMPD connects now to all MPD partitions simultaneously and holds partition specific states and settings. Each browser instance can now select the MPD partition to control.

The partition feature should be used with MPD 0.23.9 or newer. There are some annoying partition related bugs in earlier MPD versions.

The syntax of the last_played file has changed. You can convert it with
`sed -r 's/^(.*)::(.*)/{"LastPlayed":\1,"uri":"\2"}/g' /var/lib/mympd/state/last_played > /var/lib/mympd/state/default/last_played_list`

### Per partition features

- Highlight color
- Jukebox
- Last played
- Local player
- MPD options
- Triggers
- Timers

### Changelog

- Feat: concurrent MPD partition support #440
- Feat: partition specific settings #440 #826
- Feat: custom uri for local playback #826
- Feat: new API method MYMPD_API_PLAYER_VOLUME_CHANGE
- Upd: autoconfiguration improvements
- Upd: add internal api documentation
- Upd: build improvements
- Upd: Mongoose 7.8
- Upd: mjson
- Upd: Bootstrap 5.2.1
- Upd: add animation for update progress of views
- Upd: de-DE, fr-FR, nl-NL translations, thanks @tsunulukai
- Fix: case insensitive sorting of webradioDB
- Fix: popover menu for playlists on home screen
- Fix: set default sort tag for smart playlists save modal
- Fix: localize select options #834
- Fix: sort queue by pos #835

***

## mympd v9.5.4 (2022-09-11)

This is a small bug fix release that also updates some translations.

Many thanks to all translators!

### Changelog

- Upd: fr-FR, nl-NL, zh-CN (#833) translations
- Fix: allow empty smart playlist prefix #830
- Fix: default fallback to en-US #830
- Fix: serving zh-CN locale #830

***

## myMPD v9.5.3 (2022-08-29)

This is a small bug fix release.

### Changelog

- Upd: fr-FR translation
- Fix: rename cn-CHS to zh-CN #820
- Fix: read env variables at first startup #821
- Fix: use correct columns to fetch current queue view #822

***

## myMPD v9.5.2 (2022-08-24)

This is a small bug fix release.

### Changelog

- Feat: add battery indicator script, thanks @furtarball #815
- Upd: chinese translation #813
- Fix: sorting albums by last-modified #806

***

## myMPD v9.5.1 (2022-08-14)

This is a small bug fix release.

### Changelog

- Feat: add update_home jsonrpc event #814
- Fix: advanced search is not working #809
- Fix: fetching locales and ligatures behind reverse proxy #808

***

## myMPD v9.5.0 (2022-08-13)

This release improves and documents the myMPD backend code, but there are also some minor new features added. I further added the doxygen generated internal api documentation to the myMPD documentation site: [Doxygen generated internal API documentation](https://jcorporation.github.io/myMPD/doxygen/html/index.html).

This release removes the compatibility code for MPD versions older than 0.21.

### Changelog

- Feat: support TitleSort tag (mpd 0.24) #797
- Feat: use custom X-myMPD-Session http header for myMPD sessions to allow other authorization methods in reverse proxy setups
- Feat: respect last played in jukebox album mode #792 #794
- Feat: improved translation workflow integrating [POEditor](https://poeditor.com/join/project/Z54inZwdul) #803
- Feat: album view lists albums without AlbumArtist tags again #791
- Feat: fetch ligatures and i18n json only on demand
- Upd: remove compatibility code for MPD 0.20 and lower
- Upd: rename some API methods for consistency
- Upd: covercache expiry time is now a config setting (removed from GUI)
- Upd: improve many internal functions and there api
- Upd: add doxygen style internal api documentation
- Upd: add more unit tests
- Upd: Bootstrap 5.2
- Upd: use dh_helpers for debian packaging
- Fix: use only sort tags that are configured in MPD
- Fix: add missing default values
- Fix: cache building on reconnect if stickers are disabled
- Fix: hide input in pin enter dialog

***

## myMPD v9.4.1 (2022-07-19)

This is a small bugfix release.

### Changelog

- Fix: read custom navbar_icons #793
- Fix: javascript error in album detail view #795
- Fix: certificate creation #796

***

## myMPD v9.4.0 (2022-07-13)

This is mostly a maintenance release to cleanup code. It adds small improvements, optimizes the codebase and uses now a faster sorting algorithm.

This release removes the implicit fallback from AlbumArtist to Artist tag. Disable the AlbumArtist tag if you do not maintain it.

### Changelog

- Feat: sort option for all db tags #764
- Feat: add volume controls to local playback #777
- Feat: import lua scripts #765
- Feat: support of http links on the homescreen #785
- Feat: move scripts to musicdb-scripts repository #770
- Upd: improve albumcache
- Upd: improve partition support #440
- Upd: rework handling of missing AlbumArtist tag
- Upd: Bootstrap 5.2 Beta1
- Upd: BSN 4.2
- Upd: mongoose 7.7
- Upd: mjson
- Upd: utf8.h
- Upd: unit testing framework
- Upd: improve proxy code
- Upd: improve linked list code
- Upd: faster sorting (using rax) #764
- Upd: some more code optimizations
- Upd: rework source tree
- Upd: improve documentation
- Upd: add many unit tests
- Fix: show sticker values in playlist details view

***

## myMPD v9.3.4 (2022-06-07)

This is a small bugfix release.

### Changelog

- Feat: add simplified chinese translation, thanks @dream7180
- Fix: use correct filter after fetching webradiodb #768

***

## myMPD v9.3.3 (202-05-23)

This is a small bugfix release.

### Changelog

- Upd: dutch translation, thanks @ItaintYellow #763
- Fix: use-after-free bug listing database tags
- Fix: correct construction of booklet path

***

## myMPD v9.3.2 (2022-05-15)

This is a bugfix release.

### Changelog

- Fix: quick remove song from queue
- Fix: switch between mobile and desktop view
- Fix: memory leak listing webradio favorites
- Fix: memory leak in stream reverse proxy
- Fix: open folder/download buttons on song details modal do not work #758
- Fix: save scaleRatio setting

***

## myMPD v9.3.1 (2022-05-09)

This release fixes only a debian dependency error.

### Changelog

- Fix: stick lua dependency to 5.3 #755

***

## myMPD v9.3.0 (2022-05-09)

This release adds many small improvements and starts the integration of MusicBrainz and ListenBrainz.

### Changelog

- Feat: support new features of webradiodb #719
- Feat: add link to containing folder in song details modal #729
- Feat: add Permissions-Policy header #733
- Feat: add autoplay for local playback #734
- Feat: add mympd_feedback trigger #394
- Feat: add feedback script for ListenBrainz #394
- Feat: add setting for ListenBrainz Token #394
- Feat: scrollable navbar #728
- Feat: serve thumbnails for album grid #732
- Feat: support semicolon separated tag values for MUSICBRAINZ_ARTISTID and MUSICBRAINZ_ALBUMARTISTID
- Feat: add shell script to download and manage albumart
- Feat: new quick play and remove action #749
- Upd: mobile ux enhancements #748 #752
- Upd: remove obsolete headers #733
- Upd: responsive design enhancements for covers and playback card
- Fix: compilation with gentoo #731
- Fix: set crossorigin="use-credentials" for webmanifest #739
- Fix: handling of two letter language codes

***

## myMPD v9.2.4 (2022-04-19)

This is a small bugfix release.

### Changelog

- Upd: sync with upstream sds source
- Upd: some documentation improvements
- Upd: improved logging for mpd authentication
- Fix: some compilation issues

***

## myMPD v9.2.3 (2022-04-01)

This is a small bugfix release.

### Changelog

- Upd: light theme improvements
- Fix: custom select search bugs
- Fix: playlist select bugs
- Fix: add AF_NETLINK to RestrictAddressFamilies (systemd unit) #716
- Fix: cover fade-in / fade-out issues #715
- Fix: simple queue search #718

***

## myMPD v9.2.2 (2022-03-25)

This is a small bugfix release.

### Changelog

- Upd: enabled IPv6 by default
- Upd: performance improvements for filesystem and tag views
- Fix: display of coverimages in songdetails modal without direct access to mpd music directory
- Fix: do not count embedded images if music directory is not available
- Fix: do not end albumart / readpicture loop too early #702
- Fix: GCC 12 build errors #706 #707
- Fix: display correct webserver ip-address #712
- Fix: improved virtual cuesheet handling in song details modal #714

***

### myMPD v9.2.1 (2022-03-11)

This is a small bugfix release.

### Changelog

- Upd: korean translation #703
- Upd: improve http connection handling
- Fix: memory issues in albumgrid view #704

***

## myMPD v9.2.0 (2022-03-06)

This release updates components, adds support of multiple embedded images, improves the queue search (MPD 0.24.0) and implements some other minor features.

### Changelog

- Feat: improved queue search (sort and prio filter - MPD 0.24.0) #686
- Feat: support of MOOD tag (MPD 0.24.0)
- Feat: support of multiple embedded images for mp3, flac and ogg files #439
- Feat: case insensitive sorting for database tags #677
- Feat: configurable startup view #684
- Feat: new keyboard shortcuts
- Upd: libmympdclient 1.0.11
- Upd: BSN 4.1 #662
- Upd: more http compliant path for retrieving albumart #656
- Upd: improved search as you type #676
- Upd: support debian bullseye for cross compiling #698
- Upd: remove mpd.conf parsing, improve auto configuration docu #696
- Fix: hash filenames in covercache #697

***

## myMPD v9.1.2 (2922-02-17)

This is a small bugfix release.

### Changelog

- Fix: do not omit first entry of search result in queue #681
- Fix: allow filename as tag #681
- Fix: disable goto playing song button if queue is empty #678
- Fix: some database update glitches
- Fix: database rescan from filesystem view
- Fix: correct http headers for serving embedded albumart
- Fix: serving albumart read by MPD
- Fix: synced lyrics timestamps per words
- Fix: remove windows line ending from unsynced lyrics #683

***

## myMPD v9.1.1 (2022-02-08)

This is a small maintenance release.

### Changelog

- Feat: Use WebradioDB image for homescreen icon #674
- Upd: korean translation #669
- Fix: searchable selects in mobile browsers #673
- Fix: case insensitive search for WebradioDB #672
- Fix: read environment if /var/lib/mympd/config does not exist #675
- Fix: add all IPs to certificates SAN #675
- Fix: remove zone identifiers from IPv6 addresses #675
- Fix: refresh queue popover after queue state change
- Fix: save jukebox mode setting

***

### myMPD v9.1.0 (2022-01-29)

This release adds the webradio feature and some other small enhancements and updates.

You can now manage your own webradio favorites and browse the databases of my webradioDB project and the popular RadioBrowser.

myMPD now supports also extended m3u playlists and can list the contents of playlists saved in the music directory.

The central folder for images has been replaced by individual folders by image type (backgrounds and thumbs). The streams folder is renamed to thumbs on first startup.

### Changelog

- Feat: webradio favorites #502
- Feat: search radio-browser.info for webradios #502
- Feat: integration of my new webradioDB project #502
- Feat: list playlist contents from filesystem
- Feat: support of extended m3u playlistinfo #650
- Feat: support of mixrampdb
- Feat: add compile time options to enable IPv6
- Upd: improved image selects
- Upd: separate folders by image type in /pics directory
- Upd: improved jukebox code
- Upd: do not unnecessarily update cover images
- Upd: minimize work for counter refresh loop
- Upd: use sds and rax from my forks
- Upd: improved json encoding
- Upd: improved usage of sds functions

***

## myMPD v9.0.4 (2022-01-17)

This is a small bugfix release.

### Changelog

- Fix: removed unused PUID/PGUID from docker documentation
- Fix: chown workdir to mympd user on each startup #659
- Fix: add -Wno-stringop-overflow to sds compile settings #652

***

## myMPD v9.0.3 (2022-01-05)

This is a small bugfix release.

### Changelog

- Fix: parsing of lrc files with windows line endings #651
- Fix: add missing de-DE phrases

***

## myMPD v9.0.2 (2021-12-25)

This is a small bugfix release.

### Changelog

- Fix: click in queue view for small displays #648
- Fix: click on song tags in playback cards #649

***

## myMPD v9.0.1 (2021-12-21)

This release fixes some packaging issues.

### Changelog

- Fix: debian package dependency #645
- Fix: build script #643
- Fix: Gentoo ebuild #646

***

## myMPD v9.0.0 (2021-12-17)

This release upgrades Bootstrap to the new major release, brings many ui enhancements and adds support for new mpd 0.23 protocol features. With this release the major redesign of the myMPD code was final.

### Changelog

- Feat: reworked main menu
- Feat: reworked popover menu
- Feat: reworked pagination #
- Feat: priority handling for songs in queue
- Feat: harmonize popover menus #534
- Feat: songs, directories, albums and streams can be added to home screen #604
- Feat: add playlist, directory, search after current playing song (requires mpd 0.23.5) #579
- Feat: insert directory, search, album into playlist (requires mpd 0.23.5) #579
- Feat: integrate custom bootstrap and bootstrap.native build
- Feat: improved support for multiple tag values #583
- Feat: improve listviews for small displays #494
- Feat: support albumart for cuesheet virtual folders #578
- Feat: more granular timers #596
- Feat: reworked some form elements #530
- Feat: utf8 aware string handling
- Feat: quick actions for navbar icons
- Feat: replaced browser native local playback controls #634
- Feat: support build on Termux #612
- Upd: pressing pause button stops streams
- Upd: libmpdclient for mpd 0.23.x support
- Upd: improved notifications
- Upd: bootstrap 5.1.3
- Upd: removed default theme - dark is the new default
- Upd: improved json validation for settings
- Upd: improved handling for filenames with special chars #585
- Upd: removed obsolete keyboard navigation for tables and menus
- Upd: consistently use LastModified not Last-Modified
- Upd: improve mpd autodetection
- Upd: more accessible single-oneshot mode
- Upd: improve theme-autodetect
- Upd: improve jukebox mode
- Upd: migrate to pcre2 library #611
- Fix: check for fingerprint command #614
- Fix: use /var/cache/mympd as cachedir #620
- Fix: do not reset generate smartpls to default if empty #619
- Fix: correct API call for smartpls update #627
- Fix: sanitize filenames for generated smartpls files #629
- Fix: timer startplay action #625
- Fix: streamproxy connection handling #634 #618

***

## myMPD v8.1.6 (2021-11-13)

This is a small bugfix release.

### Changelog

- Fix: display correct jukebox unique tag #605
- Fix: keepalive timer for websocket #606

***

## myMPD v8.1.5 (2021-11-07)

This is a small bugfix release.

### Changelog

- Fix: accept Last-Modified as sort tag #601
- Fix: missing TRIGGER_MYMPD_START

***

## myMPD v8.1.4 (2021-11-01)

This is a small bugfix release.

### Changelog

- Fix: allow paths for playlists #588
- Fix: improve path traversal detection #585
- Fix: serviceworker behind reverse proxy (subdir) #586
- Fix: invalid jsonrpc call for mympd_api_raw #592
- Fix: return always complete result for mympd_api and mympd_api_raw
- Fix: web notifications broken #587

***

## myMPD v8.1.3 (2021-10-15)

This is a small bugfix release.

### Changelog

- Feat: add checks for compiler flags
- Upd: set Lua dependency to 5.4 for Alpine build
- Fix: compiler warning #577

***

## myMPD v8.1.2 (2021-10-08)

This is a small bug fix release.

### Changelog

- Upd: documentation for nginx as reverse proxy
- Upd: OpenSSL 3.0 compatibility
- Upd: add sds_urldecode unit test
- Fix: annoying js error in navbar event handler #574
- Fix: parser error in parsing internal timer script actions #575
- Fix: duplicate content in timer script list
- Fix: compile issues with Debian 9
- Fix: format string errors in log handling #576

***

## myMPD v8.1.1 (2021-10-01)

This is a small bugfix release.

### Changelog

- Upd: remove unnecessary utf8 unicode escape handling
- Upd: reject unicode escapes in json data
- Upd: remove recursion in jukebox
- Upd: enable more clang-tidy checks
- Upd: add more tests for validate_name
- Upd: mongoose to latest release
- Fix: repair enter action in advanced search inputs #567
- Fix: allow empty smartplsGenerateTagList #566
- Fix: respect browser locale #568
- Fix: parsing of timer minutes definition #569
- Fix: toggle outputs

***

## myMPD v8.1.0 (2021-09-26)

This release adds pin protection for all settings dialogs and enhances the validation of user input data.

### Changelog

- Feat: implement readcomments and list comments in song details modal
- Feat: option to clear current mpd error message
- Feat: settings can be secured with a pin #469
- Feat: improve validation of json input #468
- Feat: more security checks for the webserver
- Feat: clang compatibility #553
- Feat: add unit tests
- Upd: change license to GPLv3
- Upd: improve handling of empty smart playlists
- Upd: renamed some api methods
- Upd: rework C include strategy
- Upd: rearrange source tree
- Upd: improve cmake dependency messages
- Fix: respect connection: close header #551
- Fix: do not trust mime type from albumart tags

***

## myMPD v8.0.4 (2021-08-16)

This is a small bug fix release.

### Changelog

- Feat: OpenWrt package support, thanks to @tmn505, #547
- Fix: lyrics display in playback card #546
- Fix: ignore zero bytes albumart #550
- Fix: add null pointer check to converting server ip

***

## myMPD v8.0.3 (2021-08-07)

This is a small bug fix release.

### Changelog

- Feat: add color selection to home icon edit dialog #541
- Fix: light theme issues #540
- Fix: reset scrollpos #539

***

## myMPD v8.0.2 (2021-07-30)

This is a small bugfix release.

### Changelog

- Upd: bgcolor select for transparent home icons #535
- Upd: some code enhancements
- Fix: respect configured loglevel #532
- Fix: missing translation phrases #536
- Fix: html escaping issues
- Fix: set keepalive also for mpd_worker connection
- Fix: replace album grid elements only if changed

***

## myMPD v8.0.1 (2021-07-24)

This is a small bug fix release.

### Changelog

- Feat: add mpd keepalive support
- Upd: korean translation #525
- Fix: parse integer environment variables correctly #529
- Fix: MYMPD_LOGLEVEL environment always overrides configuration option as documented #529
- Fix: correct return code for mympd -c #529
- Fix: hard limit results to 1000 to keep webui responsive #528
- Fix: set interesting tags for mpd worker thread #524
- Fix: set libmpdclient buffer to 8192 bytes #524

***

## myMPD v8.0.0 (2021-07-16)

This major release improves the backend in many ways. The rework streamlines the backend, removes deprecated features, makes the configuration of myMPD easier and harmonizes the API.

### Note

This release changes the startup options of myMPD. Further the mympd.conf is no longer used. myMPD now saves all configuration values in state files. You can set some environment variables for initial startup settings like port, ssl, etc., afterwards you must edit the state files.

Smart playlists now uses search expressions and therefore are only supported for mpd >= 0.21.0.

The complete documentation was revised and is published here: https://jcorporation.github.io/myMPD/

### Removed features

- System commands
- Bookmarks
- Chroot and readonly support
- Option to disable coverimages
- Mixrampdb settings
- Scrobbler integration - would be replaced by a script in a future version
- Smart playlists for MPD < 0.21.0

### Changelog

- Feat: improve startup time
- Feat: improved backend #304
- Feat: rework settings dialog
- Feat: rework connection dialog
- Feat: some performance improvements
- Upd: remove deprecated features
- Upd: mongoose to current master
- Fix: do not use libmpdclient deprecated api functions
- Fix: bad homescreen link for filesystem browse #512
- Fix: pretty print duration after song has played in queue view #511
- Fix: respect command meta tag for keyboard shortcuts (mac) #507
- Fix: support pathnames with #-character #501
- Fix: reordering of songs in playlists #487
- Fix: mime type detection for png images #521

***

## myMPD v7.0.2 (2021-04-09)

This is a small bug fix release.

### Changelog

- Upd: remove sntp and mqtt support
- Fix: compile with disabled openssl
- Fix: hide popover if table row is dragged #464
- Fix: some small code improvements

***

## myMPD v7.0.1 (2021-04-02)

This is a small bug fix release.

### Changelog

- Upd: set api endpoint to /api/
- Fix: read dns server directly from resolv.conf (support of Alpine Linux) #461
- Fix: some warnings detected by code scanning tools

***

## myMPD v7.0.0 (2021-03-29)

This major release upgrades the integrated webserver to the mongoose 7 series and adds some http related features. The new reverse proxy integration replaces the workaround for local playback in HTTPS mode. Scripts can now make http requests. I used this feature to implement a simple scrobbler for ListenBrainz in only 20 lines of code. This script is called through an myMPD trigger.

### Notes

- This major release removes some deprecated features as announced in #356.
- This major release changes some web server related configuration options.
- myMPD now support only MPD 0.20.0 and above.

### Changelog

- Feat: builtin reverse proxy for mpd http stream
- Feat: new lua api call - mympd_api_http_client #395
- Feat: remember page offset and scroll position for filesystem browsing navigation #454
- Feat: add sample ListenBrainz scrobbling script
- Feat: limit concurrent http connections to 100
- Upd: mongoose 7.3 #388
- Fix: error handler for local playback
- Fix: albumcache is not created at startup #451
- Fix: reading triggers at startup
- Fix: save of one shot timers

### Removed features

- Webdav support - mongoose 7 does not support webdav anymore
- Configurable mpd stream uri - local playback feature now uses the integrated reverse proxy for streaming

***

## myMPD v6.12.1 (2021-03-18)

This is a small maintenance release. myMPD supports now the additional tags introduced in the actual mpd master branch. A lot of work has gone into the improved support for id3v2 SYLT tags. The parser now fully supports utf16le, utf16be, utf8 and latin encodings.

### Changelog

- Feat: support tags "ComposerSort", "Ensemble", "Movement", "MovementNumber", "Location" #445
- Upd: korean translations, thanks to @parkmino #441
- Upd: add confirmation dialog before deleting a script #442
- Upd: improve confirmation dialogs
- Upd: improve display of synced lyrics #448
- Fix: some code cleanups
- Fix: rename config.c and config.h, to avoid collision with libmpdclient #443

***

## myMPD v6.12.0 (2021-03-08)

This release revises the myMPD settings dialog. The queue and jukebox settings are now accessible directly from the playback and queue view and footer (if enabled).

The notifications are also improved and can be configured in the advanced settings dialog. The jukebox album mode got many fixes and small enhancements.

You can now view sticker values in all song lists, not only in the song details dialog. Stickers can be enabled in the column settings.

Also the support of lyrics is enhanced. myMPD now parses SYLT tags in id3v2 tagged files correctly and shows all synced and unsynced of a song.

### Note

Since this version the source does not include prebuild assets. If you do not use the provided build scripts, you must build the assets before with `./build.sh createassets`.

### Changelog

- Feat: revised settings
- Feat: show stickers in song views #413
- Feat: improved notifications #355
- Feat: one shot timer #417
- Feat: static background image #416
- Feat: check for server version vs javascript cached version
- Feat: use albumcache for jukebox album mode #436
- Feat: syslog support #432
- Upd: simplify build script
- Upd: integrate eslint in build.sh script
- Upd: integrate stylelint in build.sh script
- Upd: integrate htmlhint in build.sh script
- Upd: many small javascript enhancements and code deduplication
- Upd: some css enhancements
- Upd: return synced and unsynced lyrics
- Fix: correct parsing of SYLT id3v2 tag #437
- Fix: minimize API calls on startup
- Fix: remove javascript debug statements for release code
- Fix: display jukebox queue list, if jukebox mode is album #436

***

## myMPD v6.11.3 (2021-02-18)

This is a small bug fix release.

### Changelog

- Fix: case insensitive ligature search
- Fix: goto filtered album view
- Fix: edit search expression
- Fix: respect column order for queue update #429

***

## myMPD v6.11.2 (2021-02-12)

This is a small bugfix release.

### Changelog

- Feat: add starts_with to song search
- Feat: add option to select playback controls in settings #426 #382
- Upd: set any as default tag for browse albums view
- Fix: do not show mpd playlists in filesystem root folder
- Fix: build with disabled libid3tag and flac
- Fix: rpm, alpine and gentoo packaging
- Fix: seek backward
- Fix: set fixed width for play counter in footer #424

***

## myMPD v6.11.1 (2021-02-05)

This release fixes some bugs, enhances the build script and packaging and fixes 32bit compile time errors.

### Changelog

- Feat: integrate debian cross compile scripts in build.sh
- Feat: add any to search in album view
- Feat: add man pages #418
- Upd: remove java build dependency
- Upd: latest bootstrap.native
- Fix: packaging dependencies
- Fix: improve failure handling in the build script
- Fix: albumgrid - do not discard album if sort tag is null #419
- Fix: albumgrid - build cache dependencies #420
- Fix: 32bit build for arm - time_t format errors
- Fix: lintian errors and warnings #418
- Fix: null pointer assertion in mpd_client_get_updatedb_state #411
- Fix: disable scripting if lua is not compiled in

***

## myMPD v6.11.0 (2021-01-30)

This is a small release that improves the overall usability of myMPD. The biggest change is the new backend for the albumgrid. It should be faster and displays now also incomplete albums without track#1 or disc#1.

myMPD now supports right click and long touch to open context menus and you can set some default actions in the advanced settings.

### Changelog

- Feat: show incomplete albums in albumgrid #398
- Feat: support right click and long touch for contextmenus #406
- Feat: customizable default actions for views for left click/touch #406
- Feat: add ligature select dropdown #384
- Feat: support binarylimit command #409
- Feat: support getvol command of mpd 0.23.0
- Upd: Bootstrap 4.6.0
- Upd: libmympdclient 1.0.5
- Upd: german and korean translations #412
- Fix: setting pagination to all #405
- Fix: getting album covers from mpd #408
- Fix: theme selection - setting background color #407
- Fix: use full uri for covercache

***

## myMPD v6.10.0 (2021-01-09)

This myMPD version ships many small enhancements and some bugfixes. One of the biggest new feature is the support of synced lyrics.

### Changelog

- Feat: support synchronized lyrics #262, [Documentation](https://github.com/jcorporation/myMPD/wiki/Lyrics)
- Feat: improve pagination #346
- Feat: add italian translation, thanks @roberto
- Feat: show queue and playlists length and total play time in table footer
- Feat: add html5 color picker for color inputs
- Upd: korean translation #401, thanks @parkmino
- Fix: respect supplementary groups for mympd user #399
- Fix: filter out empty albums / artists in database view #379
- Fix: correctly handle special characters for mpd search expressions #400
- Fix: memory leak in filesystem listing

***

## myMPD v6.9.1 (2020-12-26)

This is mostly a maintenance release.

### Changelog

- Feat: set next playing song in queue #391
- Fix: some pagination issues #390
- Fix: getting lyrics from file

***

## myMPD v6.9.0 (2020-12-26)

This minor release fixes many small bugs and adds support for multiple USLT tags.

### Changelog

- Feat: support for multiple USLT tags #258
- Feat: config option to add stop button #382
- Fix: better previous button behavior #382
- Fix: browse filesystem - reset search on directory change #385
- Fix: album action in search view #381
- Fix: set selected album in grid view #386

***

## myMPD v6.8.3 (2020-12-06)

This release introduces the brand new myMPD logo and adds Last-Modified column to search results.

### Changelog

- Feat: new myMPD logo
- Feat: add Last-Modified to sort result columns
- Fix: sorting of search results
- Fix: transparently fallback from AlbumArtist to Artist tag

***

## myMPD v6.8.2 (2020-11-29)

This point release fixes some bugs and enhances the build script.

### Changelog

- Feat: limit the range of allowed volume #375
- Feat: improve behavior of queue crop or clear button
- Fix: use combination of AlbumArtist + Album as Album identifier, not the containing folder
- Fix: disable database view if mpd version < 0.21.0 is detected #364, #373, #374
- Fix: template coverimage not found #365
- Fix: layout and picture issues #370, #368, #372
- Fix: do not log initial state of websocket #371
- Fix: add checks for malloc errors

***

## myMPD v6.8.1 (2020-11-23)

This point release fixes some bugs and adds one minor feature.

### Changelog

- Feat: add direct play button to album grid view #359
- Upd: korean translation #361
- Fix: do not ask for script argument if no argument is defined #362
- Fix: missing translations in home icon edit modal #363
- Fix: set correct sort tag if tags are disabled #360
- Fix: home screen icon with search #357

***

## myMPD v6.8.0 (2020-11-19)

This release adds a brand new home screen. You can customize it with shortcuts to special views (e. g. new albums), playlists and scripts.
Further new features are the support for multiple tag values, multidisc albums and better support of pictures and booklets. The old filter letters are gone and replaced with a search bar in all views.

### Changelog

- Feat: home screen #348
- Feat: advanced search for album grid #337
- Feat: improve display of multidisc albums (respect disc tag) #220
- Feat: support multiple tag values #220
- Feat: replace filter letters with search in filesystem and playlist views
- Feat: sort filesystem and playlists views
- Feat: show extra pictures and booklet in album view, filesystem and playback view
- Feat: docker images based on Alpine for aarch64 and amd64 platforms, thanks to @niawag #333
- Feat: view jukebox queue
- Feat: customizeable navbar icons #352
- Feat: add fr-FR translation, thanks to @niawag #353
- Upd: KO translation, thanks to @parkmino #341
- Upd: NL translation, thanks to @pinkdotnl #349
- Fix: setting new mpd host if mpd is disconnected
- Fix: uninitialized variables detected by valgrind
- Fix: reset progressbar to zero for streams #340
- Fix: JavaScript linter warnings

***

## myMPD v6.7.0 (2020-10-18)

For this myMPD version the GUI was modernized. The cards and tabs are removed and all views now using the maximal width. The top and the bottom bars are also revised. The navigation bar is located at the left or the top depending on the orientation of the device. At the bottom there is a new playbar with control buttons and playback information.

Furthermore the covergrid and the database tabs are merged together. Now all tags can be viewed as a grid. To use that new feature, create a directory with the tagtype (e.g. Artist) in the /var/lib/mympd/pics directory and add images withe the tag values.

***

## myMPD v6.6.2 (2020-08-13)

This is a small maintenance release.

### Changelog

- Upd: mympd_api and mympd_api_raw returning now 2 values (errorcode and string)
- Upd: add gcc compile option -Wvla
- Upd: improved PWA support
- Fix: sysVinit startup script #325
- Fix: improve build.sh uninstall function #324

***

## myMPD v6.6.1 (2020-08-31)

This is a small maintenance release.

### Changelog

- Feat: remove path and querystring from uris displayed as title
- Upd: korean translation #306
- Upd: minor layout improvements
- Fix: overflow of card elements in chrome based browsers #321
- Fix: syscmd not expandable if mpd not connected #322
- Fix: handling of uri encoded characters in browse database card

***

## myMPD v6.6.0 (2020-08-28)

This release improves the scripting capabilities of myMPD further. Triggers can now be defined to execute scripts on events, e.g. song change.
I am not so familiar with Lua, but for my new project myMPDos I want write some scripts to enhance myMPD. The system commands feature is now deprecated and will be removed in one of the next versions.

The second new feature is the beginning support for MPD partitions (usable only with MPD 0.22.x). Now you can fully manage this partitions. In the next release the support will be enhanced (#304).

But in this release there are also many small improvements and bugfixes.

### Changelog

- Feat: support mpd partitions (mpd 0.22.x) #216
- Feat: add triggers for scripts #290
- Feat: editable output attributes
- Upd: improved Alpine packaging
- Upd: improved logging
- Upd: NL translation #303
- Upd: KO translation #306
- Upd: Bootstrap 4.5.2
- Fix: jukebox maintaining to many songs
- Fix: Lua 5.4 compatibility
- Fix: set content security policy frame-ancestors to * #307
- Fix: loading of lua libraries #310
- Fix: duplication of scripts (renaming) #309
- Fix: better hyphenation behavior #279
- Fix: urlencoding of foldernames #313
- Fix: calculate MPD stream url correctly - avoids mixed-content #311
- Fix: local playback error handling #317
- Fix: improve certificate handling and lower lifetime of server certificate #319
- Fix: remove "Local playback" -> Autoplay option -> modern browsers are preventing this to work

***

## myMPD v6.5.2 (2020-07-20)

This is only a small maintenance release.

### Changelog

- Upd: korean translation
- Fix: Debian packaging #301 #300 #299
- Fix: reset offset after queue is cleared #302
- Fix: jukebox adding to many songs if queue length > jukebox minimum queue length

***

## myMPD v6.5.1 (2020-07-11)

This is only a small maintenance release with some minor features.

#### Changelog

- Feat: build and packaging improvements
- Feat: improvements of playback card layout
- Feat: add config option to disable redirect from http to https
- Feat: add menu option and keymap to go in fullscreen mode
- Upd: korean translation
- Fix: segfault after external script execution
- Fix: parsing of mympd_api arguments in lua scripts

***

## myMPD v6.5.0 (2020-07-06)

This minor versions adds a new mpd_worker thread for heavy background tasks to prevent web ui lags. It also improves smart playlist handling and the jukebox mode.

The new scripting feature makes myMPD even more flexible. Scripts can be executed manual, through timers or by the mympd-script cli tool. You can use the full power of Lua combined with direct access to the myMPD API to automate some tasks. For more details goto the scripting page in the myMPD wiki.

### Changelog

- Feat: add scripting feature #261
- Feat: add new mpd_worker thread for heavy background tasks (smart playlist and sticker cache generation) #274
- Feat: update smart playlists only on demand #282
- Feat: prevent jukebox starving condition from uniqueness parameter #273
- Feat: IP ACL support
- Feat: viewport scale setting for mobile browsers
- Feat: IPv6 support
- Upd: update to latest libmpdclient release
- Upd: improve logging and notifications
- Upd: improve sticker handling
- Upd: update bootstrap to 4.5.0 #276
- Upd: update bootstrap.native to 3.0.6 #275

***

## myMPD v6.4.2 (2020-06-08)

This point release fixes some small bugs.

### Changelog

- Fix: mime-type detection is now case insensitive (extension) #277
- Fix: repair use after free bug in manual creation of CA and certificates #278
- Fix: change hyphenation behavior to break-word #279
- Upd: update frozen to current master
- Upd: update inih to current master #281

***

## myMPD v6.4.1 (2020-05-31)

This is a small maintenance release that fixes many bugs.

### Changelog

- Fix: support of Last-Modified sort in covergrid
- Fix: improve Gentoo ebuild, thanks to @itspec-ru #266
- Fix: correct RPM spec file changelog
- Fix: update nl-NL translation, thanks to @pinkdotnl #270
- Fix: update mongoose to 6.18 #272
- Fix: repair links in playback card #271
- Fix: limit jukebox add songs attempts to two #269
- Fix: better handling of playlists select boxes #269
- Fix: use correct field values in saved searches #269
- Fix: clear jukebox queue if triggered by timer #269
- Fix: don't set media session position state if position > duration (streams) #268

***

## myMPD v6.4.0 (2020-05-20)

This minor release fixes some bugs, enhances the build.sh check command and adds an option to sort the albumart grid by modification time. This release also adds better support for lyrics and coverimages. The default pseudo random number generator was replaced with TinyMT a Mersene Twister implementation.

### Changelog

- Feat: add Last-Modified sort option to covergrid #220
- Feat: add feature detection for mount and neighbors #246
- Feat: integrate clang-tidy in build.sh check function
- Feat: use *Sort tags for sorting #247
- Feat: use TinyMT to generate random numbers #249
- Feat: support of embedded lyrics in id3 and vorbis comments #251
- Feat: display lyrics in playback card #250
- Fix: improve handling of the pictures tab in the song details modal
- Fix: add missing weekday check for timer activation
- Fix: fix warnings reported by clang-tidy and eslint
- Fix: add all to queue fails #252
- Fix: compilation issue with gcc 10

***

## myMPD v.6.3.1 (2020-04-26)

This release fixes one ugly and security related bug:

### Changelog

- Fix: repair use after free bug in function mpd_client_last_skipped_song_uri

***

## myMPD v6.3.0 (2020-04-12)

This release adds support for the MPD mount and neighbor functions. The error handling for MPD protocol errors was improved. This is the first version that can only compiled with internal libmpdclient (called libmympdclient).

### Changelog

- Feat: support mounts and neighbors #147
- Feat: remove option to compile with external libmpdclient
- Feat: improve notifications
- Fix: improve dutch translation
- Fix: jukebox song selection from whole database #239
- Fix: improve MPD error handling #244

***

### myMPD v6.2.3 (2020-03-06)

This release fixes a ugly bug, that prevents adding new songs to the last played list and increases the song playCount endless.

***

## myMPD v6.2.2 (2020-03-04)

myMPD 6.2.2 fixes some bugs and adds a Dutch translation, thanks to Pinkdotnl for that.

### Changelog

- Feat: add link to browse to main menu #228
- Feat: adding Dutch translations #233
- Fix: reconnect to mpd on error 5 (Broken pipe)
- Fix: fix build on RPi zeros/ARMv6 #235
- Fix: MPD < 0.20.0 don't support jukebox song select from database #231

***

## myMPD v6.2.1 (202-02-26)

This is a small maintenance release.

### Changelog

- Feat: update korean translation #221
- Feat: create default mympd.conf through mympd-config utility #224
- Fix: don't include i18n.js from debug builds in release files
- Fix: some layout and theme polishing

***

## myMPD v6.2.0 (2020-02-14)

myMPD 6.2.0 adds more functionality to smart playlists and playlists generally. The publishing feature of myMPD was completely reworked and supports now webdav to manage pics, mpd music_directory and playlists. This feature is in the default config disabled and must be enabled in mympd.conf. Also the notification system was reworked and supports now the brand new HTML5 MediaSession API.

Please give the new tool mympd-config a chance. This tool parses your mpd.conf and generates a suitable mympd.conf. For details look at https://github.com/jcorporation/myMPD/wiki/mympd-config.

### Changelog

- Feat: improve (smart) playlists #165
  - smart playlist generation rule
  - sort or shuffle playlists
  - bulk deletion of playlists
- Feat: publish library, playlists and pics through http and webdav #161
- Feat: support booklets, lyrics and more pictures in song details modal #160
- Feat: support MediaSession Web API #201
- Feat: new notification and status area in bottom right corner #200
- Feat: add last_modified tag in playback card and song details modal #220
- Feat: add command line option to dump default config
- Feat: configurable highlight color
- Feat: improve settings dialog
- Feat: support replay gain mode auto
- Feat: improve console logging
- Feat: update mongoose to version 6.17
- Fix: compatibility with mpd 0.20.x
- Fix: layout of quick playback settings #218
- Fix: support of Web Notification API
- Fix: code improvements to remove errors caused by stricter eslint configuration
- Fix: rescan and update database functions respect path parameter
- Fix: repair some timer issues #225
- Fix: jukebox unpauses randomly MPD #227
- Fix: improve MPD error handling

***

## myMPD v6.1.0 (2020-01-27)

myMPD v6.1.0 adds a new timer function. You can now define multiple timers to play, stop or execute a system command. The new timer function is also internally used for covercache maintenance and building smart playlists.

The jukebox was completely rewritten for better performance. Now the jukebox maintain a separate queue from that are songs added to the MPD queue. This queue is dynamically created and respects user defined constraints as unique tags or song last played older than 24 hours (playback statistics must be enabled).

### Changelog

- Feat: add new timer module #163
- Feat: jukebox enhancements #164
- Feat: add quick playback options in playback card #200
- Feat: support MPD single oneshot mode #209
- Feat: update embedded libmpdclient to latest master
- Fix: respect websocket state "connecting"
- Fix: many small theme and layout enhancements
- Fix: enabling bookmarks don't overwrite bookmark list
- Fix: repair add buttons in search card
- Fix: improve json encoding
- Fix: improve mpd error handling

***

## myMPD v6.0.1 (2019-12-21)

myMPD v6.0.1 fixes some small issues.

### Changelog

- Fix: disable covergrid, if MPD version is older than 0.21.x #208
- Fix: disable song details modal if playing a stream #206
- Fix: update korean translation #205

***

## myMPD v6.0.0 (2019-12-19)

This release improves mainly the support for albumart and embeds an enhanced version of libmpdclient. You can now browse and search the mpd database in a album focused covergrid mode.

The c++ plugin with the dependency to libmediainfo was replaced with the c libraries libid3tag und libflac to extract embedded albumart in mp3, flac and ogg files.

### Changelog

- Feat: covergrid tab in browse card #162
- Feat: theming support; default,dark and light theme
- Feat: support more file extensions for streaming coverimages
- Feat: try covercache before extracting embedded coverimage or asking mpd for the cover
- Feat: support of mpd albumart command (mpd 0.21.x) #145
- Feat: support of mpd readpicture command (mpd 0.22.x) #145
- Feat: embedded libmpdclient (libmympdclient branch in my fork) #145
- Feat: covercache maintenance options
- Feat: replace libmediainfo with libid3tag and libflac #145
- Feat: a list of possible coverimage names can now be defined
- Feat: set cache header for coverimages
- Feat: improved build script and packaging
- Feat: update bootstrap to 4.4.1
- Fix: set correct websocket connection status
- Fix: some memory leaks in error conditions
- Fix: some small layout issues

***

### myMPD v5.7.2 (2019-11-26)

This is a small maintenance release.

### Changelog

- Fix: update korean translation #192
- Fix: small code improvements for string handling
- Fix: albumart size is not changing #193

***

## myMPD v5.7.1 (2019-11-18)

This is a small maintenance release.

## Changelog

- Feat: display fileformat, filetype and duration in playback card #180
- Feat: display filetype in songdetails dialog #180
- Feat: add configurable step for volume change, defaults to 5%
- Feat: reload and Clear function in advanced settings
- Fix: close http connection after error response
- Fix: replace references of old coverimage templates
- Fix: increase modal width on medium sized devices

***

## myMPD v5.7.0 (2019-11-12)

myMPD v5.7.0 is a big maintenance release. There are only some new minor features, but many code improvements. The integration of the simple dynamic string library prevents most potential buffer truncation or buffer overflow bugs and makes the code simpler. Thanks to code linting tools (flawfinder, eslint, ...) and memory checkers like libasan and valgrind many bugs could be solved. The API follows now the JSON-RPC 2.0 specification and was tested with a simple testsuite and a fuzzer.

myMPD v5.7.0 should be the most stable and secure release.

### Changelog

- Feat: integrate simple dynamic string library
- Feat: migrate API to JSON-RPC 2.0
- Feat: readonly mode support
- Feat: split javascript in smaller files
- Feat: new config option to enable/disable bookmarks
- Feat: validate saved columns
- Feat: translations phrases are extracted from source at compile time
- Feat: add simple JSON-RPC 2.0 API fuzzer to testsuite (uses https://github.com/minimaxir/big-list-of-naughty-strings)
- Feat: add SPDX-License-Identifier
- Feat: optimize minimize and compression functions in build script
- Feat: IPv6 support for self created myMPD certificate
- Fix: add missing translations
- Fix: javascript code uses now === and !== operators
- Fix: warnings from code linting tools (cppcheck, flawfinder, shellcheck, eslint)
- Fix: don't use deprecated gethostbyname
- Fix: keyboard shortcuts that call the api directly

***

## myMPD v5.6.2 (2019-10-10)

This is a small maintenance release. The complete javascript code is now linted with eslint and deepscan.io.

### Changelog

- Feat: beautify song details modal
- Feat: add flawfinder check to build script
- Feat: add eslint configuration file
- Feat: add .gitignore file
- Fix: update mongoose to current version #173
- Fix: update inih to current version #173
- Fix: some javascript and css bugfixes #170

***

## myMPD 5.6.1 (2019-09-16)

This point release fixes some issues and adds minor enhancements.

### Changelog

- Feat: chroot support
- Feat: build.sh improvements #154
  - optionally sign arch linux package
  - optionally sign debian package
  - add option to use osc (open build service)
  - detect change of assets
- Fix: update ko-kr translation #156
- Fix: better calculation of install directories #155
- Fix: invalid combined javascript file, if java is not found at build time #157

***

## myMPD v5.6.0 (2019-09-14)

WARNING: there are possible breaking changes in this release

- default config file is now /etc/mympd.conf (build.sh moves the old config to the new place)
- system commands are now defined in a section in /etc/mympd.conf
- systemd unit is installed in /lib/systemd/system (build.sh removes the old file)
- directories /usr/lib/mympd and /etc/mympd can be safely removed (build.sh does this in the install step)

### New features

This minor release supports now table and popup menu navigation through the keyboard. Supports the new mpd fingerprint command in the song details modal and displays now the current song title in the header.

### Changelog

- Feat: table navigation mode, see https://github.com/jcorporation/myMPD/wiki/Keyboard-Shortcuts for details
- Feat: support mpd fingerprint command #146
- Feat: display current song title in header bar
- Feat: new "update_lastplayed" event
- Feat: LSB-compliant startup script
- Feat: create needed /var/lib/mympd/ directories on startup
- Feat: create ssl certificates on startup
- Feat: command line options
- Feat: central build script
  - can now package for Alpine, Arch, Debian, RPM and Docker
- Feat: embedded document root for release build
  - gzip compressed files
- Feat: new splash screen
- Fix: better table formatting #143 #148
- Fix: close popup menus if associated row is replaced
- Fix: prettier keyboard shortcuts help
- Fix: more resource friendly last_played implementation
- Fix: cleanup the source tree
- Fix: move default configuration to /etc/mympd.conf
- Fix: define system commands in mympd.conf
- Fix: don't remove mympd user/group and /var/lib/mympd directory on package removal
- Fix: move logic from postinstall scripts to cmake

***

## myMPD v5.5.3 (2019-08-20)

This point release fixes some minor issues and adds small enhancements.

### Changelog

- Feat: add more secure headers to http responses
- Feat: enable directory listing for /library #140
- Fix: purging the debian package #141
- Fix: small UI enhancements #139
- Fix: sub-menu for system commands #123

***

## myMPD v5.5.2 (2019-07-15)

This point release fixes some minor issues.

### Changelog

- Fix: polish ko-kr translation
- Fix: cards don't truncate popovers anymore
- Fix: sync with latest bootstrap.native master

***

## myMPD v5.5.1 (2019-07-01)

This point release adds a ABUILD file for alpine linux and fixes some minor issues.

### Changelog

- Feat: packaging for alpine
- Fix: polish ko-kr translation
- Fix: determine default /var/lib/ path from compile time settings
- Fix: do not create /usr/share/mympd/lib directory

***

## myMPD v5.5.0 (2019-06-25)

This new release adds a tiny translation framework to myMPD. The translation is implemented only on client side and inspired by polyglot.js. The default language is en-US. For now myMPD ships a german and a korean translation, further translations are very welcome.

### Changelog

- Feat: tiny translation framework #108
  - German translation
  - Korean translation
- Feat: gitlab CI/CD integration #133
- Feat: PKDBUILD follows Arch Linux web application packaging guidelines #131
- Fix: updated bootstrap.native to current master
- Fix: updated Mongoose to 6.15
- Fix: update inih to version 44
- Fix: update frozen to current master

***

## myMPD v5.4.0 (2019-06-03)

The biggest changes in this release are the new settings and connection dialogue. Now are the most myMPD settings configurable in the web gui. All settings in the configuration file or through environment variables are overwritten with these settings.

The jukebox mode and play statistics are improved and the coverimages now fade-in smoothly.

### Changelog

- Feat: redesigned settings dialogue with many new options
- Feat: mpd connection dialogue
- Feat: new sticker lastSkipped
- Feat: integrate shortcuts help in about dialogue
- Feat: improve song skip detection
- Feat: improve play statistics
- Feat: improve jukebox mode - add new song 10 + crossfade seconds before current song ends
- Feat: manually add random songs or albums to queue
- Feat: add option to update smart playlist in contextmenu
- Feat: locale setting (used only to display datetime fields for now)
- Feat: redesigned local playback feature, added autoplay option
- Feat: smooth transition of coverimage in background and playback card
- Feat: add content-security-policy header
- Feat: use loglevel in javascript code
- Fix: replace template coverimages with a svg version
- Fix: compile coverextract plugin with older c++ standards
- Fix: html cleanups
- Fix: posix compliant shell scripts

***

## myMPD 5.3.1 (2019-04-28)

This point release fixes some bugs.

### Changelog

- Fix: allow same characters in playlistnames as mpd itself #111
- Fix: debian packaging #113
- Fix: docker build

***

## myMPD v5.3.0 (2019-04-25)

This versions optimizes the backend code even more and introduces some minor features.

### Changelog

- Feat: coverimage as background #99
- Feat: support hosting myMDP behind a reverse proxy under a subdir, #93
- Feat: split parsing of settings in myMPD settings and mpd options, #105
- Fix: subscribe only to interesting mpd idle events
- Fix: various code improvements
- Fix: better tmp file handling
- Fix: return only requested tags in lists
- Fix: update docker file for coverextract plugin compilations, #109
- Fix: create certificates under /var/lib/mympd/ssl
- Fix: support install prefix other than /usr #112

***

## myMPD v5.2.1 (2019-04-01)

This point release fixes some bugs.

### Changelog

- Don't run syscmds twice #102
- Support filenames with special characters for embedded albumcovers #62

***

## myMPD v5.2.0 (2019-03-25)

myMPD 5.2.0 supports embedded albumart through a plugin, that uses libmediainfo. It became a plugin because I did not want to introduce any more dependencies. The plugin is build automatically, if libmediainfo is found. The plugin must be enabled in the configuration file.

Another nice feature detects the mpd music_directory automatically, if myMPD connects over a local socket to mpd. If not, you must configure the musicdirectory option in /etc/mympd/mympd.conf. The symlinks within htdocs are no longer needed.

The last feature enables myMPD to send a love song message to a running scrobbling client through the mpd client protocol.

### Changelog

- Feat: get mpd music_directory automatically or through config file
- Feat: support embedded albumart #62
- Feat: love button for external scrobbling clients
- Fix: song change in streams #97
- Fix: update mongoose to 6.14
- Fix: create state files with defaults at runtime, not at install time, fixes #96
- Fix: serve /library and /pics directories directly (replaces symlinks in htdocs)
- Fix: exclude /ws, /library, /pics, /api from service worker fetch
- Fix: simplified conn_id tracking in webserver
- Fix: improved logging

***

## myMPD v5.1.0 (2019-02-23)

This is a small maintenance release.

### Changelog

- Feat: bookmarks for directories #86
- Fix: compile against musl #92
- Fix: update Bootstrap to 4.3.1
- Fix: various code cleanups

***

## myMPD v5.0.0 (2019-02-11)

This new myMPD major version brings only a few new features, but the backend code has been completely rewritten. The web server and the mpd client code have been swapped out into their own threads, which communicate with each other with an asynchronous message queue. This provides the necessary flexibility to enhance existing features and implement new ones.

Many thanks to rollenwiese for his work on the docker support for myMPD.

The release code is now compiled with -fstack-protector -D_FORTIFY_SOURCE=2 -pie for security reasons. myMPD 5 is the first release, that was completely checked with the valingrid memchecker.

WARNING: This release has a new configuration file syntax. You should copy the mympd.conf.dist file over your configuration and customize it.

### Changelog

- Feat: docker support (experimental)
- Feat: read environment variables (overwrites configuration options)
- Feat: AutoPlay - add song to (empty) queue and mpd starts playing
- Feat: new webui startup modal
- Feat: separate threads for backend and frontend functions
- Upd: better handling of mpd connection errors
- Fix: many security and memory leak fixes
- Fix: remove global states
- Fix: compiler warnings

***

## myMPD v4.7.2 (2018-12-24)

This is a small maintenance release.

### Changelog

- Feat: configurable coverimage size #78
- Fix: remove original port for redirect uri, append ssl port to uri, if not 443

***

## myMPD v4.7.1 (2018-12-06)

This is only a small bugfix release.

### Changelog

- Feat: use tagtypes command to disable unused tags
- Fix: no sort and group tags in advanced search, if tags are disabled
- Fix: seek in progress bar

***

## myMPD v4.7.0 (2018-12-03)

This release supports the new filter expressions introduced in MPD 0.21. Simply press Enter in the search input to add more than one search expression.

The advanced search needs libmpdclient 2.17 and falls back to simple search, if older versions of mpd or libmpdclient are installed.

### Changelog

- Feat: sanitize user input in backend
- Feat: advanced search with sorting
- Feat: add logging configuration option
- Fix: improved smart playlists
- Fix: smart playlist save dialog
- Fix: include patched bootstrap-native-v4.js
- Fix: much more granular progress bar in playback card
- Fix: honour sslport configuration option
- Fix: show warnings for unknown configuration options
- Fix: improved api error handling

***

## myMPD v4.6.0 (2018-11-15)

This release introduce a last played tab in the queue card, improves the overall usability and last but not least fixes some bugs.

### Changelog

- Feat: last played songs view
- Feat: configurable tags in playback card
- Feat: song voting and goto browse action in song details modal
- Feat: more keyboard shortcuts #58
- Fix: song voting
- Fix: song numbering in queue view #74
- Fix: update mongoose to 6.13

***

## myMPD v4.5.1 (2018-11-05)

This point release fixes only one, but ugly bug.

### Changelog

- Fix: myMPD crashes if mpd plays a stream #72

***

## myMPD v4.5.0 (2018-11-04)

WARNING: This release has new and incompatible configuration options. You should copy the mympd.conf.dist file over your configuration and customize it.

This release uses detection of mpd features and many new configuration options to let you customize myMPD to your needs. myMPD now supports all tags you enabled in mpd and works also with no metadata enabled. Other highlights are the ability to add/hide/sort columns of all tables and the possibility to define system commands.

### Changelog

- Feat: add settings for coverimages, localplayer, streamurl, searchtaglist, browsetaglist, syscmds #70
- Feat: disable playlists and browse database feature if not enabled in mpd #68 #69
- Feat: configureable columns in all song views #47
- Feat: reworked view of albums
- Feat: add ability to define and execute system commands, e.g. reboot and shutdown
- Fix: use AlbumArtist tag only if it found in enabled tags #69
- Fix: link uri in song details only if mpd music_directory is linked
- Fix: hidding of popover menus
- Fix: some memory free errors

***

## myMPD v4.4.0 (2018-10-15)

### Changelog

- Feat: collapse title list in album view
- Feat: display total entities in list headers #47
- Feat: configurable max_elements_per_page #47
- Feat: confirm dialog for delete playlist #47
- Feat: lazy loading of coverimages in album view
- Fix: deleting track from playlist don't deletes playlist anymore
- Fix: use update_stored_playlist notify for updating playlist view

***

## myMPD v4.3.1 (2018-10-10)

### Changelog

- Feat: add more keyboard shortcuts #58
- Feat: performance improvements for jukebox mode
- Feat: performance improvements in smart playlists creation, fixes #64
- Feat: support all mpd tags
- Fix: check existence of needed directories under /var/lib/mympd
- Fix: install default state files under /var/lib/mympd/state

***

## myMPD v4.3.0 (2018-10-01)

This myMPD release improves the jukebox mode and adds the smart playlist feature.
Installing this release resets all myMPD state settings.

### Changelog

- Feat: smart playlists #38
- Feat: improve jukebox mode #57
- Feat: improve state store
- Fix: filename check in save dialogs #61
- Fix: encoding of special characters in popover #60
- Fix: popover eventhandling
- Fix: calculate correct websocket url if connected by ip

***

## myMPD v4.2.1 (2018-09-24)

This is mainly a bugfix release.

### Changelog

- Feat: add rescan database command
- Fix: don't use serviceworker for http:// uris -> fixes http stream in local player
- Fix: show lastPlayed in song details
- Fix: moved pics directory to /var/lib/mympd
- Fix packaging issues
  - Fix: improve uninstall scripts
  - Fix: don't remove user/group on uninstall
  - Fix: test of /var/lib/mympd ownership

***

## myMPD v4.2.0 (2018-09-20)

myMPD 4.2.0 fixes some bugs and includes the new jukebox mode.

In jukebox mode, myMPD adds random songs from database or selected playlist to the queue, if it's empty. To use jukebox mode enable it in settings and enable Consume.

### Changelog

- Feat: jukebox mode #37
- Feat: configuration option for used tags in ui #52
- Fix: don't add redundant eventhandler on popovers
- Fix: don't hide "connection error"
- Fix: check if document root exists
- Fix: allow websocket connections only to /ws
- Fix: improve packaging
- Fix: filter empty tag values in browse database install and uninstall

***

## myMPD v4.1.2 (2018-09-16)

This release fixes some packaging issues.

### Changelog

- Fix: Archlinux PKGBUILD checksums and install script #45
- Fix: Debian packaging #54

***

## myMPD v4.1.1 (2018-09-13)

This is only a small bugfix release. It also fixes a security issue in mongoose.

### Changelog

- Feat: add PKGBUILD for Archlinux
- Fix: base64 encode JSON.stringify output, if saved in dom - should fix bug #50
- Fix: update mongoose to 6.12
- Fix: add mympd user & group as system user & group #51

***

## myMPD v4.1.0 (2018-09-11)

This minor release supports more tags for search and database browse mode. myMPD now runs under a dedicated user.

### Changelog

- Feat: packaging for Fedora, Suse and Debian #45
- Feat: central backend for search functions #36
- Feat: album actions in menu in search card #42
- Feat: mpd feature detection #44
- Feat: check supported mpd tag types #44
- Feat: browse and search database by more tags #43 #36
- Feat: link album in playback card
- Feat: run myMPD under myMPD user
- Fix: use mpd taglist in songdetails modal
- Fix: setgroups before setuid

***

## myMPD v4.0.0 (2018-08-27)

The fourth major release of myMPD. Now myMPD don't poll mpd every second anymore. myMPD instead uses the idle protocol to listening for status changes. This should be much more resource friendly.

### Changelog

- Feat: replace status polling with mpd idle protocol #33
- Feat: song voting and play statistics (uses mpd stickers) #32
- Feat: support covers for http streams
- Feat: improved layout of playback card
- Feat: handle database update events #31
- Fix: cleanup api #34
- Fix: cleanup logging
- Fix: cleanup source files
- Fix: improve speed for listing large queues #35
- Fix: link to music_directory
- Fix: redirect to https with request host header #30

***

## myMPD v3.5.0 (2018-08-09)

WARNING: This version breaks all command line options. Use /etc/mympd/mympd.conf for configuration.
myMPD now setuids to user nobody in default configuration.

### Changelog

- Feat: get outputnames and outputstates in single command
- Feat: clear playback card if songpos = -1
- Fix: formating of source code

***

## myMPD v3.4.0 (2018-08-06)

Now are playlists fully supported.

### Changelog

- Feat: option to create playlist in "Add to playlist" dialog
- Feat: "add all to playlist" in search card
- Feat: "add all to playlist" in browse filesystem card
- Feat: add stream to playlist
- Feat: hide pagination if not needed
- Feat: crop queue
- Feat: link to songdetails and albumlist in playback card
- Fix: move 3rd-party sources and buildtools to dist directory
- Fix: delete song from playlist

***

## myMPD v3.3.0 (2018-07-30)

### Changelog

- Feat: add playlist actions
- Feat: validation feedback for queue save
- Feat: validation feedback for add stream
- Feat: enable queue and playlist sorting with drag & drop
- Fix: mkrelease.sh
- Fix: html markup
- Fix: renamed card "Now playing" to "Playback"
- Fix: service worker cache update

***

## myMPD v3.2.1 (2018-07-19)

This is mainly a bugfix release.

### Changelog

- Feat: use javascript in strict mode
- Upd: error handling code for ajax requests
- Upd: better error handling for unknown requests
- Upd: enabled ssl by default
- Fix: fixed some javascript errors (issue #22)

***

## myMPD v3.2.0 (2018-07-16)

WARNING: This release has new and incompatible command line options.

### Changelog

- Feat: enable Progressive Web App and Add2HomeScreen Feature
- Feat: enable ssl options
- Feat: contrib/crcert.sh script for creating certificates automatically
- Upd: use /etc/mympd/ directory for options
- Fix: many cleanups and small bug fixes

***

## myMPD v3.1.1 (2018-07-09)

### Changelog

- Feat: add songdetails to actions popover
- Feat: central tag handling in backend
- Fix: alignment of icons
- Fix: columns in database view

***

## myMPD v3.1.0 (2018-07-05)

For this minor release, much of the javascript code is rewritten.

### Changelog

- Feat: add first advanced actions
- Feat: removed jQuery in favour of bootstrap.native
- Feat: removed Bootstrap plugins and replaced with native implementations
- Fix: stability fixes in json parsing (backend)

***

## myMPD v3.0.1 (2018-06-24)

### Changelog

- Fix: javascript error in about dialog

***

## myMPD v3.0.0 (2018-06-24)

This is the first release with a completely rewritten backend and the new jsonrpc api.

### Changelog

- Feat: upgraded mongoose to latest version
- Feat: implemented jsonrpc api for request from frontend to backend
- Feat: realtime notifications over websocket
- Feat: minified .js and .css files
- Feat: mkrelease.sh and mkdebug.sh scripts for simple compile and install
- Feat: save myMPD settings in /var/lib/mympd/mympd.state (removed cookie usage)
- Feat: backend now handles cover images
- Fix: layout fixes

***

## myMPD v2.3.0 (2018-06-17)

### Changelog

- Feat: replace sammy.js with own implementation with state save for cards, tabs and views
- Feat: use queue version for song change in http streams

***

## myMPD v2.2.1 (2018-06-10)

This is a small bugfix release.

### Changelog

- Feat: added stop button #14
- Fix: material-icons in chrome #13
- Fix: html markup errors

***

## myMPD v2.2.0 (2018-06-07)

This is my third release of myMPD.

### Changelog

- Feat: reworked browse view
  - Browse database -> Albumartist -> Album
  - playlist view
  - serverside filtering of tables
- Feat: new action "Add all from search"
- Feat: incremental change of tables
- Feat: improved settings
- Feat: improved coverimage display
- Fix: some bug fixes

Many thanks to archphile for heavily testing this release.

***

## myMPD v2.1.0 (2018-05-28)

This is my second release of myMPD.

### Changelog

- Feat: reworked queue View
  - pagination in header
  - search in queue
  - playing song info
- Feat: mpd statistics in About dialog
- Feat: configurable cover image filename
- Fix: some minor bug fixes

***

## myMPD v2.0.0 (2018-05-24)

Initial release for my ympd fork myMPD.

### Changelog

- Feat: new modern ui based on Bootstrap4
- Feat: updated javascript libraries
- Feat: album cover support
- Fix: song title refresh for http streams
- Fix: removed dirble support
