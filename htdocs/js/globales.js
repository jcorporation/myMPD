"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module globales_js */

/** @type {number} */
const startTime = Date.now();

// generate uniq id for this browser session
/** @type {number} */
const jsonrpcClientIdMin = 100000;
const jsonrpcClientIdMax = 999999;
const jsonrpcClientId = Math.floor(Math.random() * (jsonrpcClientIdMax - jsonrpcClientIdMin + 1) + jsonrpcClientIdMin);
let jsonrpcRequestId = 0;

const jsonRpcError = {
    "jsonrpc": "2.0",
    "id": 0,
    "error": {
        "method": "",
        "facility": "general",
        "severity": "error",
        "message": "",
        "data": {}
    }
};

let socket = null;

let websocketKeepAliveTimer = null;
let websocketLastPong = null;
let searchTimer = null;
let resizeTimer = null;
let progressTimer = null;

/** @type {number} */
const searchTimerTimeout = 500;

/** @type {object} */
let currentSongObj = {};

/** @type {object} */
let currentState = {};

/** @type {object} */
let settings = {
    /** @type {number} */
    "loglevel": 2,
    "partition": {}
};

/** @type {boolean} */
let myMPDready = false;

/** @type {boolean} */
let appInited = false;

/** @type {boolean} */
let scriptsInited = false;

/** @type {boolean} */
let uiEnabled = true;

/** @type {string} */
let settingsParsed = 'no';

// Reference to dom node for drag & drop
/** @type {EventTarget} */
let dragEl = undefined;

/** @type {boolean} */
let showSyncedLyrics = false;

/** @type {boolean} */
let scrollSyncedLyrics = true;

/** @type {string} */
const subdir = window.location.pathname.replace('/index.html', '').replace(/\/$/, '');

/** @type {object} */
const ligatures = {
    'checked': 'task_alt',
    'more': 'menu',
    'unchecked': 'radio_button_unchecked',
    'partitionSpecific': 'dashboard',
    'browserSpecific': 'web_asset',
    'stared': 'star',
    'star': 'star_border'
};

/** @type {Array} */
const widgetRefresh = [];

// pre-generated elements
/** @type {object} */
const pEl = {};

/** @type {string} */
const smallSpace = '\u2009';

/** @type {string} */
const nDash = '\u2013';

/** @type {string} */
let tagAlbumArtist = 'AlbumArtist';

/** @type {object} */
const albumFilters = [
    'AlbumArtist',
    'Composer',
    'Performer',
    'Conductor',
    'Ensemble'
];

/** @type {object} */
const webradioFields = [
    'StreamUri',
    'Homepage',
    'Genres',
    'Country',
    'Region',
    'Languages',
    'Codec',
    'Bitrate',
    'Description',
    'Added',
    'Last-Modified'
];

/** @type {object} */
const session = {
    "token": "",
    "timeout": 0
};

/** @type {number} */
const sessionLifetime = 1780;

/** @type {number} */
const sessionRenewInterval = sessionLifetime * 500;

let sessionTimer = null;

/** log message buffer */
const messages = [];
/** @type {number} */
const messagesMax = 100;

/** @type {boolean} */
const debugMode = document.querySelector("script").src.replace(/^.*[/]/, '') === 'combined.js' ? false : true;

/** @type {string} */
const scriptsUri = 'https://github.com/jcorporation/mympd-scripts/tree/main/';
/** @type {string} */
const scriptsImportUri = 'https://raw.githubusercontent.com/jcorporation/mympd-scripts/main/';

/** @type {object} */
const imageExtensions = ['webp', 'png', 'jpg', 'jpeg', 'svg', 'avif'];

/** @type {string} */
let locale = navigator.language || navigator.userLanguage;

const localeMap = {
    'de': 'de-DE',
    'es': 'es-ES',
    'fi': 'fi-FI',
    'fr': 'fr-FR',
    'it': 'it-IT',
    'ja': 'ja-JP',
    'ko': 'ko-KR',
    'nl': 'nl-NL',
    'zh': 'zh-Hans',
    'zh-CN': 'zh-Hans',
    'zh-TW': 'zh-Hant'
};

/** @type {object} */
let materialIcons = {};
/** @type {object} */
let phrasesDefault = {};
/** @type {object} */
let phrases = {};

/** @type {number} */
let lastSeekStep = 10;

/** @type {number} */
const maxElementsPerPage = 1000;

/**
 * This settings are saved in the browsers localStorage
 */
const settingsLocalFields = {
    "localPlaybackAutoplay": {
        "defaultValue": false,
        "inputType": "checkbox",
        "title": "Autoplay",
        "form": "modalSettingsLocalPlaybackCollapse",
        "help": "helpSettingsLocalPlaybackAutoplay",
        "hintIcon": ligatures['browserSpecific'],
        "hintText": "Browser specific setting"
    },
    "partition": {
        "defaultValue": "default",
        "inputType": "none"
    },
    "scaleRatio": {
        "defaultValue": "1.0",
        "inputType": "text",
        "title": "Scale ratio",
        "form": "modalSettingsThemeFrm2",
        "hintIcon": ligatures['browserSpecific'],
        "hintText": "Browser specific setting",
        "cssClass": ["featMobile"],
        "validate": {
            "cmd": "validateFloatEl",
            "options": []
        },
        "invalid": "Invalid scale ratio"
    },
    "viewMode": {
        "defaultValue": "auto",
        "validValues": {
            "auto": "Autodetect",
            "mobile": "Mobile",
            "desktop": "Desktop"
        },
        "inputType": "select",
        "title": "View mode",
        "form": "modalSettingsThemeFrm2",
        "help": "helpSettingsViewMode",
        "hintIcon": ligatures['browserSpecific'],
        "hintText": "Browser specific setting",
        "sort": 1
    }
};

const localSettings = {
    /** @type {string} */
    "viewMode": settingsLocalFields.viewMode.defaultValue,
    /** @type {boolean} */
    "localPlaybackAutoplay": settingsLocalFields.localPlaybackAutoplay.defaultValue,
    /** @type {string} */
    "partition": settingsLocalFields.partition.defaultValue,
    /** @type {string} */
    "scaleRatio": settingsLocalFields.scaleRatio.defaultValue
};

// partition specific settings
const settingsPartitionFields = {
    "mpdStreamPort": {
        "defaultValue": defaults["PARTITION_MPD_STREAM_PORT"],
        "inputType": "text",
        "contentType": "number",
        "title": "Stream port",
        "form": "modalSettingsLocalPlaybackCollapse",
        "help": "helpSettingsStreamPort",
        "hintIcon": ligatures['partitionSpecific'],
        "hintText": "Partition specific setting"
    },
    "streamUri": {
        "defaultValue": defaults["PARTITION_MPD_STREAM_URI"],
        "placeholder": "auto",
        "inputType": "text",
        "title": "Stream URI",
        "form": "modalSettingsLocalPlaybackCollapse",
        "help": "helpSettingsStreamUri",
        "hintIcon": ligatures['partitionSpecific'],
        "hintText": "Partition specific setting"
    },
    "highlightColor": {
        "defaultValue": defaults["PARTITION_HIGHLIGHT_COLOR"],
        "inputType": "color",
        "title": "Highlight color",
        "form": "modalSettingsThemeFrm1",
        "hintIcon": ligatures['partitionSpecific'],
        "hintText": "Partition specific setting",
        "sort": 3
    },
    "highlightColorContrast": {
        "defaultValue": defaults["PARTITION_HIGHLIGHT_COLOR_CONTRAST"],
        "inputType": "color",
        "title": "Highlight contrast color",
        "form": "modalSettingsThemeFrm1",
        "hintIcon": ligatures['partitionSpecific'],
        "hintText": "Partition specific setting",
        "sort":4
    }
};

// global settings
const settingsFields = {
    "volumeMin": {
        "defaultValue": defaults["MYMPD_VOLUME_MIN"],
        "inputType": "text",
        "contentType": "number",
        "title": "Volume min.",
        "form": "modalSettingsVolumeFrm",
        "invalid": "Must be a number"
    },
    "volumeMax": {
        "defaultValue": defaults["MYMPD_VOLUME_MAX"],
        "inputType": "text",
        "contentType": "number",
        "title": "Volume max.",
        "form": "modalSettingsVolumeFrm",
        "invalid": "Must be a number"
    },
    "volumeStep": {
        "defaultValue": defaults["MYMPD_VOLUME_STEP"],
        "inputType": "text",
        "contentType": "number",
        "title": "Volume step",
        "form": "modalSettingsVolumeFrm",
        "invalid": "Must be a number"
    },
    "lyricsUsltExt": {
        "defaultValue": defaults["MYMPD_LYRICS_USLT_EXT"],
        "inputType": "text",
        "title": "Unsynced lyrics extension",
        "form": "modalSettingsLyricsCollapse",
        "help": "helpSettingsUsltExt"
    },
    "lyricsSyltExt": {
        "defaultValue": defaults["MYMPD_LYRICS_SYLT_EXT"],
        "inputType": "text",
        "title": "Synced lyrics extension",
        "form": "modalSettingsLyricsCollapse",
        "help": "helpSettingsSyltExt"
    },
    "lyricsVorbisUslt": {
        "defaultValue": defaults["LYRICS"],
        "inputType": "text",
        "title": "Unsynced lyrics vorbis comment",
        "form": "modalSettingsLyricsCollapse",
        "help": "helpSettingsVorbisUslt"
    },
    "lyricsVorbisSylt": {
        "defaultValue": defaults["SYNCEDLYRICS"],
        "inputType": "text",
        "title": "Synced lyrics vorbis comment",
        "form": "modalSettingsLyricsCollapse",
        "help": "helpSettingsVorbisSylt"
    },
    "lastPlayedCount": {
        "defaultValue": defaults["MYMPD_LAST_PLAYED_COUNT"],
        "inputType": "text",
        "contentType": "number",
        "title": "Last played list count",
        "form": "modalSettingsStatisticsFrm",
        "help": "helpSettingsLastPlayedCount"
    },
    "bookletName": {
        "defaultValue": defaults["MYMPD_BOOKLET_NAME"],
        "inputType": "text",
        "title": "Booklet filename",
        "form": "modalSettingsAlbumInfoFrm",
        "help": "helpSettingsBookletName"
    },
    "infoTxtName": {
        "defaultValue": defaults["MYMPD_INFO_TXT_NAME"],
        "inputType": "text",
        "title": "Info filename",
        "form": "modalSettingsAlbumInfoFrm",
        "help": "helpSettingsInfoTxtName"
    },
    "coverimageNames": {
        "defaultValue": defaults["MYMPD_COVERIMAGE_NAMES"],
        "inputType": "text",
        "title": "Filenames",
        "form": "modalSettingsAlbumartFrm",
        "help": "helpSettingsCoverimageNames",
        "cssClass": ["featLibrary"]
    },
    "thumbnailNames": {
        "defaultValue": defaults["MYMPD_THUMBNAIL_NAMES"],
        "inputType": "text",
        "title": "Thumbnail names",
        "form": "modalSettingsAlbumartFrm",
        "help": "helpSettingsThumbnailNames",
        "cssClass": ["featLibrary"],
    },
    "smartpls": {
        "defaultValue": defaults["MYMPD_SMARTPLS"],
        "inputType": "checkbox"
    },
    "smartplsPrefix": {
        "defaultValue": defaults["MYMPD_SMARTPLS_PREFIX"],
        "inputType": "text",
        "title": "Smart playlists prefix",
        "form": "modalSettingsSmartplsFrm",
        "help": "helpSettingsSmartplsPrefix"
    },
    "smartplsSort": {
        "defaultValue": "",
        "inputType": "select",
        "title": "Order",
        "form": "modalSettingsSmartplsFrm",
        "help": "helpSettingsSmartplsSort",
    },
    "smartplsInterval": {
        "defaultValue": defaults["MYMPD_SMARTPLS_INTERVAL_HOURS"],
        "inputType": "text",
        "contentType": "number",
        "title": "Update interval",
        "unit": "Hours",
        "form": "modalSettingsSmartplsFrm",
        "help": "helpSettingsSmartplsInterval"
    },
    "tagDiscEmptyIsFirst": {
        "defaultValue": defaults["MYMPD_TAG_DISC_EMPTY_IS_FIRST"],
        "inputType": "checkbox",
        "title": "Enforce disc tag",
        "form": "modalSettingsTagsFrm",
        "help": "helpSettingsTagDiscEmptyIsFirst"
    }
};

// webui specific settings
const settingsWebuiFields = {
    "clickSong": {
        "defaultValue": "append",
        "validValues": {
            "append": "Append to queue",
            "appendPlay": "Append to queue and play",
            "insertAfterCurrent": "Insert after current playing song",
            "replace": "Replace queue",
            "replacePlay": "Replace queue and play",
            "view": "Song details",
            "context": "Context menu"
        },
        "inputType": "select",
        "title": "Click song",
        "form": "modalSettingsDefaultActionsFrm"
    },
    "clickWebradiodb": {
        "defaultValue": "append",
        "validValues": {
            "append": "Append to queue",
            "appendPlay": "Append to queue and play",
            "insertAfterCurrent": "Insert after current playing song",
            "replace": "Replace queue",
            "replacePlay": "Replace queue and play",
            "add": "Add to favorites",
            "view": "Webradio details",
            "context": "Context menu"
        },
        "inputType": "select",
        "title": "Click webradio",
        "form": "modalSettingsDefaultActionsFrm"
    },
    "clickRadioFavorites": {
        "defaultValue": "append",
        "validValues": {
            "append": "Append to queue",
            "appendPlay": "Append to queue and play",
            "insertAfterCurrent": "Insert after current playing song",
            "replace": "Replace queue",
            "replacePlay": "Replace queue and play",
            "edit": "Edit webradio favorite",
            "context": "Context menu"
        },
        "inputType": "select",
        "title": "Click webradio favorite",
        "form": "modalSettingsDefaultActionsFrm"
    },
    "clickQueueSong": {
        "defaultValue": "play",
        "validValues": {
            "play": "Play",
            "view": "Song details",
            "context": "Context menu"
        },
        "inputType": "select",
        "title": "Click song in queue",
        "form": "modalSettingsDefaultActionsFrm"
    },
    "clickPlaylist": {
        "defaultValue": "append",
        "validValues": {
            "append": "Append to queue",
            "appendPlay": "Append to queue and play",
            "insertAfterCurrent": "Insert after current playing song",
            "replace": "Replace queue",
            "replacePlay": "Replace queue and play",
            "view": "View playlist",
            "context": "Context menu"
        },
        "inputType": "select",
        "title": "Click playlist",
        "form": "modalSettingsDefaultActionsFrm"
    },
    "clickFilesystemPlaylist": {
        "defaultValue": "view",
        "validValues": {
            "append": "Append to queue",
            "appendPlay": "Append to queue and play",
            "insertAfterCurrent": "Insert after current playing song",
            "replace": "Replace queue",
            "replacePlay": "Replace queue and play",
            "view": "View playlist",
            "context": "Context menu"
        },
        "inputType": "select",
        "title": "Click filesystem playlist",
        "form": "modalSettingsDefaultActionsFrm"
    },
    "clickQuickPlay": {
        "defaultValue": "replacePlay",
        "validValues": {
            "append": "Append to queue",
            "appendPlay": "Append to queue and play",
            "insertAfterCurrent": "Insert after current playing song",
            "replace": "Replace queue",
            "replacePlay": "Replace queue and play"
        },
        "inputType": "select",
        "title": "Click quick play button",
        "form": "modalSettingsDefaultActionsFrm"
    },
    "notificationPlayer": {
        "defaultValue": false,
        "inputType": "checkbox",
        "title": "Playback",
        "form": "modalSettingsFacilitiesFrm",
        "help": "helpSettingsNotificationPlayer"
    },
    "notificationQueue": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Queue",
        "form": "modalSettingsFacilitiesFrm",
        "help": "helpSettingsNotificationQueue"
    },
    "notificationGeneral": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "General",
        "form": "modalSettingsFacilitiesFrm",
        "help": "helpSettingsNotificationGeneral"
    },
    "notificationDatabase": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Database",
        "form": "modalSettingsFacilitiesFrm",
        "help": "helpSettingsNotificationDatabase"
    },
    "notificationPlaylist": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Playlist",
        "form": "modalSettingsFacilitiesFrm",
        "help": "helpSettingsNotificationPlaylist"
    },
    "notificationScript": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Script",
        "form": "modalSettingsFacilitiesFrm",
        "help": "helpSettingsNotificationScript"
    },
    "notifyPage": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "On page notifications",
        "form": "modalSettingsNotificationsFrm",
        "help": "helpSettingsNotifyPage"
    },
    "notifyWeb": {
        "defaultValue": false,
        "inputType": "checkbox",
        "title": "Web notifications",
        "form": "modalSettingsNotificationsFrm",
        "onClick": "toggleBtnNotifyWeb",
        "help": "helpSettingsNotifyWeb"
    },
    "mediaSession": {
        "defaultValue": false,
        "inputType": "checkbox",
        "title": "Media session",
        "form": "modalSettingsNotificationsFrm",
        "warn": "Browser has no MediaSession support",
        "help": "helpSettingsMediaSession"
    },
    "footerPlaybackControls": {
        "defaultValue": "pause",
        "validValues": {
            "pause": "pause only",
            "stop": "stop only",
            "both": "pause and stop"
        },
        "inputType": "select",
        "title": "Playback controls",
        "form": "modalSettingsFooterFrm",
        "sort": 0
    },
    "footerSettingsPlayback": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Playback settings",
        "form": "modalSettingsFooterFrm",
        "sort": 2
    },
    "footerPlaybackControlsPopover": {
        "defaultValue": false,
        "inputType": "checkbox",
        "title": "Advanced playback controls",
        "form": "modalSettingsFooterFrm",
        "sort": 1
    },
    "footerSeek": {
        "defaultValue": false,
        "inputType": "checkbox",
        "title": "Seek",
        "form": "modalSettingsFooterFrm",
        "sort": 1
    },
    "footerAudioFormat": {
        "defaultValue": false,
        "inputType": "checkbox",
        "title": "AudioFormat",
        "form": "modalSettingsFooterFrm",
        "sort": 1
    },
    "footerVolumeLevel": {
        "defaultValue": false,
        "inputType": "checkbox",
        "title": "Volume level",
        "form": "modalSettingsFooterFrm",
        "sort": 3
    },
    "footerNotifications": {
        "defaultValue": false,
        "inputType": "checkbox",
        "title": "Notification icon",
        "form": "modalSettingsFooterFrm",
        "sort": 4
    },
    "showHelp": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Show help",
        "form": "modalSettingsThemeFrm3",
        "help": "helpSettingsHelp"
    },
    "maxElementsPerPage": {
        "defaultValue": 100,
        "validValues": {
            "25": 25,
            "50": 50,
            "100": 100,
            "250": 250,
            "500": 500,
            "1000": 1000
        },
        "inputType": "select",
        "contentType": "number",
        "title": "Elements per page",
        "form": "modalSettingsListsFrm",
        "help": "helpSettingsMaxElementsPerPage"
    },
    "smallWidthTagRows": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Display tags in rows for small displays",
        "form": "modalSettingsListsFrm",
        "help": "helpSettingsSmallWidthTagRows"
    },
    "quickPlayButton": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Quick play button",
        "form": "modalSettingsListsFrm",
        "help": "helpSettingsQuickPlay"
    },
    "quickRemoveButton": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Quick remove button",
        "form": "modalSettingsListsFrm",
        "help": "helpSettingsQuickRemove"
    },
    "compactGrids": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Compact grids",
        "form": "modalSettingsListsFrm",
        "help": "helpSettingsCompactGrids"
    },
    "viewTitles": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Show view titles",
        "form": "modalSettingsListsFrm",
        "help": "helpSettingsViewTitles"
    },
    "showBackButton": {
        "defaultValue": false,
        "inputType": "checkbox",
        "title": "History back button",
        "form": "modalSettingsNavigationBarFrm",
        "help": "helpSettingsBackButton"
    },
    "enableHome": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Homescreen",
        "form": "modalSettingsFurtherFeaturesFrm",
        "help": "helpSettingsEnableHome"
    },
    "enableScripting": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Scripting",
        "form": "modalSettingsFurtherFeaturesFrm",
        "warn": "Lua is not compiled in",
        "help": "helpSettingsEnableScripting"
    },
    "enableTimer": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Timer",
        "form": "modalSettingsFurtherFeaturesFrm",
        "help": "helpSettingsEnableTimer"
    },
    "enableMounts": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Mounts",
        "form": "modalSettingsFurtherFeaturesFrm",
        "warn": "MPD does not support mounts",
        "help": "helpSettingsEnableMounts"
    },
    "enableLocalPlayback": {
        "defaultValue": false,
        "inputType": "checkbox",
    },
    "enablePartitions": {
        "defaultValue": false,
        "inputType": "checkbox",
        "title": "Partitions",
        "form": "modalSettingsFurtherFeaturesFrm",
        "warn": "MPD does not support partitions",
        "help": "helpSettingsEnablePartitions"
    },
    "enableLyrics": {
        "defaultValue": true,
        "inputType": "checkbox",
    },
    "theme": {
        "defaultValue": "dark",
        "validValues": {
            "auto": "Autodetect",
            "dark": "Dark",
            "light": "Light"
        },
        "inputType": "select",
        "title": "Theme",
        "form": "modalSettingsThemeFrm1",
        "onChange": "eventChangeTheme",
        "sort": 0
    },
    "gridSize": {
        "defaultValue": 175,
        "inputType": "text",
        "contentType": "number",
        "title": "Grid size",
        "form": "modalSettingsListsFrm",
        "invalid": "Must be a number and greater than zero",
        "validate": {
            "cmd": "validateUintEl",
            "options": []
        },
        "unit": "Pixel",
        "help": "helpSettingsGridSize"
    },
    "bgCover": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Albumart",
        "form": "modalSettingsBgFrm",
        "sort": 0
    },
    "bgCssFilter": {
        "defaultValue": "grayscale(100%) opacity(20%)",
        "inputType": "text",
        "title": "CSS filter",
        "form": "modalSettingsBgFrm",
        "sort": 1
    },
    "bgColor": {
        "defaultValue": "#060708",
        "inputType": "color",
        "title": "Color",
        "form": "modalSettingsBgFrm",
        "reset": true,
        "sort": 2,
        "validate": {
            "cmd": "validateColorEl",
            "options": []
        },
        "invalid": "Must be a hex color value"
    },
    "bgImage": {
        "defaultValue": "",
        "inputType": "mympd-select-search",
        "readOnly": true,
        "cbCallback": "filterImageSelect",
        "cbCallbackOptions": ["modalSettingsBgImageInput"],
        "title": "Image",
        "form": "modalSettingsBgFrm",
        "sort": 3,
        "validate": {
            "cmd": "validatePlistEl",
            "options": []
        },
        "invalid": "Must be a valid filename"
    },
    "locale": {
        "defaultValue": "default",
        "inputType": "select",
        "title": "Locale",
        "form": "modalSettingsLocaleFrm",
        "onChange": "eventChangeLocale"
    },
    "startupView": {
        "defaultValue": "Home",
        "validValues": {
            "Home": "Home",
            "Playback": "Playback",
            "Queue/Current": "Queue",
            "Queue/LastPlayed": "LastPlayed",
            "Queue/Jukebox": "Jukebox Queue",
            "Browse/Database": "Database",
            "Browse/Playlists": "Playlists",
            "Browse/Filesystem": "Filesystem",
            "Browse/Radio": "Webradios",
            "Search": "Search"
        },
        "inputType": "select",
        "title": "Startup view",
        "form": "modalSettingsStartupFrm",
        "onChange": "eventChangeTheme"
    },
    "musicbrainzLinks": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Show MusicBrainz links",
        "form": "modalSettingsTagsFrm",
        "help": "helpSettingsMusicBrainzLinks"
    },
    "outputLigatures": {
        "defaultValue": {
            "default": "speaker",
            "fifo": "read_more",
            "httpd": "stream",
            "null": "check_box_outline_blank",
            "pipe": "terminal",
            "recorder": "voicemail",
            "shout": "cast",
            "snapcast": "hub"
        },
        "inputType": "none"
    },
    "seekStep": {
        "defaultValue": 10,
        "inputType": "text",
        "contentType": "number",
        "title": "Seek step",
        "unit": "Seconds",
        "form": "modalSettingsSeekFrm",
        "validate": {
            "cmd": "validateUintEl",
            "options": []
        },
        "invalid": "Must be a number"
    },
    "feedback": {
        "defaultValue": "like",
        "inputType": "none"
    },
    "browseDatabaseAlbumListSort": {
        "defaultValue": tagAlbumArtist,
        "validValues": {},
        "inputType": "select",
        "title": "Album list sort",
        "form": "modalSettingsSortFrm",
        "help": "helpSettingsAlbumListSort"
    },
    "showWorkTagAlbumDetail": {
        "defaultValue": false,
        "inputType": "checkbox",
        "title": "Show work in album detail",
        "form": "modalSettingsTagsFrm",
        "help": "helpSettingsShowWorkTagAlbumDetail"
    }
};

const settingsConnectionFields = {
    "mpdHost": {
        "defaultValue": defaults["MYMPD_MPD_HOST"],
        "inputType": "text",
        "title": "MPD host",
        "form": "modalConnectionFrm",
        "help": "helpConnectionMPDHost",
        "class": ["alwaysEnabled"]
    },
    "mpdPort": {
        "defaultValue": defaults["MYMPD_MPD_PORT"],
        "inputType": "text",
        "contentType": "number",
        "title": "MPD port",
        "form": "modalConnectionFrm",
        "help": "helpConnectionMPDPort",
        "class": ["alwaysEnabled"]
    },
    "mpdPass": {
        "defaultValue": defaults["MYMPD_MPD_PASS"],
        "inputType": "password",
        "title": "MPD password",
        "form": "modalConnectionAdvFrm1",
        "help": "helpConnectionMPDPassword",
        "class": ["alwaysEnabled"]
    },
    "mpdTimeout": {
        "defaultValue": defaults["MYMPD_MPD_TIMEOUT_SEC"],
        "inputType": "text",
        "title": "Timeout",
        "form": "modalConnectionAdvFrm2",
        "help": "helpConnectionTimeout",
        "unit": "Seconds",
        "class": ["alwaysEnabled"]
    },
    "mpdKeepalive": {
        "defaultValue": defaults["MYMPD_MPD_KEEPALIVE"],
        "inputType": "checkbox",
        "title": "Keepalive",
        "form": "modalConnectionAdvFrm2",
        "help": "helpConnectionKeepalive",
        "class": ["alwaysEnabled"]
    },
    "mpdBinarylimit": {
        "defaultValue": defaults["MYMPD_MPD_BINARYLIMIT"] / 1024,
        "inputType": "text",
        "title": "Binary limit",
        "form": "modalConnectionAdvFrm2",
        "help": "helpConnectionBinaryLimit",
        "unit": "kB",
        "class": ["alwaysEnabled"]
    },
    "stickerdbMpdHost": {
        "defaultValue": defaults["MYMPD_MPD_HOST"],
        "inputType": "text",
        "title": "MPD host",
        "form": "modalConnectionStickerdbFrm",
        "help": "helpConnectionMPDHost",
        "class": ["alwaysEnabled"]
    },
    "stickerdbMpdPort": {
        "defaultValue": defaults["MYMPD_MPD_PORT"],
        "inputType": "text",
        "contentType": "number",
        "title": "MPD port",
        "form": "modalConnectionStickerdbFrm",
        "help": "helpConnectionMPDPort",
        "class": ["alwaysEnabled"]
    },
    "stickerdbMpdPass": {
        "defaultValue": defaults["MYMPD_MPD_PASS"],
        "inputType": "password",
        "title": "MPD password",
        "form": "modalConnectionStickerdbFrm",
        "help": "helpConnectionMPDPassword",
        "class": ["alwaysEnabled"]
    },
    "stickerdbMpdTimeout": {
        "defaultValue": defaults["MYMPD_MPD_TIMEOUT_SEC"],
        "inputType": "text",
        "title": "Timeout",
        "form": "modalConnectionStickerdbFrm",
        "help": "helpConnectionTimeout",
        "unit": "Seconds",
        "class": ["alwaysEnabled"]
    },
    "stickerdbMpdKeepalive": {
        "defaultValue": defaults["MYMPD_MPD_KEEPALIVE"],
        "inputType": "checkbox",
        "title": "Keepalive",
        "form": "modalConnectionStickerdbFrm",
        "help": "helpConnectionKeepalive",
        "class": ["alwaysEnabled"]
    },
};

const settingsPlaybackFields = {
    "random": {
        "inputType": "checkbox",
        "title": "Random",
        "form": "modalPlaybackPlaybackFrm1",
        "help": "helpQueueRandom"
    },
    "repeat": {
        "inputType": "checkbox",
        "title": "Repeat",
        "form": "modalPlaybackPlaybackFrm1",
        "help": "helpQueueRepeat"
    },
    "autoPlay": {
        "inputType": "checkbox",
        "title": "Autoplay",
        "form": "modalPlaybackPlaybackFrm1",
        "help": "helpQueueAutoPlay"
    },
    "crossfade": {
        "defaultValue": 0,
        "inputType": "text",
        "contentType": "number",
        "title": "Crossfade",
        "form": "modalPlaybackPlaybackFrm2",
        "help": "helpQueueCrossfade",
        "unit": "Seconds"
    },
    "mixrampDb": {
        "defaultValue": 0,
        "inputType": "text",
        "contentType": "number",
        "title": "Mixramp db",
        "form": "modalPlaybackPlaybackFrm2",
        "help": "helpQueueMixrampDb",
        "unit": "DB"
    },
    "mixrampDelay": {
        "defaultValue": -1,
        "inputType": "text",
        "contentType": "number",
        "title": "Mixramp delay",
        "form": "modalPlaybackPlaybackFrm2",
        "help": "helpQueueMixrampDelay",
        "unit": "Seconds"
    },
    "jukeboxPlaylist": {
        "inputType": "mympd-select-search",
        "defaultValue": defaults["MYMPD_JUKEBOX_PLAYLIST"],
        "readOnly": true,
        "cbCallback": "filterPlaylistsSelect",
        "cbCallbackOptions": [0, 'selectJukeboxPlaylist'],
        "title": "Playlist",
        "form": "modalPlaybackJukeboxCollapse",
        "help": "helpJukeboxPlaylist",
        "class": ["jukeboxSongOnly"]
    },
    "jukeboxQueueLength": {
        "inputType": "text",
        "defaultValue": defaults["MYMPD_JUKEBOX_QUEUE_LENGTH"],
        "contentType": "number",
        "title": "Keep queue length",
        "form": "modalPlaybackJukeboxCollapse",
        "help": "helpJukeboxQueueLength",
        "class": ["jukeboxSongOnly"]
    },
    "jukeboxUniqTag": {
        "inputType": "select",
        "defaultValue": defaults["MYMPD_JUKEBOX_UNIQ_TAG"],
        "title": "Enforce uniqueness",
        "form": "modalPlaybackJukeboxCollapse",
        "help": "helpJukeboxUniqueTag"
    },
    "jukeboxLastPlayed": {
        "inputType": "text",
        "contentType": "number",
        "defaultValue": defaults["MYMPD_JUKEBOX_LAST_PLAYED"],
        "title": "Song was played last",
        "form": "modalPlaybackJukeboxCollapse",
        "help": "helpJukeboxLastPlayed",
        "unit": "Hours ago",
        "class": ["featAdvAlbum"]
    },
    "jukeboxIgnoreHated": {
        "inputType": "checkbox",
        "defaultValue": defaults["MYMPD_JUKEBOX_IGNORE_HATED"],
        "title": "Ignore hated songs",
        "form": "modalPlaybackJukeboxCollapse",
        "help": "helpJukeboxIgnoreHated",
        "class": ["jukeboxSongOnly"]
    },
    "jukeboxMinSongDuration": {
        "inputType": "text",
        "contentType": "number",
        "defaultValue": defaults["MYMPD_JUKEBOX_MIN_SONG_DURATION"],
        "title": "Min. song duration",
        "form": "modalPlaybackJukeboxCollapse",
        "help": "helpJukeboxMinSongDuration",
        "class": ["jukeboxSongOnly"],
        "unit": "Seconds"
    },
    "jukeboxMaxSongDuration": {
        "inputType": "text",
        "contentType": "number",
        "defaultValue": defaults["MYMPD_JUKEBOX_MAX_SONG_DURATION"],
        "title": "Max. song duration",
        "form": "modalPlaybackJukeboxCollapse",
        "help": "helpJukeboxMaxSongDuration",
        "class": ["jukeboxSongOnly"],
        "unit": "Seconds"
    },
    "jukeboxFilterInclude": {
        "inputType": "text",
        "defaultValue": "",
        "title": "Include expression",
        "form": "modalPlaybackJukeboxCollapse",
        "help": "helpJukeboxFilterInclude"
    },
    "jukeboxFilterExclude": {
        "inputType": "text",
        "defaultValue": "",
        "title": "Exclude expression",
        "form": "modalPlaybackJukeboxCollapse",
        "help": "helpJukeboxFilterExclude"
    }
};

/**
 * Parses a string to boolean or number
 * @param {string} str string to parse
 * @returns {string | number | boolean} parsed string
 */
function parseString(str) {
    return str === 'true'
        ? true
        : str === 'false'
            ? false
            // @ts-ignore
            : isNaN(str)
                ? str
                : Number(str);
}

//Get settings from localStorage
for (const key in localSettings) {
    const value = localStorage.getItem(key);
    if (value !== null) {
        localSettings[key] = parseString(value);
    }
}

const userAgentData = {};

/**
 * Sets the useragentData object
 * @returns {void}
 */
function setUserAgentData() {
    //get interesting browser agent data
    //https://developer.mozilla.org/en-US/docs/Web/API/User-Agent_Client_Hints_API
    if (navigator.userAgentData) {
        navigator.userAgentData.getHighEntropyValues(["platform"]).then(ua => {
            /** @type {boolean} */
            userAgentData.isMobile = localSettings.viewMode === 'mobile'
                ? true
                : localSettings.viewMode === 'desktop'
                    ? false
                    : ua.mobile;
            //Safari does not support this API
            /** @type {boolean} */
            userAgentData.isSafari = false;
            /** @type {boolean} */
            userAgentData.isAndroid = /Android/i.test(ua.platform);
        });
    }
    else {
        /** @type {boolean} */
        userAgentData.isMobile = localSettings.viewMode === 'mobile'
            ? true
            : localSettings.viewMode === 'desktop'
                ? false
                : /iPhone|iPad|iPod|Android|Mobile/i.test(navigator.userAgent);
        /** @type {boolean} */
        userAgentData.isSafari = /Safari/i.test(navigator.userAgent) && ! /Chrome/i.test(navigator.userAgent);
        /** @type {boolean} */
        userAgentData.isAndroid = /Android/i.test(navigator.userAgent);
    }
}
setUserAgentData();

//minimum stable mpd version to support all myMPD features
const mpdVersion = {
    "major": 0,
    "minor": 23,
    "patch": 5
};

//remember offset for filesystem browsing uris
const browseFilesystemHistory = {};

//list of stickers for tags, playlists and filters (albums)
/** @type {Array} */
const stickerListAll = [
    'like',
    'rating',
    'playCount',
    'lastPlayed'
];

//list of stickers for songs
/** @type {Array} */
const stickerListSongs = stickerListAll.concat([
    'skipCount',
    'lastSkipped',
    'elapsed'
]);

//application state
const app = {};
app.cards = {
    "Home": {
        "offset": 0,
        "limit": 100,
        "filter": "",
        "sort": {
            "tag": "",
            "desc": false
        },
        "tag": "",
        "search": "",
        "scrollPos": 0
    },
    "Playback": {
        "offset": 0,
        "limit": 100,
        "filter": "",
        "sort": {
            "tag": "",
            "desc": false
        },
        "tag": "",
        "search": "",
        "scrollPos": 0
    },
    "Queue": {
        "active": "Current",
        "tabs": {
            "Current": {
                "offset": 0,
                "limit": 100,
                "filter": "any",
                "sort": {
                    "tag": "Priority",
                    "desc": false
                },
                "tag": "",
                "search": "",
                "scrollPos": 0
            },
            "LastPlayed": {
                "offset": 0,
                "limit": 100,
                "filter": "any",
                "sort": {
                    "tag": "",
                    "desc": false
                },
                "tag": "",
                "search": "",
                "scrollPos": 0
            },
            "Jukebox": {
                "active": "Song",
                "views": {
                    "Song": {
                        "offset": 0,
                        "limit": 100,
                        "filter": "any",
                        "sort": {
                            "tag": "",
                            "desc": false
                        },
                        "tag": "",
                        "search": "",
                        "scrollPos": 0
                    },
                    "Album": {
                        "offset": 0,
                        "limit": 100,
                        "filter": "any",
                        "sort": {
                            "tag": "",
                            "desc": false
                        },
                        "tag": "",
                        "search": "",
                        "scrollPos": 0
                    }
                }
            }
        }
    },
    "Browse": {
        "active": "Database",
        "tabs": {
            "Filesystem": {
                "offset": 0,
                "limit": 100,
                "filter": "/",
                "sort": {
                    "tag": "",
                    "desc": false
                },
                "tag": "dir",
                "search": "",
                "scrollPos": 0
            },
            "Playlist": {
                "active": "List",
                "views": {
                    "List": {
                        "offset": 0,
                        "limit": 100,
                        "filter": "",
                        "sort": {
                            "tag": "Name",
                            "desc": false
                        },
                        "tag": "",
                        "search": "",
                        "scrollPos": 0
                    },
                    "Detail": {
                        "offset": 0,
                        "limit": 100,
                        "filter": "any",
                        "sort": {
                            "tag": "",
                            "desc": false
                        },
                        "tag": "",
                        "search": "",
                        "scrollPos": 0
                    }
                }
            },
            "Database": {
                "active": "AlbumList",
                "views": {
                    "TagList": {
                        "offset": 0,
                        "limit": 100,
                        "filter": "any",
                        "sort": {
                            "tag": "",
                            "desc": false
                        },
                        "tag": "Album",
                        "search": "",
                        "scrollPos": 0
                    },
                    "AlbumList": {
                        "offset": 0,
                        "limit": 100,
                        "filter": "any",
                        "sort": {
                            "tag": "",
                            "desc": false
                        },
                        "tag": "Album",
                        "search": "",
                        "scrollPos": 0
                    },
                    "AlbumDetail": {
                        "offset": 0,
                        "limit": 100,
                        "filter": "",
                        "sort": {
                            "tag": "",
                            "desc": false
                        },
                        "tag": "",
                        "search": "",
                        "scrollPos": 0
                    }
                }
            },
            "Radio": {
                "active": "Favorites",
                "views": {
                    "Favorites": {
                        "offset": 0,
                        "limit": 100,
                        "filter": "any",
                        "sort": {
                            "tag": "Name",
                            "desc": false
                        },
                        "tag": "",
                        "search": "",
                        "scrollPos": 0
                    },
                    "Webradiodb": {
                        "offset": 0,
                        "limit": 100,
                        "filter": "any",
                        "sort": {
                            "tag": "Name",
                            "desc": false
                        },
                        "tag": "",
                        "search": "",
                        "scrollPos": 0
                    }
                }
            }
        }
    },
    "Search": {
        "offset": 0,
        "limit": 100,
        "filter": "any",
        "sort": {
            "tag": "Title",
            "desc": false
        },
        "tag": "",
        "search": "",
        "scrollPos": 0
    }
};

app.id = 'Home';
app.current = {
    "card": "Home",
    "tab": undefined,
    "view": undefined,
    "offset": 0,
    "limit": 100,
    "filter": "",
    "search": "",
    "sort": {
        "tag": "",
        "desc": false
    },
    "tag": "",
    "scrollPos": 0
};

app.last = {
    "card": undefined,
    "tab": undefined,
    "view": undefined,
    "offset": 0,
    "limit": 100,
    "filter": "",
    "search": "",
    "sort": {
        "tag": "",
        "desc": false
    },
    "tag": "",
    "scrollPos": 0
};
app.goto = false;

//features
const features = {
    "featBinarylimit": true,
    "featCacert": false,
    "featConsumeOneshot": false,
    "featFingerprint": false,
    "featHome": true,
    "featLibrary": false,
    "featLocalPlayback": false,
    "featLyrics": false,
    "featMediaSession": false,
    "featMounts": true,
    "featNeighbors": true,
    "featPartitions": true,
    "featPlaylistDirAuto": false,
    "featPlaylists": true,
    "featScripting": true,
    "featSmartpls": true,
    "featSmartplsAvailable": true,
    "featStickers": false,
    "featTags": true,
    "featTimer": true
};

//keyboard shortcuts
const keymap = {
    "playback": {"order": 0, "desc": "Playback"},
        " ": {"order": 1, "cmd": "clickPlay", "options": [], "desc": "Toggle play / pause", "key": "space_bar"},
        "s": {"order": 2, "cmd": "clickStop", "options": [], "desc": "Stop playing"},
        "ArrowLeft": {"order": 3, "cmd": "clickPrev", "options": [], "desc": "Previous song", "key": "keyboard_arrow_left"},
        "ArrowRight": {"order": 4, "cmd": "clickNext", "options": [], "desc": "Next song", "key": "keyboard_arrow_right"},
        "-": {"order": 5, "cmd": "volumeStep", "options": ["down"], "desc": "Volume down"},
        "+": {"order": 6, "cmd": "volumeStep", "options": ["up"], "desc": "Volume up"},
        "r": {"order": 7, "cmd": "togglePlaymode", "options": ["random"], "desc": "Toggle random"},
        "c": {"order": 8, "cmd": "togglePlaymode", "options": ["consume"], "desc": "Toggle consume"},
        "p": {"order": 9, "cmd": "togglePlaymode", "options": ["repeat"], "desc": "Toggle repeat"},
        "i": {"order": 9, "cmd": "togglePlaymode", "options": ["single"], "desc": "Toggle single mode"},
    "modals": {"order": 200, "desc": "Dialogs"},
        "A": {"order": 207, "cmd": "showAddToPlaylist", "options": ["stream", []], "desc": "Add stream"},
        "C": {"order": 207, "cmd": "openModal", "options": ["modalConnection"], "desc": "Open MPD connection"},
        "D": {"order": 207, "cmd": "clickTitle", "options": [], "desc": "Open current song details"},
        "G": {"order": 207, "cmd": "openModal", "options": ["modalTrigger"], "desc": "Open trigger", "feature": "featScripting"},
        "I": {"order": 207, "cmd": "openModal", "options": ["modalTimer"], "desc": "Open timer", "feature": "featTimer"},
        "L": {"order": 207, "cmd": "loginOrLogout", "options": [], "desc": "Login or logout", "feature": "featSession"},
        "M": {"order": 207, "cmd": "openModal", "options": ["modalMaintenance"], "desc": "Open maintenance"},
        "N": {"order": 207, "cmd": "openModal", "options": ["modalNotifications"], "desc": "Open notifications"},
        "O": {"order": 207, "cmd": "openModal", "options": ["modalMounts"], "desc": "Open mounts", "feature": "featMounts"},
        "P": {"order": 207, "cmd": "openModal", "options": ["modalPartitions"], "desc": "Open partitions", "feature": "featPartitions"},
        "Q": {"order": 207, "cmd": "openModal", "options": ["modalPlayback"], "desc": "Open playback settings"},
        "S": {"order": 207, "cmd": "showListScriptModal", "options": [], "desc": "Open scripts", "feature": "featScripting"},
        "T": {"order": 207, "cmd": "openModal", "options": ["modalSettings"], "desc": "Open settings"},
        "V": {"order": 207, "cmd": "showListVariablesModal", "options": [], "desc": "Open variables", "feature": "featScripting"},
        "?": {"order": 207, "cmd": "openModal", "options": ["modalAbout"], "desc": "Open about"},
        
    "navigation": {"order": 300, "desc": "Navigation"},
        "0": {"order": 301, "cmd": "appGoto", "options": ["Home"], "desc": "Show home", "feature": "featHome"},
        "1": {"order": 302, "cmd": "appGoto", "options": ["Playback"], "desc": "Show playback"},
        "2": {"order": 303, "cmd": "appGoto", "options": ["Queue", "Current"], "desc": "Show queue"},
        "3": {"order": 304, "cmd": "appGoto", "options": ["Queue", "LastPlayed"], "desc": "Show last played"},
        "4": {"order": 305, "cmd": "gotoJukebox", "options": [], "desc": "Show jukebox queue"},
        "5": {"order": 306, "cmd": "appGoto", "options": ["Browse", "Database"], "desc": "Show browse database", "feature": "featTags"},
        "6": {"order": 307, "cmd": "appGoto", "options": ["Browse", "Playlists"], "desc": "Show browse playlists", "feature": "featPlaylists"},
        "7": {"order": 308, "cmd": "appGoto", "options": ["Browse", "Filesystem"], "desc": "Show browse filesystem"},
        "8": {"order": 308, "cmd": "appGoto", "options": ["Browse", "Radio"], "desc": "Show browse webradio"},
        "9": {"order": 309, "cmd": "appGoto", "options": ["Search"], "desc": "Show search"},
        "/": {"order": 310, "cmd": "focusSearch", "options": [], "desc": "Focus search"}
};

//cache often accessed dom elements
const domCache = {};
domCache.body = document.querySelector('body');
domCache.counter = elGetById('counter');
domCache.footer = document.querySelector('footer');
domCache.main = document.querySelector('main');
domCache.progress = elGetById('footerProgress');
domCache.progressBar = elGetById('footerProgressBar');
domCache.progressPos = elGetById('footerProgressPos');
domCache.volumeBar = elGetById('volumeBar');
domCache.localPlayerProgress = elGetById('localPlayerProgress');

//Get BSN object references for fast access
const uiElements = {};
//all modals
for (const m of document.querySelectorAll('.modal')) {
    uiElements[m.id] = BSN.Modal.getInstance(m);
    m.addEventListener('shown.bs.modal', function(event) {
        focusFirstInput(event.target);
    }, false);
}
//other directly accessed BSN objects
uiElements.modalHomeIconLigatureDropdown = BSN.Dropdown.getInstance(elGetById('modalHomeIconLigatureBtn'));
uiElements.modalMountsNeighborsDropdown = BSN.Dropdown.getInstance(elGetById('modalMountsNeighborsBtn'));

const LUAfunctions = {
    "json.decode": {
        "desc": "Parses a Json string to a Lua table.",
        "func": "local data = json.decode(str)",
        "feat": ""
    },
    "json.encode": {
        "desc": "Encodes a Lua table as Json string.",
        "func": "local str = json.encode(data)",
        "feat": ""
    },
    "mympd.cache_cover_write": {
        "desc": "Write a file for the cover cache.",
        "func": "local rc, filename = mympd.cache_cover_write(src, uri)",
        "feat": ""
    },
    "mympd.cache_lyrics_write": {
        "desc": "Write the lyrics entry object to the lyrics cache.",
        "func": "local rc, filename = mympd.cache_lyrics_write(json.encode(entry), song_uri)",
        "feat": ""
    },
    "mympd.cache_thumbs_write": {
        "desc": "Write a file for the thumbs cache.",
        "func": "local rc, filename =  = mympd.cache_thumbs_write(src, uri)",
        "feat": ""
    },
    "mympd.dialog": {
        "desc": "Returns an Jsonrpc response for a script dialog.",
        "func": "return mympd.dialog(title, data, callback)",
        "feat": ""
    },
    "mympd.hash_md5": {
        "desc": "MD5 hash of string.",
        "func": "local hash = mympd.hash_md5(string)",
        "feat": ""
    },
    "mympd.hash_sha1": {
        "desc": "SHA1 hash of string.",
        "func": "local hash = mympd.hash_sha1(string)",
        "feat": ""
    },
    "mympd.hash_sha256": {
        "desc": "SHA256 hash of string.",
        "func": "local hash = mympd.hash_sha256(string)",
        "feat": ""
    },
    "mympd.http_client": {
        "desc": "HTTP client",
        "func": "local rc, code, headers, body = mympd.http_client(method, uri, extra_headers, payload)",
        "feat": ""
    },
    "mympd.http_download": {
        "desc": "HTTP download",
        "func": "local rc, code, headers = mympd.http_download(uri, extra_headers, out)",
        "feat": ""
    },
    "mympd.http_jsonrpc_error": {
        "desc": "Sends a JSONRPC 2.0 error.",
        "func": "return mympd.http_jsonrpc_error(method, msg)",
        "feat": ""
    },
    "mympd.http_jsonrpc_response": {
        "desc": "Sends a JSONRPC 2.0 response.",
        "func": "return mympd.http_jsonrpc_response(obj)",
        "feat": ""
    },
    "mympd.http_jsonrpc_warn": {
        "desc": "Sends a JSONRPC 2.0 warning.",
        "func": "return mympd.http_jsonrpc_warn(method, msg)",
        "feat": ""
    },
    "mympd.http_redirect": {
        "desc": "Sends a HTTP redirect.",
        "func": "return mympd.http_reply(location)",
        "feat": ""
    },
    "mympd.http_reply": {
        "desc": "Sends a HTTP reply.",
        "func": "return mympd.http_reply(status, header, body)",
        "feat": ""
    },
    "mympd.http_serve_file": {
        "desc": "Serves a file from the filesystem. Only files from the diskcache are allowed.",
        "func": "return mympd.http_serve_file(file)",
        "feat": ""
    },
    "mympd.init": {
        "desc": "Initializes the global lua table mympd_state.",
        "func": "mympd.init()",
        "feat": ""
    },
    "mympd.log": {
        "desc": "Logs messages to the myMPD log.",
        "func": "mympd.log(loglevel, message)",
        "feat": ""
    },
    "mympd.tmp_file": {
        "desc": "Generates a random tmp filename for the misc cache.",
        "func": "local tmp_file = mympd.tmp_file()",
        "feat": ""
    },
    "mympd.urldecode": {
        "desc": "URL decodes a string.",
        "func": "local decoded = mympd.urlencode(string, false)",
        "feat": ""
    },
    "mympd.update_mtime": {
        "desc":	"Updates the timestamp of a file.",
        "func": "local rc = mympd.update_mtime(filename)",
        "feat": ""
    },
    "mympd.urlencode": {
        "desc":	"URL encodes a string.",
        "func": "local encoded = mympd.urlencode(string)",
        "feat": ""
    },
    "mympd.os_capture": {
        "desc": "Executes a system command and capture its output.",
        "func": "local output = mympd.os_capture(command)",
        "feat": ""
    },
    "mympd.gpio_blink": {
        "desc": "Blinks a GPIO with given timeout and interval.",
        "func": "local rc = mympd.gpio_blink(gpio, timeout_ms, interval_ms)",
        "feat": "featMygpiod"
    },
    "mympd.gpio_get": {
        "desc": "Returns the active state of a GPIO.",
        "func": "local rc = mympd.gpio_get(gpio)",
        "feat": "featMygpiod"
    },
    "mympd.gpio_set": {
        "desc": "Sets the active state of a GPIO.",
        "func": "local rc = mygpio_gpio_set(mympd.mygpiod_socket, gpio, value)",
        "feat": "featMygpiod"
    },
    "mympd.gpio_toggle": {
        "desc": "Toggles the active state of a GPIO.",
        "func": "local rc = mygpio_gpio_toggle(mympd.mygpiod_socket, gpio)",
        "feat": "featMygpiod"
    },
    "mympd.notify_client": {
        "desc": "Sends a notification to the client that started this script.",
        "func": "mympd.notify_client(severity, message)",
        "feat": ""
    },
    "mympd.notify_partition": {
        "desc": "Sends a notification to all clients in the current partition.",
        "func": "mympd.notify_partition(severity, message)",
        "feat": ""
    },
    "mympd.splitlines": {
        "desc": "Split a multiline string in lines.",
        "func": "local lines = mympd.splitlines(str)",
        "feat": ""
    },
    "mympd.trim": {
        "desc": "Trims a string.",
        "func": "local trimed = mympd.trim(str)",
        "feat": ""
    },
    "mympd.sleep": {
        "desc": "Sleeps number of milliseconds.",
        "func": "sleep(ms)",
        "feat": ""
    },
    "mympd.read_file": {
        "desc": "Read an ascii file.",
        "func": "local content = mympd.read_file(path)",
        "feat": ""
    }
};

const typeFriendly = {
    'album': 'Album',
    'dir': 'Directory',
    'externalLink': 'External link',
    'openExternalLink': 'External link',
    'modal': 'Modal',
    'openModal': 'Modal',
    'plist': 'Playlist',
    'script': 'Script',
    'execScriptFromOptions': 'Script',
    'search': 'Search',
    'smartpls': 'Smart playlist',
    'song': 'Song',
    'stream': 'Stream',
    'view': 'View',
    'appGoto': 'View',
    'webradio': 'Webradio',
    'viewSettings': 'View settings',
    'disc': 'Disc',
    'work': 'Work'
};

const friendlyActions = {
    'appendPlayQueueAlbum': 'Append to queue and play',
    'appendPlayQueue': 'Append to queue and play',
    'appendQueueAlbum': 'Append to queue',
    'appendQueue': 'Append to queue',
    'appGoto': 'Goto view',
    'execScriptFromOptions': 'Execute script',
    'homeIconGoto': 'Show',
    'insertAfterCurrentQueueAlbum': 'Insert after current playing song',
    'insertAfterCurrentQueue': 'Insert after current playing song',
    'openExternalLink': 'Open external link',
    'openModal': 'Open modal',
    'replacePlayQueueAlbum': 'Replace queue and play',
    'replacePlayQueue': 'Replace queue and play',
    'replaceQueueAlbum': 'Replace queue',
    'replaceQueue': 'Replace queue'
};

const bgImageValues = [
    {"value": "", "text": "None"},
    {"value": "/assets/mympd-background-dark.svg", "text": "Default image dark"},
    {"value": "/assets/mympd-background-light.svg", "text": "Default image light"}
];
