"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

const startTime = Date.now();
let socket = null;
let websocketConnected = false;
let websocketTimer = null;
let lastSong = '';
let lastSongObj = {};
let lastState;
const currentSong = {};
let playstate = '';
let settings = {"loglevel": 2};
let settingsParsed = 'no';
let progressTimer = null;
let deferredA2HSprompt;
let dragSrc;
let dragEl;
let showSyncedLyrics = false;
let scrollSyncedLyrics = true;
let appInited = false;
let scriptsInited = false;
let subdir = '';
let uiEnabled = true;
let locale = navigator.language || navigator.userLanguage;
let scale = '1.0';
const isMobile = /iPhone|iPad|iPod|Android/i.test(navigator.userAgent);
const hasIO = 'IntersectionObserver' in window ? true : false;
const ligatureMore = 'menu';
const progressBarTransition = 'width 1s linear';
let tagAlbumArtist = 'AlbumArtist';
const session = {"token": "", "timeout": 0};
const sessionLifetime = 1780;
const sessionRenewInterval = sessionLifetime * 500;
let sessionTimer = null;

//remember offset for filesystem browsing uris
const browseFilesystemHistory = {};

//list of stickers
const stickerList = ['stickerPlayCount', 'stickerSkipCount', 'stickerLastPlayed', 
    'stickerLastSkipped', 'stickerLike'];

//application state
const app = {};
app.apps = { 
    "Home": { 
        "offset": 0,
        "limit": 100,
        "filter": "-",
        "sort": "-",
        "tag": "-",
        "search": "",
        "scrollPos": 0
    },
    "Playback": { 
        "offset": 0,
        "limit": 100,
        "filter": "-",
        "sort": "-",
        "tag": "-",
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
                "sort": "-",
                "tag": "-",
                "search": "",
                "scrollPos": 0
            },
            "LastPlayed": {
                "offset": 0,
                "limit": 100,
                "filter": "any",
                "sort": "-",
                "tag": "-",
                "search": "",
                "scrollPos": 0 
            },
            "Jukebox": {
                "offset": 0,
                "limit": 100,
                "filter": "any",
                "sort": "-",
                "tag": "-",
                "search": "",
                "scrollPos": 0 
            }
        }
    },
    "Browse": { 
        "active": "Database", 
        "tabs":  { 
            "Filesystem": { 
                "offset": 0,
                "limit": 100,
                "filter": "-",
                "sort": "-",
                "tag": "-",
                "search": "",
                "scrollPos": 0 
            },
            "Playlists": { 
                "active": "List",
                "views": { 
                    "List": {
                        "offset": 0,
                        "limit": 100,
                        "filter": "-",
                        "sort": "-",
                        "tag": "-",
                        "search": "", 
                        "scrollPos": 0 
                    },
                    "Detail": {
                        "offset": 0,
                        "limit": 100,
                        "filter": "-",
                        "sort": "-",
                        "tag": "-",
                        "search": "",
                        "scrollPos": 0
                    }
                }
            },
            "Database": { 
                "active": "List",
                "views": { 
                    "List": { 
                        "offset": 0,
                        "limit": 100,
                        "filter": "any",
                        "sort": "AlbumArtist",
                        "tag": "Album",
                        "search": "",
                        "scrollPos": 0
                    },
                    "Detail": { 
                        "offset": 0,
                        "limit": 100,
                        "filter": "-",
                        "sort": "-",
                        "tag": "-",
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
        "sort": "-",
        "tag": "-",
        "search": "",
        "scrollPos": 0
    }
};

app.id = "Home";
app.current = { "app": "Home", "tab": undefined, "view": undefined, "offset": 0, "limit": 100, "filter": "", "search": "", "sort": "", "tag": "", "scrollPos": 0 };
app.last = { "app": undefined, "tab": undefined, "view": undefined, "offset": 0, "limit": 100, "filter": "", "search": "", "sort": "", "tag": "", "scrollPos": 0 };

//normal settings
const settingFields = {
    "volumeMin": {
        "defaultValue": 0,
        "inputType": "input",
        "title": "Volume min.",
        "form": "volumeSettingsFrm",
        "reset": true
    },
    "volumeMax": {
        "defaultValue": 100,
        "inputType": "input",
        "title": "Volume max.",
        "form": "volumeSettingsFrm",
        "reset": true
    },
    "volumeStep": {
        "defaultValue": 5,
        "inputType": "input",
        "title": "Volume step",
        "form": "volumeSettingsFrm",
        "reset": true
    },
    "lyricsUsltExt": {
        "defaultValue": "txt",
        "inputType": "input",
        "title": "Unsynced lyrics extension",
        "form": "collapseEnableLyrics",
        "reset": true
    },
    "lyricsSyltExt": {
        "defaultValue": "lrc",
        "inputType": "input",
        "title": "Synced lyrics extension",
        "form": "collapseEnableLyrics",
        "reset": true
    },
    "lyricsVorbisUslt": {
        "defaultValue": "LYRICS",
        "inputType": "input",
        "title": "Unsynced lyrics vorbis comment",
        "form": "collapseEnableLyrics",
        "reset": true
    },
    "lyricsVorbisSylt": {
        "defaultValue": "SYNCEDLYRICS",
        "inputType": "input",
        "title": "Synced lyrics vorbis comment",
        "form": "collapseEnableLyrics",
        "reset": true
    },
    "lastPlayedCount": {
        "defaultValue": 200,
        "inputType": "input",
        "title": "Last played list count",
        "form": "statisticsFrm",
        "reset": true,
        "invalid": "Must be a number and greater than zero"
    }
};

//webui settings default values
const webuiSettingsDefault = {
    "clickSong": { 
        "defaultValue": "append", 
        "validValues": { 
            "append": "Append to queue", 
            "replace": "Replace queue", 
            "view": "Song details"
        }, 
        "inputType": "select",
        "title": "Click song",
        "form": "clickSettingsFrm"
    },
    "clickQueueSong": { 
        "defaultValue": "play", 
        "validValues": {
            "play": "Play", 
            "view": "Song details",
        },
        "inputType": "select",
        "title": "Click song in queue",
        "form": "clickSettingsFrm"
    },
    "clickPlaylist": { 
        "defaultValue": "append", 
        "validValues": {
            "append": "Append to queue",
            "replace": "Replace queue",
            "view": "View playlist"
        },
        "inputType": "select",
        "title": "Click playlist",
        "form": "clickSettingsFrm"
    },
    "clickFolder": { 
        "defaultValue": "view", 
        "validValues": {
            "append": "Append to queue",
            "replace": "Replace queue",
            "view": "Open folder"
        },
        "inputType": "select",
        "title": "Click folder",
        "form": "clickSettingsFrm"
    },
    "clickAlbumPlay": { 
        "defaultValue": "replace", 
        "validValues": {
            "append": "Append to queue",
            "replace": "Replace queue",
        },
        "inputType": "select",
        "title": "Click album play button",
        "form": "clickSettingsFrm"
    },
    "notificationAAASection": {
        "inputType": "section",
        "subtitle": "Facilities",
        "form": "NotificationSettingsAdvFrm"
    },
    "notificationPlayer": {
        "defaultValue": false,
        "inputType": "checkbox",
        "title": "Playback",
        "form": "NotificationSettingsAdvFrm"
    },
    "notificationQueue": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Queue",
        "form": "NotificationSettingsAdvFrm"
    },
    "notificationGeneral": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "General",
        "form": "NotificationSettingsAdvFrm"
    },
    "notificationDatabase": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Database",
        "form": "NotificationSettingsAdvFrm"
    },
    "notificationPlaylist": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Playlist",
        "form": "NotificationSettingsAdvFrm"
    },
    "notificationScript": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Script",
        "form": "NotificationSettingsAdvFrm"
    },
    "notifyPage": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "On page notifications",
        "form": "NotificationSettingsFrm"
    },
    "notifyWeb": {
        "defaultValue": false,
        "inputType": "checkbox",
        "title": "Web notifications",
        "form": "NotificationSettingsFrm"
    },
    "mediaSession": {
        "defaultValue": false,
        "inputType": "checkbox",
        "title": "Media session",
        "form": "NotificationSettingsFrm"
    },
    "uiFooterQueueSettings": {
        "defaultValue": false,
        "inputType": "checkbox",
        "title": "Show playback settings in footer",
        "form": "footerFrm"
    },
    "uiFooterPlaybackControls": {
        "defaultValue": "pause",
        "validValues": {
            "pause": "pause only",
            "stop": "stop only",
            "both": "pause and stop"
        },
        "inputType": "select",
        "title": "Playback controls",
        "form": "footerFrm"
    },
    "uiMaxElementsPerPage": {
        "defaultValue": "100",
        "validValues": {
            "25": "25",
            "50": "50",
            "100": "100",
            "250": "250",
            "500": "500"
        },
        "inputType": "select",
        "contentType": "integer",
        "title": "Elements per page",
        "form": "appearanceSettingsFrm"
    },
    "enableHome": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Homescreen",
        "form": "enableFeaturesFrm"
    },
    "enableScripting": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Scripting",
        "form": "enableFeaturesFrm",
        "warn": "Lua is not compiled in"
    },
    "enableTrigger": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Trigger",
        "form": "enableFeaturesFrm"
    },
    "enableTimer": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Timer",
        "form": "enableFeaturesFrm"
    },
    "enableMounts": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Mounts",
        "form": "enableFeaturesFrm",
        "warn": "MPD does not support mounts"
    },
    "enableLocalPlayback": {
        "defaultValue": false,
        "inputType": "checkbox",
        "title": "Local playback",
        "form": "enableFeaturesFrm"
    },
    "enablePartitions": {
        "defaultValue": false,
        "inputType": "checkbox",
        "title": "Partitions",
        "form": "enableFeaturesFrm",
        "warn": "MPD does not support partitions"
    },
    "uiTheme": {
        "defaultValue": "theme-dark",
        "validValues": {
            "theme-autodetect": "Autodetect",
            "theme-dark": "Dark",
            "theme-light": "Light"
        },
        "inputType": "select",
        "title": "Theme",
        "form": "themeFrm",
        "onChange": "eventChangeTheme"
    },
    "uiHighlightColor": {
        "defaultValue": "#28a745",
        "inputType": "color",
        "title": "Highlight color",
        "form": "themeFrm",
        "reset": true
    },
    "uiCoverimageSize": {
        "defaultValue": 250,
        "inputType": "input",
        "contentType": "integer",
        "title": "Size normal",
        "form": "coverimageFrm",
        "reset": true
    },
    "uiCoverimageSizeSmall": {
        "defaultValue": 175,
        "inputType": "input",
        "contentType": "integer",
        "title": "Size small",
        "form": "coverimageFrm",
        "reset": true
    },
    "uiBgColor": {
        "defaultValue": "#000000",
        "inputType": "color",
        "title": "Color",
        "form": "bgFrm",
        "reset": true
    },
    "uiBgImage": {
        "defaultValue": "",
        "inputType": "select",
        "title": "Image",
        "form": "bgFrm2"
    },
    "uiBgCover": {
        "defaultValue": true,
        "inputType": "checkbox",
        "title": "Albumart",
        "form": "bgFrm"
    },
    "uiBgCssFilter": {
        "defaultValue": "grayscale(100%) opacity(10%)",
        "inputType": "input",
        "title": "CSS filter",
        "form": "bgCssFilterFrm",
        "reset": true
    },
    "uiLocale": {
        "defaultValue": "default",
        "inputType": "select",
        "title": "Locale",
        "form": "localeFrm",
        "onChange": "eventChangeLocale"
    }
};

//features
const features = {
    "featAdvsearch": true,
    "featCacert": false,
    "featHome": true,
    "featLibrary": false,
    "featLocalPlayback": false,
    "featLyrics": false,
    "featMounts": true,
    "featNeighbors": true,
    "featPartitions": true,
    "featPlaylists": true,
    "featSingleOneShot": true,
    "featScripting": true,
    "featSmartpls": true,
    "featStickers": false,
    "featTags": true,
    "featTimer": true,
    "featTrigger": true,
    "featBinarylimit": true,
    "featFingerprint": false
};

//keyboard shortcuts
const keymap = {
    "ArrowLeft": {"cmd": "clickPrev", "options": [], "desc": "Previous song", "key": "keyboard_arrow_left"},
    "ArrowRight": {"cmd": "clickNext", "options": [], "desc": "Next song", "key": "keyboard_arrow_right"},
    " ": {"cmd": "clickPlay", "options": [], "desc": "Toggle play / pause", "key": "space_bar"},
    "s": {"cmd": "clickStop", "options": [], "desc": "Stop playing"},
    "-": {"cmd": "volumeStep", "options": ["down"], "desc": "Volume down"},
    "+": {"cmd": "volumeStep", "options": ["up"], "desc": "Volume up"},
    "u": {"cmd": "updateDB", "options": ["", true, false], "desc": "Update database"},
    "r": {"cmd": "updateDB", "options": ["", true, true], "desc": "Rescan database"},
    "p": {"cmd": "updateSmartPlaylists", "options": [false], "desc": "Update smart playlists", "req": "featSmartpls"},
    "a": {"cmd": "showAddToPlaylist", "options": ["stream", ""], "desc": "Add stream"},
    "t": {"cmd": "openModal", "options": ["modalSettings"], "desc": "Open settings"},
    "i": {"cmd": "clickTitle", "options": [], "desc": "Open song details"},
    "0": {"cmd": "appGoto", "options": ["Home"], "desc": "Goto home"},
    "1": {"cmd": "appGoto", "options": ["Playback"], "desc": "Goto playback"},
    "2": {"cmd": "appGoto", "options": ["Queue", "Current"], "desc": "Goto queue"},
    "3": {"cmd": "appGoto", "options": ["Queue", "LastPlayed"], "desc": "Goto last played"},
    "4": {"cmd": "appGoto", "options": ["Queue", "Jukebox"], "desc": "Goto jukebox queue"},
    "5": {"cmd": "appGoto", "options": ["Browse", "Database"], "desc": "Goto browse database", "req": "featTags"},
    "6": {"cmd": "appGoto", "options": ["Browse", "Playlists"], "desc": "Goto browse playlists", "req": "featPlaylists"},
    "7": {"cmd": "appGoto", "options": ["Browse", "Filesystem"], "desc": "Goto browse filesystem"},
    "8": {"cmd": "appGoto", "options": ["Search"], "desc": "Goto search"},
    "?": {"cmd": "openModal", "options": ["modalAbout"], "desc": "Open about"},
    "/": {"cmd": "focusSearch", "options": [], "desc": "Focus search"},
    "F": {"cmd": "openFullscreen", "options": [], "desc": "Open fullscreen"}
};

//cache often accessed dom elements
const domCache = {};
domCache.body = document.getElementsByTagName('body')[0];
domCache.counter = document.getElementById('counter');
domCache.progress = document.getElementById('footerProgress');
domCache.progressBar = document.getElementById('footerProgressBar');
domCache.progressPos = document.getElementById('footerProgressPos');

//BSN ui objects
const uiElements = {};
uiElements.modalConnection = new BSN.Modal(document.getElementById('modalConnection'));
uiElements.modalSettings = new BSN.Modal(document.getElementById('modalSettings'));
uiElements.modalQueueSettings = new BSN.Modal(document.getElementById('modalQueueSettings'));
uiElements.modalAbout = new BSN.Modal(document.getElementById('modalAbout')); 
uiElements.modalSaveQueue = new BSN.Modal(document.getElementById('modalSaveQueue'));
uiElements.modalAddToQueue = new BSN.Modal(document.getElementById('modalAddToQueue'));
uiElements.modalSongDetails = new BSN.Modal(document.getElementById('modalSongDetails'));
uiElements.modalAddToPlaylist = new BSN.Modal(document.getElementById('modalAddToPlaylist'));
uiElements.modalRenamePlaylist = new BSN.Modal(document.getElementById('modalRenamePlaylist'));
uiElements.modalUpdateDB = new BSN.Modal(document.getElementById('modalUpdateDB'));
uiElements.modalSaveSmartPlaylist = new BSN.Modal(document.getElementById('modalSaveSmartPlaylist'));
uiElements.modalTimer = new BSN.Modal(document.getElementById('modalTimer'));
uiElements.modalMounts = new BSN.Modal(document.getElementById('modalMounts'));
uiElements.modalExecScript = new BSN.Modal(document.getElementById('modalExecScript'));
uiElements.modalScripts = new BSN.Modal(document.getElementById('modalScripts'));
uiElements.modalPartitions = new BSN.Modal(document.getElementById('modalPartitions'));
uiElements.modalPartitionOutputs = new BSN.Modal(document.getElementById('modalPartitionOutputs'));
uiElements.modalTrigger = new BSN.Modal(document.getElementById('modalTrigger'));
uiElements.modalOutputAttributes = new BSN.Modal(document.getElementById('modalOutputAttributes'));
uiElements.modalPicture = new BSN.Modal(document.getElementById('modalPicture'));
uiElements.modalEditHomeIcon = new BSN.Modal(document.getElementById('modalEditHomeIcon'));
uiElements.modalConfirm = new BSN.Modal(document.getElementById('modalConfirm'));
uiElements.modalEnterPin = new BSN.Modal(document.getElementById('modalEnterPin'));
uiElements.modalSetSongPriority = new BSN.Modal(document.getElementById('modalSetSongPriority'));

uiElements.dropdownVolumeMenu = new BSN.Dropdown(document.getElementById('volumeMenu'));
uiElements.dropdownLocalPlayer = new BSN.Dropdown(document.getElementById('localPlaybackMenu'));
uiElements.dropdownDatabaseSort = new BSN.Dropdown(document.getElementById('btnDatabaseSortDropdown'));
uiElements.dropdownNeighbors = new BSN.Dropdown(document.getElementById('btnDropdownNeighbors'));
uiElements.dropdownHomeIconLigature = new BSN.Dropdown(document.getElementById('btnHomeIconLigature'));

uiElements.collapseDBupdate = new BSN.Collapse(document.getElementById('navDBupdate'));
uiElements.collapseSettings = new BSN.Collapse(document.getElementById('navSettings'));
uiElements.collapseScripting = new BSN.Collapse(document.getElementById('navScripting'));
uiElements.collapseJukeboxMode = new BSN.Collapse(document.getElementById('collapseJukeboxMode'));

const LUAfunctions = {
    "mympd_api_http_client": {
        "desc": "HTTP client",
        "func": "rc, response, header, body = mympd_api_http_client(method, uri, headers, payload)"
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
