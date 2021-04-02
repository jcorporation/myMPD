"use strict";
// SPDX-License-Identifier: GPL-2.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

let socket = null;
let websocketConnected = false;
let websocketTimer = null;
let lastSong = '';
let lastSongObj = {};
let lastState;
const currentSong = {};
let playstate = '';
let settingsLock = false;
let settingsParsed = false;
let settingsNew = {};
let settings = {};
settings.loglevel = 2;
let alertTimeout = null;
let progressTimer = null;
let deferredA2HSprompt;
let dragSrc;
let dragEl;
let showSyncedLyrics = false;
let scrollSyncedLyrics = true;
let appInited = false;
let subdir = '';
let uiEnabled = true;
let locale = navigator.language || navigator.userLanguage;
let scale = '1.0';
const isMobile = /iPhone|iPad|iPod|Android/i.test(navigator.userAgent);
const ligatureMore = 'menu';
const progressBarTransition = 'width 1s linear';
let tagAlbumArtist = 'AlbumArtist';

//remember offset for filesystem browsing uris
const browseFilesystemHistory = {};

//list of stickers
const stickerList = ['stickerPlayCount', 'stickerSkipCount', 'stickerLastPlayed', 
    'stickerLastSkipped', 'stickerLike'];

//list of themes
const themes = {
    "theme-autodetect": "Autodetect",
    "theme-default": "Default",
    "theme-dark": "Dark",
    "theme-light": "Light"
};

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

app.current = { "app": "Home", "tab": undefined, "view": undefined, "offset": 0, "limit": 100, "filter": "", "search": "", "sort": "", "tag": "", "scrollPos": 0 };
app.last = { "app": undefined, "tab": undefined, "view": undefined, "offset": 0, "limit": 100, "filter": "", "search": "", "sort": "", "tag": "", "scrollPos": 0 };

//advanced settings default values und ui display configuration
const advancedSettingsDefault = {
    "clickSong": { 
        "defaultValue": "append", 
        "validValues": { 
            "append": "Append to queue", 
            "replace": "Replace queue", 
            "view": "Song details"
        }, 
        "inputType": "select",
        "title": "Click song",
        "form": "AdvancedSettingsFrm"
    },
    "clickQueueSong": { 
        "defaultValue": "play", 
        "validValues": {
            "play": "Play", 
            "view": "Song details",
        },
        "inputType": "select",
        "title": "Click song in queue",
        "form": "AdvancedSettingsFrm"
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
        "form": "AdvancedSettingsFrm"
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
        "form": "AdvancedSettingsFrm"
    },
    "clickAlbumPlay": { 
        "defaultValue": "replace", 
        "validValues": {
            "append": "Append to queue",
            "replace": "Replace queue",
        },
        "inputType": "select",
        "title": "Click album play button",
        "form": "AdvancedSettingsFrm"
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
    "uiAAASection": {
        "inputType": "section",
        "title": "Appearance",
        "form": "AdvancedSettingsFrm"
    },
    "uiFooterQueueSettings": {
        "defaultValue": false,
        "inputType": "checkbox",
        "title": "Show playback settings in footer",
        "form": "AdvancedSettingsFrm"
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
        "form": "AdvancedSettingsFrm"
    },
    "uiMaxElementsPerPage": {
        "defaultValue": "100",
        "validValues": {
            "25": "25",
            "50": "50",
            "100": "100",
            "200": "200",
            "0": "All"
        },
        "inputType": "select",
        "title": "Elements per page",
        "form": "AdvancedSettingsFrm"
    },
    "uiLocalPlayback": {
        "defaultValue": false,
        "inputType": "checkbox",
        "title": "Local playback",
        "form": "AdvancedSettingsFrm"
    }
};

//keyboard shortcuts
const keymap = {
    "ArrowLeft": {"cmd": "clickPrev", "options": [], "desc": "Previous song", "key": "keyboard_arrow_left"},
    "ArrowRight": {"cmd": "clickNext", "options": [], "desc": "Next song", "key": "keyboard_arrow_right"},
    " ": {"cmd": "clickPlay", "options": [], "desc": "Toggle play / pause", "key": "space_bar"},
    "s": {"cmd": "clickStop", "options": [], "desc": "Stop playing"},
    "-": {"cmd": "volumeStep", "options": ["down"], "desc": "Volume down"},
    "+": {"cmd": "volumeStep", "options": ["up"], "desc": "Volume up"},
    "c": {"cmd": "sendAPI", "options": [{"cmd": "MPD_API_QUEUE_CLEAR"}], "desc": "Clear queue"},
    "u": {"cmd": "updateDB", "options": ["", true], "desc": "Update database"},
    "r": {"cmd": "rescanDB", "options": ["", true], "desc": "Rescan database"},
    "p": {"cmd": "updateSmartPlaylists", "options": [false], "desc": "Update smart playlists", "req": "featSmartpls"},
    "a": {"cmd": "showAddToPlaylist", "options": ["stream", ""], "desc": "Add stream"},
    "t": {"cmd": "openModal", "options": ["modalSettings"], "desc": "Open settings"},
    "i": {"cmd": "clickTitle", "options": [], "desc": "Open song details"},
    "l": {"cmd": "openDropdown", "options": ["dropdownLocalPlayer"], "desc": "Open local player"},
    "0": {"cmd": "appGoto", "options": ["Home"], "desc": "Goto home"},
    "1": {"cmd": "appGoto", "options": ["Playback"], "desc": "Goto playback"},
    "2": {"cmd": "appGoto", "options": ["Queue", "Current"], "desc": "Goto queue"},
    "3": {"cmd": "appGoto", "options": ["Queue", "LastPlayed"], "desc": "Goto last played"},
    "4": {"cmd": "appGoto", "options": ["Queue", "Jukebox"], "desc": "Goto jukebox queue"},
    "5": {"cmd": "appGoto", "options": ["Browse", "Database"], "desc": "Goto browse database", "req": "featTags"},
    "6": {"cmd": "appGoto", "options": ["Browse", "Playlists"], "desc": "Goto browse playlists", "req": "featPlaylists"},
    "7": {"cmd": "appGoto", "options": ["Browse", "Filesystem"], "desc": "Goto browse filesystem"},
    "8": {"cmd": "appGoto", "options": ["Search"], "desc": "Goto search"},
    "m": {"cmd": "openDropdown", "options": ["dropdownMainMenu"], "desc": "Open main menu"},
    "v": {"cmd": "openDropdown", "options": ["dropdownVolumeMenu"], "desc": "Open volume menu"},
    "S": {"cmd": "sendAPI", "options": [{"cmd": "MPD_API_QUEUE_SHUFFLE"}], "desc": "Shuffle queue"},
    "C": {"cmd": "sendAPI", "options": [{"cmd": "MPD_API_QUEUE_CROP"}], "desc": "Crop queue"},
    "?": {"cmd": "openModal", "options": ["modalAbout"], "desc": "Open about"},
    "/": {"cmd": "focusSearch", "options": [], "desc": "Focus search"},
    "n": {"cmd": "focusTable", "options": [], "desc": "Focus table"},
    "q": {"cmd": "queueSelectedItem", "options": [true], "desc": "Append item to queue"},
    "Q": {"cmd": "queueSelectedItem", "options": [false], "desc": "Replace queue with item"},
    "d": {"cmd": "dequeueSelectedItem", "options": [], "desc": "Remove item from queue"},
    "x": {"cmd": "addSelectedItemToPlaylist", "options": [], "desc": "Append item to playlist"},
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
uiElements.modalSaveBookmark = new BSN.Modal(document.getElementById('modalSaveBookmark'));
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

uiElements.dropdownMainMenu = new BSN.Dropdown(document.getElementById('mainMenu'));
uiElements.dropdownVolumeMenu = new BSN.Dropdown(document.getElementById('volumeMenu'));
uiElements.dropdownBookmarks = new BSN.Dropdown(document.getElementById('BrowseFilesystemBookmark'));
uiElements.dropdownLocalPlayer = new BSN.Dropdown(document.getElementById('localPlaybackMenu'));
uiElements.dropdownDatabaseSort = new BSN.Dropdown(document.getElementById('btnDatabaseSortDropdown'));
uiElements.dropdownNeighbors = new BSN.Dropdown(document.getElementById('btnDropdownNeighbors'));
uiElements.dropdownHomeIconLigature = new BSN.Dropdown(document.getElementById('btnHomeIconLigature'));

uiElements.collapseDBupdate = new BSN.Collapse(document.getElementById('navDBupdate'));
uiElements.collapseSettings = new BSN.Collapse(document.getElementById('navSettings'));
uiElements.collapseScripting = new BSN.Collapse(document.getElementById('navScripting'));
uiElements.collapseJukeboxMode = new BSN.Collapse(document.getElementById('labelJukeboxMode'));
