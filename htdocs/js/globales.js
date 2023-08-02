"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
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
let allOutputs = null;

/** @type {object} */
const ligatures = {
    'checked': 'task_alt',
    'more': 'menu',
    'unchecked': 'radio_button_unchecked',
    'partitionSpecific': 'dashboard',
    'browserSpecific': 'web_asset'
};

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

let webradioDb = null;
const webradioDbPicsUri = 'https://jcorporation.github.io/webradiodb/db/pics/';

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

let materialIcons = {};
let phrasesDefault = {};
let phrases = {};

/**
 * This settings are saved in the browsers localStorage
 */
const settingsLocalFields = {
    "localPlaybackAutoplay": {
        "defaultValue": false,
        "inputType": "checkbox",
        "title": "Autoplay",
        "form": "SettingLocalPlaybackCollapse",
        "help": "helpSettingsLocalPlaybackAutoplay",
        "hint": ligatures['browserSpecific']
    },
    "partition": {
        "defaultValue": "default",
        "inputType": "none"
    },
    "scaleRatio": {
        "defaultValue": "1.0",
        "inputType": "text",
        "title": "Scale ratio",
        "form": "settingsThemeFrm2",
        "invalid": "Must be a number and greater than zero",
        "hint": ligatures['browserSpecific'],
        "cssClass": ["featMobile"],
        "validate": {
            "cmd": "validateFloatEl",
            "options": []
        }
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
        "form": "settingsThemeFrm2",
        "help": "helpSettingsViewMode",
        "hint": ligatures['browserSpecific']
    },
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
        "defaultValue": "8080",
        "inputType": "text",
        "title": "Stream port",
        "form": "SettingLocalPlaybackCollapse",
        "help": "helpSettingsStreamPort",
        "hint": ligatures['partitionSpecific'],
        "invalid": "Invalid stream port",
        "validate": {
            "cmd": "validateIntRangeEl",
            "options": [0, 65535]
        }
    },
    "streamUri": {
        "defaultValue": "",
        "placeholder": "auto",
        "inputType": "text",
        "title": "Stream URI",
        "form": "SettingLocalPlaybackCollapse",
        "help": "helpSettingsStreamUri",
        "hint": ligatures['partitionSpecific'],
        "invalid": "Invalid URI",
        "validate": {
            "cmd": "validateStreamEl",
            "options": []
        }
    },
    "highlightColor": {
        "defaultValue": "#28a745",
        "inputType": "color",
        "title": "Highlight color",
        "form": "settingsThemeFrm2",
        "hint": ligatures['partitionSpecific'],
        "invalid": "Invalid color",
        "validate": {
            "cmd": "validateColorEl",
            "options": []
        }
    },
    "highlightColorContrast": {
        "defaultValue": "#f6f5f4",
        "inputType": "color",
        "title": "Highlight contrast color",
        "form": "settingsThemeFrm2",
        "hint": ligatures['partitionSpecific'],
        "invalid": "Invalid color",
        "validate": {
            "cmd": "validateColorEl",
            "options": []
        }
    }
};

// global settings
const settingsFields = {
    "volumeMin": {
        "defaultValue": 0,
        "inputType": "input",
        "title": "Volume min.",
        "form": "settingsVolumeFrm",
        "reset": true,
        "invalid": "Must be a number between 0 and 100",
        "validate": {
            "cmd": "validateUintEl",
            "options": []
        }
    },
    "volumeMax": {
        "defaultValue": 100,
        "inputType": "input",
        "title": "Volume max.",
        "form": "settingsVolumeFrm",
        "reset": true,
        "invalid": "Must be a number between 0 and 100",
        "validate": {
            "cmd": "validateUintEl",
            "options": []
        }
    },
    "volumeStep": {
        "defaultValue": 5,
        "inputType": "input",
        "title": "Volume step",
        "form": "settingsVolumeFrm",
        "reset": true,
        "invalid": "Must be a number between 1 and 25",
        "validate": {
            "cmd": "validateUintEl",
            "options": []
        }
    },
    "lyricsUsltExt": {
        "defaultValue": "txt",
        "inputType": "input",
        "title": "Unsynced lyrics extension",
        "form": "SettingLyricsCollapse",
        "reset": true,
        "help": "helpSettingsUsltExt"
    },
    "lyricsSyltExt": {
        "defaultValue": "lrc",
        "inputType": "input",
        "title": "Synced lyrics extension",
        "form": "SettingLyricsCollapse",
        "reset": true,
        "help": "helpSettingsSyltExt"
    },
    "lyricsVorbisUslt": {
        "defaultValue": "LYRICS",
        "inputType": "input",
        "title": "Unsynced lyrics vorbis comment",
        "form": "SettingLyricsCollapse",
        "reset": true,
        "help": "helpSettingsVorbisUslt"
    },
    "lyricsVorbisSylt": {
        "defaultValue": "SYNCEDLYRICS",
        "inputType": "input",
        "title": "Synced lyrics vorbis comment",
        "form": "SettingLyricsCollapse",
        "reset": true,
        "help": "helpSettingsVorbisSylt"
    },
    "lastPlayedCount": {
        "defaultValue": "2000",
        "inputType": "input",
        "title": "Last played list count",
        "form": "settingsStatisticsFrm",
        "reset": true,
        "invalid": "Must be a number and greater than zero",
        "help": "helpSettingsLastPlayedCount",
        "validate": {
            "cmd": "validateUintEl",
            "options": []
        }
    },
    "listenbrainzToken": {
        "defaultValue": "",
        "inputType": "password",
        "title": "ListenBrainz Token",
        "form": "settingsCloudFrm1",
        "help": "helpSettingsListenBrainzToken"
    },
    "bookletName": {
        "defaultValue": "booklet.pdf",
        "inputType": "text",
        "title": "Booklet filename",
        "form": "settingsBookletFrm",
        "help": "helpSettingsBookletName",
        "invalid": "Invalid filename",
        "validate": {
            "cmd": "validateFilenameEl",
            "options": []
        }
    },
    "coverimageNames": {
        "defaultValue": "folder,cover",
        "inputType": "text",
        "title": "Filenames",
        "form": "settingsAlbumartFrm1",
        "reset": true,
        "invalid": "Invalid filename",
        "help": "helpSettingsCoverimageNames",
        "cssClass": [ "featLibrary" ],
        "validate": {
            "cmd": "validateFilenameListEl",
            "options": []
        }
    },
    "thumbnailNames": {
        "defaultValue": "folder-sm,cover-sm",
        "inputType": "text",
        "title": "Thumbnail names",
        "form": "settingsAlbumartFrm1",
        "reset": true,
        "invalid": "Invalid filename",
        "help": "helpSettingsThumbnailNames",
        "cssClass": [ "featLibrary" ],
        "validate": {
            "cmd": "validateFilenameListEl",
            "options": []
        }
    },
    "smartplsEnable": {
        "defaultValue": true,
        "inputType": "checkbox"
    },
    "smartplsInterval": {
        "contentType": "number",
        "invalid": "Must be a number and greater than zero",
        "validate": {
            "cmd": "validateUintEl",
            "options": []
        }
    },
    "smartplsPrefix": {
        "defaultValue": "myMPDsmart",
        "inputType": "text",
        "title": "Smart playlists prefix",
        "form": "settingsSmartplsFrm",
        "reset": true,
        "invalid": "Invalid prefix",
        "help": "helpSettingsSmartplsPrefix",
    },
    "smartplsSort": {
        "defaultValue": "",
        "inputType": "select",
        "title": "Order",
        "form": "settingsSmartplsFrm",
        "help": "helpSettingsSmartplsSort",
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
        "form": "settingsDefaultActionsFrm"
    },
    "clickRadiobrowser": {
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
        "form": "settingsDefaultActionsFrm"
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
        "form": "settingsDefaultActionsFrm"
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
        "form": "settingsDefaultActionsFrm"
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
        "form": "settingsDefaultActionsFrm"
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
        "form": "settingsDefaultActionsFrm"
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
        "form": "settingsDefaultActionsFrm"
    },
    "notificationPlayer": {
        "defaultValue": false,
        "inputType": "checkbox",
        "title": "Playback",
        "form": "settingsFacilitiesFrm",
        "help": "helpSettingsNotificationPlayer"
    },
    "notificationQueue": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Queue",
        "form": "settingsFacilitiesFrm",
        "help": "helpSettingsNotificationQueue"
    },
    "notificationGeneral": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "General",
        "form": "settingsFacilitiesFrm",
        "help": "helpSettingsNotificationGeneral"
    },
    "notificationDatabase": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Database",
        "form": "settingsFacilitiesFrm",
        "help": "helpSettingsNotificationDatabase"
    },
    "notificationPlaylist": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Playlist",
        "form": "settingsFacilitiesFrm",
        "help": "helpSettingsNotificationPlaylist"
    },
    "notificationScript": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Script",
        "form": "settingsFacilitiesFrm",
        "help": "helpSettingsNotificationScript"
    },
    "notifyPage": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "On page notifications",
        "form": "settingsNotificationsFrm",
        "help": "helpSettingsNotifyPage"
    },
    "notifyWeb": {
        "defaultValue": false,
        "inputType": "checkbox",
        "title": "Web notifications",
        "form": "settingsNotificationsFrm",
        "onClick": "toggleBtnNotifyWeb",
        "help": "helpSettingsNotifyWeb"
    },
    "mediaSession": {
        "defaultValue": false,
        "inputType": "checkbox",
        "title": "Media session",
        "form": "settingsNotificationsFrm",
        "warn": "Browser has no MediaSession support",
        "help": "helpSettingsMediaSession"
    },
    "footerSettingsPlayback": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Playback settings",
        "form": "settingsFooterFrm"
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
        "form": "settingsFooterFrm"
    },
    "footerVolumeLevel": {
        "defaultValue": false,
        "inputType": "checkbox",
        "title": "Volume level",
        "form": "settingsFooterFrm"
    },
    "footerNotifications": {
        "defaultValue": false,
        "inputType": "checkbox",
        "title": "Notification icon",
        "form": "settingsFooterFrm"
    },
    "maxElementsPerPage": {
        "defaultValue": 100,
        "validValues": {
            "25": 25,
            "50": 50,
            "100": 100,
            "250": 250,
            "500": 500
        },
        "inputType": "select",
        "contentType": "number",
        "title": "Elements per page",
        "form": "settingsListsFrm",
        "help": "helpSettingsMaxElementsPerPage"
    },
    "smallWidthTagRows": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Display tags in rows for small displays",
        "form": "settingsListsFrm",
        "help": "helpSettingsSmallWidthTagRows"
    },
    "quickPlayButton": {
        "defaultValue": false,
        "inputType": "checkbox",
        "title": "Quick play button",
        "form": "settingsListsFrm",
        "help": "helpSettingsQuickPlay"
    },
    "quickRemoveButton": {
        "defaultValue": false,
        "inputType": "checkbox",
        "title": "Quick remove button",
        "form": "settingsListsFrm",
        "help": "helpSettingsQuickRemove"
    },
    "compactGrids": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Compact grids",
        "form": "settingsListsFrm",
        "help": "helpSettingsCompactGrids"
    },
    "showHelp": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Show help",
        "form": "settingsThemeFrm3",
        "help": "helpSettingsHelp"
    },
    "showBackButton": {
        "defaultValue": false,
        "inputType": "checkbox",
        "title": "History back button",
        "form": "settingsNavigationBarFrm",
        "help": "helpSettingsBackButton"
    },
    "enableHome": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Homescreen",
        "form": "settingsFurtherFeaturesFrm",
        "help": "helpSettingsEnableHome"
    },
    "enableScripting": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Scripting",
        "form": "settingsFurtherFeaturesFrm",
        "warn": "Lua is not compiled in",
        "help": "helpSettingsEnableScripting"
    },
    "enableTrigger": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Trigger",
        "form": "settingsFurtherFeaturesFrm",
        "help": "helpSettingsEnableTrigger"
    },
    "enableTimer": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Timer",
        "form": "settingsFurtherFeaturesFrm",
        "help": "helpSettingsEnableTimer"
    },
    "enableMounts": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Mounts",
        "form": "settingsFurtherFeaturesFrm",
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
        "form": "settingsFurtherFeaturesFrm",
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
        "form": "settingsThemeFrm1",
        "onChange": "eventChangeTheme"
    },
    "thumbnailSize": {
        "defaultValue": 175,
        "inputType": "input",
        "contentType": "number",
        "title": "Thumbnail size",
        "form": "settingsAlbumartFrm2",
        "reset": true,
        "invalid": "Must be a number and greater than zero",
        "validate": {
            "cmd": "validateUintEl",
            "options": []
        }
    },
    "bgColor": {
        "defaultValue": "#060708",
        "inputType": "color",
        "title": "Color",
        "form": "settingsBgFrm",
        "reset": true
    },
    "bgImage": {
        "defaultValue": "",
        "inputType": "mympd-select-search",
        "cbCallback": "filterImageSelect",
        "title": "Image",
        "form": "settingsBgFrm"
    },
    "bgCover": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Albumart",
        "form": "settingsBgFrm"
    },
    "bgCssFilter": {
        "defaultValue": "grayscale(100%) opacity(20%)",
        "inputType": "input",
        "title": "CSS filter",
        "form": "settingsBgFrm",
        "reset": true
    },
    "locale": {
        "defaultValue": "default",
        "inputType": "select",
        "title": "Locale",
        "form": "settingsLocaleFrm",
        "onChange": "eventChangeLocale"
    },
    "startupView": {
        "defaultValue": null,
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
        "form": "settingsStartupFrm",
        "onChange": "eventChangeTheme"
    },
    "musicbrainzLinks": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Show MusicBrainz links",
        "form": "settingsCloudFrm2",
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
userAgentData.hasIO = 'IntersectionObserver' in window ? true : false;

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
        userAgentData.isSafari = /Safari/i.test(navigator.userAgent);
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

//list of stickers
const stickerList = [
    'stickerPlayCount',
    'stickerSkipCount',
    'stickerLastPlayed',
    'stickerLastSkipped',
    'stickerLike',
    'stickerElapsed'
];

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
                    "tag": "Pos",
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
                            "tag": "",
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
                            "tag": tagAlbumArtist,
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
                        "filter": "",
                        "sort": {
                            "tag": "",
                            "desc": false
                        },
                        "tag": "",
                        "search": "",
                        "scrollPos": 0
                    },
                    "Webradiodb": {
                        "offset": 0,
                        "limit": 100,
                        "filter": {
                            "genre": "",
                            "country": "",
                            "language": "",
                            "codec": "",
                            "bitrate": ""
                        },
                        "sort": {
                            "tag": "Name",
                            "desc": false
                        },
                        "tag": "",
                        "search": "",
                        "scrollPos": 0
                    },
                    "Radiobrowser": {
                        "offset": 0,
                        "limit": 100,
                        "filter": {
                            "tags": "",
                            "genre": "",
                            "country": "",
                            "language": ""
                        },
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
    "featStickers": false,
    "featTags": true,
    "featTimer": true,
    "featTrigger": true
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
        "A": {"order": 201, "cmd": "showAddToPlaylist", "options": ["stream", []], "desc": "Add stream"},
        "C": {"order": 202, "cmd": "openModal", "options": ["modalSettingsConnection"], "desc": "Open MPD connection"},
        "G": {"order": 207, "cmd": "openModal", "options": ["modalTrigger"], "desc": "Open trigger", "feature": "featTrigger"},
        "I": {"order": 207, "cmd": "openModal", "options": ["modalTimer"], "desc": "Open timer", "feature": "featTimer"},
        "L": {"order": 207, "cmd": "loginOrLogout", "options": [], "desc": "Login or logout", "feature": "featSession"},
        "M": {"order": 205, "cmd": "openModal", "options": ["modalMaintenance"], "desc": "Open maintenance"},
        "N": {"order": 206, "cmd": "openModal", "options": ["modalNotifications"], "desc": "Open notifications"},
        "O": {"order": 207, "cmd": "openModal", "options": ["modalMounts"], "desc": "Open mounts", "feature": "featMounts"},
        "P": {"order": 207, "cmd": "openModal", "options": ["modalPartitions"], "desc": "Open partitions", "feature": "featPartitions"},
        "Q": {"order": 203, "cmd": "openModal", "options": ["modalSettingsPlayback"], "desc": "Open queue settings"},
        "S": {"order": 207, "cmd": "showListScriptModal", "options": [], "desc": "Open scripts", "feature": "featScripting"},
        "T": {"order": 204, "cmd": "openModal", "options": ["modalSettings"], "desc": "Open settings"},
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
domCache.counter = document.getElementById('counter');
domCache.footer = document.querySelector('footer');
domCache.main = document.querySelector('main');
domCache.progress = document.getElementById('footerProgress');
domCache.progressBar = document.getElementById('footerProgressBar');
domCache.progressPos = document.getElementById('footerProgressPos');
domCache.volumeBar = document.getElementById('volumeBar');

//Get BSN object references for fast access
const uiElements = {};
//all modals
for (const m of document.querySelectorAll('.modal')) {
    uiElements[m.id] = BSN.Modal.getInstance(m);
}
//other directly accessed BSN objects
uiElements.dropdownHomeIconLigature = BSN.Dropdown.getInstance(document.getElementById('btnHomeIconLigature'));
uiElements.dropdownNeighbors = BSN.Dropdown.getInstance(document.getElementById('btnDropdownNeighbors'));
uiElements.collapseJukeboxMode = BSN.Collapse.getInstance(document.getElementById('collapseJukeboxMode'));

const LUAfunctions = {
    "mympd.http_client": {
        "desc": "HTTP client",
        "func": "rc, code, header, body = mympd.http_client(method, uri, headers, payload)"
    },
    "mympd.init": {
        "desc": "Initializes the mympd_state lua table",
        "func": "mympd.init()"
    },
    "mympd.os_capture": {
        "desc":	"Executes a system command and capture its output.",
        "func": "output = mympd.os_capture(command)"
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
    'cols': 'Columns',
    'disc': 'Disc'
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
