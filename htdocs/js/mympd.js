"use strict";
// SPDX-License-Identifier: GPL-2.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

// Disable eslint warnings
// global BSN, phrases, locales

var socket = null;
var websocketConnected = false;
var websocketTimer = null;
var socketRetry = 0;

var lastSong = '';
var lastSongObj = {};
var lastState;
var currentSong = {};
var playstate = '';
var settingsLock = false;
var settingsParsed = false;
var settingsNew = {};
var settings = {};
settings.loglevel = 2;
var alertTimeout = null;
var progressTimer = null;
var deferredA2HSprompt;
var dragSrc;
var dragEl;
var showSyncedLyrics = false;

var appInited = false;
var subdir = '';
var uiEnabled = true;
var locale = navigator.language || navigator.userLanguage;
var scale = '1.0';
var isMobile = /iPhone|iPad|iPod|Android/i.test(navigator.userAgent);
var ligatureMore = 'menu';
var progressBarTransition = 'width 1s linear';
var tagAlbumArtist = 'AlbumArtist';

var app = {};
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
                "active": "All",
                "views": { 
                    "All": {
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

var domCache = {};
domCache.counter = document.getElementById('counter');
domCache.volumePrct = document.getElementById('volumePrct');
domCache.volumeControl = document.getElementById('volumeControl');
domCache.volumeMenu = document.getElementById('volumeMenu');
domCache.btnsPlay = document.getElementsByClassName('btnPlay');
domCache.btnsPlayLen = domCache.btnsPlay.length;
domCache.btnPrev = document.getElementById('btnPrev');
domCache.btnNext = document.getElementById('btnNext');
domCache.progress = document.getElementById('footerProgress');
domCache.progressBar = document.getElementById('footerProgressBar');
domCache.progressPos = document.getElementById('footerProgressPos');
domCache.volumeBar = document.getElementById('volumeBar');
domCache.outputs = document.getElementById('outputs');
domCache.currentCover = document.getElementById('currentCover');
domCache.currentTitle = document.getElementById('currentTitle');
domCache.footerTitle = document.getElementById('footerTitle');
domCache.footerArtist = document.getElementById('footerArtist');
domCache.footerAlbum = document.getElementById('footerAlbum');
domCache.footerCover = document.getElementById('footerCover');
domCache.btnVoteUp = document.getElementById('btnVoteUp');
domCache.btnVoteDown = document.getElementById('btnVoteDown');
domCache.badgeQueueItems = null;
domCache.searchstr = document.getElementById('searchstr');
domCache.searchCrumb = document.getElementById('searchCrumb');
domCache.body = document.getElementsByTagName('body')[0];
domCache.footer = document.getElementsByTagName('footer')[0];
domCache.header = document.getElementById('header');
domCache.mainMenu = document.getElementById('mainMenu');

/* eslint-disable no-unused-vars */
var modalConnection = new BSN.Modal(document.getElementById('modalConnection'));
var modalSettings = new BSN.Modal(document.getElementById('modalSettings'));
var modalAbout = new BSN.Modal(document.getElementById('modalAbout')); 
var modalSaveQueue = new BSN.Modal(document.getElementById('modalSaveQueue'));
var modalAddToQueue = new BSN.Modal(document.getElementById('modalAddToQueue'));
var modalSongDetails = new BSN.Modal(document.getElementById('modalSongDetails'));
var modalAddToPlaylist = new BSN.Modal(document.getElementById('modalAddToPlaylist'));
var modalRenamePlaylist = new BSN.Modal(document.getElementById('modalRenamePlaylist'));
var modalUpdateDB = new BSN.Modal(document.getElementById('modalUpdateDB'));
var modalSaveSmartPlaylist = new BSN.Modal(document.getElementById('modalSaveSmartPlaylist'));
var modalDeletePlaylist = new BSN.Modal(document.getElementById('modalDeletePlaylist'));
var modalSaveBookmark = new BSN.Modal(document.getElementById('modalSaveBookmark'));
var modalTimer = new BSN.Modal(document.getElementById('modalTimer'));
var modalMounts = new BSN.Modal(document.getElementById('modalMounts'));
var modalExecScript = new BSN.Modal(document.getElementById('modalExecScript'));
var modalScripts = new BSN.Modal(document.getElementById('modalScripts'));
var modalPartitions = new BSN.Modal(document.getElementById('modalPartitions'));
var modalPartitionOutputs = new BSN.Modal(document.getElementById('modalPartitionOutputs'));
var modalTrigger = new BSN.Modal(document.getElementById('modalTrigger'));
var modalOutputAttributes = new BSN.Modal(document.getElementById('modalOutputAttributes'));
var modalPicture = new BSN.Modal(document.getElementById('modalPicture'));
var modalEditHomeIcon = new BSN.Modal(document.getElementById('modalEditHomeIcon'));
var modalReally = new BSN.Modal(document.getElementById('modalReally'));

var dropdownMainMenu = new BSN.Dropdown(document.getElementById('mainMenu'));
var dropdownVolumeMenu = new BSN.Dropdown(document.getElementById('volumeMenu'));
var dropdownBookmarks = new BSN.Dropdown(document.getElementById('BrowseFilesystemBookmark'));
var dropdownLocalPlayer = new BSN.Dropdown(document.getElementById('localPlaybackMenu'));
var dropdownPlay = new BSN.Dropdown(document.getElementById('btnPlayDropdown'));
var dropdownDatabaseSort = new BSN.Dropdown(document.getElementById('btnDatabaseSortDropdown'));
var dropdownNeighbors = new BSN.Dropdown(document.getElementById('btnDropdownNeighbors'));
var dropdownHomeIconLigature = new BSN.Dropdown(document.getElementById('btnHomeIconLigature'));

var collapseDBupdate = new BSN.Collapse(document.getElementById('navDBupdate'));
var collapseSettings = new BSN.Collapse(document.getElementById('navSettings'));
var collapseSyscmds = new BSN.Collapse(document.getElementById('navSyscmds'));
var collapseScripting = new BSN.Collapse(document.getElementById('navScripting'));
var collapseJukeboxMode = new BSN.Collapse(document.getElementById('labelJukeboxMode'));
/* eslint-enable no-unused-vars */

function appPrepare(scrollPos) {
    if (app.current.app !== app.last.app || app.current.tab !== app.last.tab || app.current.view !== app.last.view) {
        //Hide all cards + nav
        for (let i = 0; i < domCache.navbarBtnsLen; i++) {
            domCache.navbarBtns[i].classList.remove('active');
        }
        document.getElementById('cardHome').classList.add('hide');
        document.getElementById('cardPlayback').classList.add('hide');
        document.getElementById('cardQueue').classList.add('hide');
        document.getElementById('cardBrowse').classList.add('hide');
        document.getElementById('cardSearch').classList.add('hide');
        document.getElementById('cardQueueCurrent').classList.add('hide');
        document.getElementById('cardQueueLastPlayed').classList.add('hide');
        document.getElementById('cardQueueJukebox').classList.add('hide');
        document.getElementById('cardBrowsePlaylists').classList.add('hide');
        document.getElementById('cardBrowseFilesystem').classList.add('hide');
        document.getElementById('cardBrowseDatabase').classList.add('hide');
        //show active card
        document.getElementById('card' + app.current.app).classList.remove('hide');
        if (app.current.tab !== undefined) {
            document.getElementById('card' + app.current.app + app.current.tab).classList.remove('hide');
        }
        //show active navbar icon
        let nav = document.getElementById('nav' + app.current.app + app.current.tab);
        if (nav) {
            nav.classList.add('active');
        }
        else {
            nav = document.getElementById('nav' + app.current.app);
            if (nav) {
                document.getElementById('nav' + app.current.app).classList.add('active');
            }
        }
    }
    scrollToPosY(scrollPos);
    const list = document.getElementById(app.current.app + 
        (app.current.tab === undefined ? '' : app.current.tab) + 
        (app.current.view === undefined ? '' : app.current.view) + 'List');
    if (list) {
        list.classList.add('opacity05');
    }
}

function appGoto(card, tab, view, offset, limit, filter, sort, tag, search, newScrollPos) {
    //save scrollPos of current view
    let scrollPos = 0;
    if (document.body.scrollTop) {
        scrollPos = document.body.scrollTop
    }
    else {
        scrollPos = document.documentElement.scrollTop;
    }
        
    if (app.apps[app.current.app].scrollPos !== undefined) {
        app.apps[app.current.app].scrollPos = scrollPos
    }
    else if (app.apps[app.current.app].tabs[app.current.tab].scrollPos !== undefined) {
        app.apps[app.current.app].tabs[app.current.tab].scrollPos = scrollPos
    }
    else if (app.apps[app.current.app].tabs[app.current.tab].views[app.current.view].scrollPos !== undefined) {
        app.apps[app.current.app].tabs[app.current.tab].views[app.current.view].scrollPos = scrollPos;
    }

    //set null options to undefined
    if (offset === null) {
        offset = undefined;
    }
    if (limit === null) {
        limit = undefined;
    }
    if (filter === null) {
        filter = undefined;
    }
    if (sort === null) {
        sort = undefined;
    }
    if (tag === null) {
        tag = undefined;
    }
    if (search === null) {
        search = undefined;
    }

    //build new hash
    let hash = '';
    if (app.apps[card].tabs) {
        if (tab === undefined) {
            tab = app.apps[card].active;
        }
        if (app.apps[card].tabs[tab].views) {
            if (view === undefined) {
                view = app.apps[card].tabs[tab].active;
            }
            hash = '/' + encodeURIComponent(card) + '/' + encodeURIComponent(tab) + '/' + encodeURIComponent(view) + '!' + 
                encodeURIComponent(offset === undefined ? app.apps[card].tabs[tab].views[view].offset : offset) + '/' +
                encodeURIComponent(limit === undefined ? app.apps[card].tabs[tab].views[view].limit : limit) + '/' +
                encodeURIComponent(filter === undefined ? app.apps[card].tabs[tab].views[view].filter : filter) + '/' +
                encodeURIComponent(sort === undefined ? app.apps[card].tabs[tab].views[view].sort : sort) + '/' +
                encodeURIComponent(tag === undefined ? app.apps[card].tabs[tab].views[view].tag : tag) + '/' +
                encodeURIComponent(search === undefined ? app.apps[card].tabs[tab].views[view].search : search);
            if (newScrollPos !== undefined) {
                app.apps[card].tabs[tab].views[view].scrollPos = newScrollPos;
            }
        }
        else {
            hash = '/' + encodeURIComponent(card) + '/' + encodeURIComponent(tab) + '!' + 
                encodeURIComponent(offset === undefined ? app.apps[card].tabs[tab].offset : offset) + '/' +
                encodeURIComponent(limit === undefined ? app.apps[card].tabs[tab].limit : limit) + '/' +
                encodeURIComponent(filter === undefined ? app.apps[card].tabs[tab].filter : filter) + '/' +
                encodeURIComponent(sort === undefined ? app.apps[card].tabs[tab].sort : sort) + '/' +
                encodeURIComponent(tag === undefined ? app.apps[card].tabs[tab].tag : tag) + '/' +
                encodeURIComponent(search === undefined ? app.apps[card].tabs[tab].search : search);
            if (newScrollPos !== undefined) {
                app.apps[card].tabs[tab].scrollPos = newScrollPos;
            }
        }
    }
    else {
        hash = '/' + encodeURIComponent(card) + '!' + 
            encodeURIComponent(offset === undefined ? app.apps[card].offset : offset) + '/' +
            encodeURIComponent(limit === undefined ? app.apps[card].limit : limit) + '/' +
            encodeURIComponent(filter === undefined ? app.apps[card].filter : filter) + '/' +
            encodeURIComponent(sort === undefined ? app.apps[card].sort : sort) + '/' +
            encodeURIComponent(tag === undefined ? app.apps[card].tag : tag) + '/' +
            encodeURIComponent(search === undefined ? app.apps[card].search : search);
        if (newScrollPos !== undefined) {
            app.apps[card].scrollPos = newScrollPos;
        }
    }
    location.hash = hash;
}

function appRoute() {
    //called on hash change
    if (settingsParsed === false) {
        appInitStart();
        return;
    }
    let hash = location.hash;
    let params = hash.match(/^#\/(\w+)\/?(\w+)?\/?(\w+)?!(\d+)\/(\d+)\/([^/]+)\/([^/]+)\/([^/]+)\/(.*)$/);
    if (params) {
        app.current.app = decodeURIComponent(params[1]);
        app.current.tab = params[2] !== undefined ? decodeURIComponent(params[2]) : undefined;
        app.current.view = params[3] !== undefined ? decodeURIComponent(params[3]) : undefined;
        app.current.offset = parseInt(decodeURIComponent(params[4]));
        app.current.limit = parseInt(decodeURIComponent(params[5]));
        app.current.filter = decodeURIComponent(params[6]);
        app.current.sort = decodeURIComponent(params[7]);
        app.current.tag = decodeURIComponent(params[8]);
        app.current.search = decodeURIComponent(params[9]);
        
        if (app.apps[app.current.app].offset !== undefined) {
            app.apps[app.current.app].offset = app.current.offset;
            app.apps[app.current.app].limit = app.current.limit;
            app.apps[app.current.app].filter = app.current.filter;
            app.apps[app.current.app].sort = app.current.sort;
            app.apps[app.current.app].tag = app.current.tag;
            app.apps[app.current.app].search = app.current.search;
            app.current.scrollPos = app.apps[app.current.app].scrollPos;
        }
        else if (app.apps[app.current.app].tabs[app.current.tab].offset !== undefined) {
            app.apps[app.current.app].tabs[app.current.tab].offset = app.current.offset;
            app.apps[app.current.app].tabs[app.current.tab].limit = app.current.limit;
            app.apps[app.current.app].tabs[app.current.tab].filter = app.current.filter;
            app.apps[app.current.app].tabs[app.current.tab].sort = app.current.sort;
            app.apps[app.current.app].tabs[app.current.tab].tag = app.current.tag;
            app.apps[app.current.app].tabs[app.current.tab].search = app.current.search;
            app.apps[app.current.app].active = app.current.tab;
            app.current.scrollPos = app.apps[app.current.app].tabs[app.current.tab].scrollPos;
        }
        else if (app.apps[app.current.app].tabs[app.current.tab].views[app.current.view].offset !== undefined) {
            app.apps[app.current.app].tabs[app.current.tab].views[app.current.view].offset = app.current.offset;
            app.apps[app.current.app].tabs[app.current.tab].views[app.current.view].limit = app.current.limit;
            app.apps[app.current.app].tabs[app.current.tab].views[app.current.view].filter = app.current.filter;
            app.apps[app.current.app].tabs[app.current.tab].views[app.current.view].sort = app.current.sort;
            app.apps[app.current.app].tabs[app.current.tab].views[app.current.view].tag = app.current.tag;
            app.apps[app.current.app].tabs[app.current.tab].views[app.current.view].search = app.current.search;
            app.apps[app.current.app].active = app.current.tab;
            app.apps[app.current.app].tabs[app.current.tab].active = app.current.view;
            app.current.scrollPos = app.apps[app.current.app].tabs[app.current.tab].views[app.current.view].scrollPos;
        }
    }
    else {
        appPrepare(0);
        if (settings.featHome === true) {
            appGoto('Home');
        }
        else {
            appGoto('Playback');
        }
        return;
    }
    appPrepare(app.current.scrollPos);

    if (app.current.app === 'Home') {
        sendAPI("MYMPD_API_HOME_LIST", {}, parseHome);
    }
    else if (app.current.app === 'Playback') {
        sendAPI("MPD_API_PLAYER_CURRENT_SONG", {}, songChange);
    }    
    else if (app.current.app === 'Queue' && app.current.tab === 'Current' ) {
        selectTag('searchqueuetags', 'searchqueuetagsdesc', app.current.filter);
        getQueue();
    }
    else if (app.current.app === 'Queue' && app.current.tab === 'LastPlayed') {
        sendAPI("MPD_API_QUEUE_LAST_PLAYED", {"offset": app.current.offset, "limit": app.current.limit, "cols": settings.colsQueueLastPlayed}, parseLastPlayed);
    }
    else if (app.current.app === 'Queue' && app.current.tab === 'Jukebox') {
        sendAPI("MPD_API_JUKEBOX_LIST", {"offset": app.current.offset, "limit": app.current.limit, "cols": settings.colsQueueJukebox}, parseJukeboxList);
    }
    else if (app.current.app === 'Browse' && app.current.tab === 'Playlists' && app.current.view === 'All') {
        sendAPI("MPD_API_PLAYLIST_LIST", {"offset": app.current.offset, "limit": app.current.limit, "searchstr": app.current.search}, parsePlaylists);
        const searchPlaylistsStrEl = document.getElementById('searchPlaylistsStr');
        if (searchPlaylistsStrEl.value === '' && app.current.search !== '') {
            searchPlaylistsStrEl.value = app.current.search;
        }
    }
    else if (app.current.app === 'Browse' && app.current.tab === 'Playlists' && app.current.view === 'Detail') {
        sendAPI("MPD_API_PLAYLIST_CONTENT_LIST", {"offset": app.current.offset, "limit": app.current.limit, "searchstr": app.current.search, "uri": app.current.filter, "cols": settings.colsBrowsePlaylistsDetail}, parsePlaylists);
        const searchPlaylistsStrEl = document.getElementById('searchPlaylistsStr');
        if (searchPlaylistsStrEl.value === '' && app.current.search !== '') {
            searchPlaylistsStrEl.value = app.current.search;
        }
    }    
    else if (app.current.app === 'Browse' && app.current.tab === 'Filesystem') {
        sendAPI("MPD_API_DATABASE_FILESYSTEM_LIST", {"offset": app.current.offset, "limit": app.current.limit, "path": (app.current.search ? app.current.search : "/"), 
            "searchstr": (app.current.filter !== '-' ? app.current.filter : ''), "cols": settings.colsBrowseFilesystem}, parseFilesystem, true);
        // Don't add all songs from root
        if (app.current.search) {
            enableEl('BrowseFilesystemAddAllSongs');
            enableEl('BrowseFilesystemAddAllSongsBtn');
        }
        else {
            disableEl('BrowseFilesystemAddAllSongs');
            disableEl('BrowseFilesystemAddAllSongsBtn');
        }
        // Create breadcrumb
        let breadcrumbs='<li class="breadcrumb-item"><a data-uri="" class="text-body mi">home</a></li>';
        let pathArray = app.current.search.split('/');
        let pathArrayLen = pathArray.length;
        let fullPath = '';
        for (let i = 0; i < pathArrayLen; i++) {
            if (pathArrayLen - 1 === i) {
                breadcrumbs += '<li class="breadcrumb-item active">' + e(pathArray[i]) + '</li>';
                break;
            }
            fullPath += pathArray[i];
            breadcrumbs += '<li class="breadcrumb-item"><a class="text-body" href="#" data-uri="' + encodeURI(fullPath) + '">' + e(pathArray[i]) + '</a></li>';
            fullPath += '/';
        }
        document.getElementById('BrowseBreadcrumb').innerHTML = breadcrumbs;
        const searchFilesystemStrEl = document.getElementById('searchFilesystemStr');
        searchFilesystemStrEl.value = app.current.filter === '-' ? '' :  app.current.filter;
    }
    else if (app.current.app === 'Browse' && app.current.tab === 'Database' && app.current.view === 'List') {
        document.getElementById('viewListDatabase').classList.remove('hide');
        document.getElementById('viewDetailDatabase').classList.add('hide');
        selectTag('searchDatabaseTags', 'searchDatabaseTagsDesc', app.current.filter);
        selectTag('BrowseDatabaseByTagDropdown', 'btnBrowseDatabaseByTagDesc', app.current.tag);
        let sort = app.current.sort;
        let sortdesc = false;
        if (app.current.sort.charAt(0) === '-') {
            sortdesc = true;
            sort = app.current.sort.substr(1);
            toggleBtnChk('databaseSortDesc', true);
        }
        else {
            toggleBtnChk('databaseSortDesc', false);
        }
        selectTag('databaseSortTags', undefined, sort);
        if (app.current.tag === 'Album') {
            createSearchCrumbs(app.current.search, document.getElementById('searchDatabaseStr'), document.getElementById('searchDatabaseCrumb'));
            document.getElementById('searchDatabaseMatch').classList.remove('hide');
            enableEl('btnDatabaseSortDropdown');
            enableEl('btnDatabaseSearchDropdown');
            sendAPI("MPD_API_DATABASE_GET_ALBUMS", {"offset": app.current.offset, "limit": app.current.limit, "searchstr": app.current.search, 
                "filter": app.current.filter, "sort": sort, "sortdesc": sortdesc}, parseDatabase);
        }
        else {
            document.getElementById('searchDatabaseCrumb').classList.add('hide');
            document.getElementById('searchDatabaseMatch').classList.add('hide');
            disableEl('btnDatabaseSortDropdown');
            disableEl('btnDatabaseSearchDropdown');
            document.getElementById('searchDatabaseStr').value = app.current.search;
            sendAPI("MPD_API_DATABASE_TAG_LIST", {"offset": app.current.offset, "limit": app.current.limit, "searchstr": app.current.search, 
                "filter": app.current.filter, "sort": sort, "sortdesc": sortdesc, "tag": app.current.tag}, parseDatabase);
        }
    }
    else if (app.current.app === 'Browse' && app.current.tab === 'Database' && app.current.view === 'Detail') {
        document.getElementById('viewListDatabase').classList.add('hide');
        document.getElementById('viewDetailDatabase').classList.remove('hide');
        if (app.current.filter === 'Album') {
            let cols = settings.colsBrowseDatabaseDetail.slice();
            if (cols.includes('Disc') === false) {
                cols.push('Disc');
            }
            sendAPI("MPD_API_DATABASE_TAG_ALBUM_TITLE_LIST", {"album": app.current.tag,
                "searchstr": app.current.search,
                "tag": app.current.sort, "cols": cols}, parseAlbumDetails);
        }    
    }
    else if (app.current.app === 'Search') {
        domCache.searchstr.focus();
        if (settings.featAdvsearch) {
            createSearchCrumbs(app.current.search, domCache.searchstr, domCache.searchCrumb);
        }
        else if (domCache.searchstr.value === '' && app.current.search !== '') {
                domCache.searchstr.value = app.current.search;
        }
        
        if (app.last.app !== app.current.app && app.current.search !== '') {
            let colspan = settings['cols' + app.current.app].length;
            document.getElementById('SearchList').getElementsByTagName('tbody')[0].innerHTML=
                '<tr><td><span class="mi">search</span></td>' +
                '<td colspan="' + colspan + '">' + t('Searching...') + '</td></tr>';
        }

        if (domCache.searchstr.value.length >= 2 || domCache.searchCrumb.children.length > 0) {
            if (settings.featAdvsearch) {
                let sort = app.current.sort;
                let sortdesc = false;
                if (sort === '-') {
                    if (settings.tags.includes('Title')) {
                        sort = 'Title';
                    }
                    else {
                        sort = '-';
                    }
                    setAttEnc(document.getElementById('SearchList'), 'data-sort', sort);
                }
                else if (sort.indexOf('-') === 0) {
                    sortdesc = true;
                    sort = sort.substring(1);
                }
                sendAPI("MPD_API_DATABASE_SEARCH_ADV", {"plist": "", "offset": app.current.offset, "limit": app.current.limit, "sort": sort, "sortdesc": sortdesc, "expression": app.current.search, "cols": settings.colsSearch, "replace": false}, parseSearch);
            }
            else {
                sendAPI("MPD_API_DATABASE_SEARCH", {"plist": "", "offset": app.current.offset, "limit": app.current.limit, "filter": app.current.filter, "searchstr": app.current.search, "cols": settings.colsSearch, "replace": false}, parseSearch);
            }
        }
        else {
            document.getElementById('SearchList').getElementsByTagName('tbody')[0].innerHTML = '';
            disableEl('searchAddAllSongs');
            disableEl('searchAddAllSongsBtn');
            document.getElementById('SearchList').classList.remove('opacity05');
            setPagination(0, 0);
        }
        selectTag('searchtags', 'searchtagsdesc', app.current.filter);
    }
    else {
        appGoto("Home");
    }

    app.last.app = app.current.app;
    app.last.tab = app.current.tab;
    app.last.view = app.current.view;
}

function showAppInitAlert(text) {
    document.getElementById('splashScreenAlert').innerHTML = '<p class="text-danger">' + t(text) + '</p>' +
        '<p><a id="appReloadBtn" class="btn btn-danger text-light clickable">' + t('Reload') + '</a></p>';
    document.getElementById('appReloadBtn').addEventListener('click', function() {
        clearAndReload();
    }, false);
}


function clearAndReload() {
    if ('serviceWorker' in navigator) {
        caches.keys().then(function(cacheNames) {
            cacheNames.forEach(function(cacheName) {
                caches.delete(cacheName);
            });
        });
    }
    location.reload();
}

function a2hsInit() {
    window.addEventListener('beforeinstallprompt', function(event) {
        logDebug('Event: beforeinstallprompt');
        // Prevent Chrome 67 and earlier from automatically showing the prompt
        event.preventDefault();
        // Stash the event so it can be triggered later
        deferredA2HSprompt = event;
        // Update UI notify the user they can add to home screen
        document.getElementById('nav-add2homescreen').classList.remove('hide');
    });

    document.getElementById('nav-add2homescreen').addEventListener('click', function(event) {
        // Hide our user interface that shows our A2HS button
        event.target.classList.add('hide');
        // Show the prompt
        deferredA2HSprompt.prompt();
        // Wait for the user to respond to the prompt
        deferredA2HSprompt.userChoice.then((choiceResult) => {
            choiceResult.outcome === 'accepted' ? logDebug('User accepted the A2HS prompt') : logDebug('User dismissed the A2HS prompt');
            deferredA2HSprompt = null;
        });
    });
    
    window.addEventListener('appinstalled', function() {
        logInfo('myMPD installed as app');
        showNotification(t('myMPD installed as app'), '', '', 'success');
    });
}

function appInitStart() {
    //add app routing event handler
    window.addEventListener('hashchange', appRoute, false);

    //set initial scale
    if (isMobile === true) {
        scale = localStorage.getItem('scale-ratio');
        if (scale === null) {
            scale = '1.0';
        }
        setViewport(false);
    }
    else {
        let m = document.getElementsByClassName('featMobile');
        for (let i = 0; i < m.length; i++) {
            m[i].classList.add('hide');
        }        
    }

    subdir = window.location.pathname.replace('/index.html', '').replace(/\/$/, '');
    let localeList = '<option value="default" data-phrase="Browser default"></option>';
    for (let i = 0; i < locales.length; i++) {
        localeList += '<option value="' + e(locales[i].code) + '">' + e(locales[i].desc) + ' (' + e(locales[i].code) + ')</option>';
    }
    document.getElementById('selectLocale').innerHTML = localeList;
    
    i18nHtml(document.getElementById('splashScreenAlert'));
    
    //set loglevel
    let script = document.getElementsByTagName("script")[0].src.replace(/^.*[/]/, '');
    if (script !== 'combined.js') {
        settings.loglevel = 4;
    }
    //register serviceworker
    if ('serviceWorker' in navigator && window.location.protocol === 'https:' 
        && window.location.hostname !== 'localhost' && script === 'combined.js')
    {
        window.addEventListener('load', function() {
            navigator.serviceWorker.register('/sw.js', {scope: '/'}).then(function(registration) {
                // Registration was successful
                logInfo('ServiceWorker registration successful.');
                registration.update();
            }, function(err) {
                // Registration failed
                logError('ServiceWorker registration failed: ' + err);
            });
        });
    }

    appInited = false;
    document.getElementById('splashScreen').classList.remove('hide');
    domCache.body.classList.add('overflow-hidden');
    document.getElementById('splashScreenAlert').innerText = t('Fetch myMPD settings');

    a2hsInit();

    getSettings(true);
    appInitWait();
}

function appInitWait() {
    setTimeout(function() {
        if (settingsParsed === 'true' && websocketConnected === true) {
            //app initialized
            document.getElementById('splashScreenAlert').innerText = t('Applying settings');
            document.getElementById('splashScreen').classList.add('hide-fade');
            setTimeout(function() {
                document.getElementById('splashScreen').classList.add('hide');
                document.getElementById('splashScreen').classList.remove('hide-fade');
                domCache.body.classList.remove('overflow-hidden');
            }, 500);
            appInit();
            appInited = true;
            return;
        }
        
        if (settingsParsed === 'true') {
            //parsed settings, now its save to connect to websocket
            document.getElementById('splashScreenAlert').innerText = t('Connect to websocket');
            webSocketConnect();
        }
        else if (settingsParsed === 'error') {
            return;
        }
        appInitWait();
    }, 500);
}

function appInit() {
    //collaps arrows for submenus
    let collapseArrows = document.querySelectorAll('.subMenu');
    let collapseArrowsLen = collapseArrows.length;
    for (let i = 0; i < collapseArrowsLen; i++) {
        collapseArrows[i].addEventListener('click', function(event) {
            event.stopPropagation();
            event.preventDefault();
            let icon = this.getElementsByTagName('span')[0];
            icon.innerText = icon.innerText === 'keyboard_arrow_right' ? 'keyboard_arrow_down' : 'keyboard_arrow_right';
        }, false);
    }    
    //align dropdowns
    let dropdowns = document.querySelectorAll('.dropdown-toggle');
    for (let i = 0; i < dropdowns.length; i++) {
        dropdowns[i].parentNode.addEventListener('show.bs.dropdown', function () {
            alignDropdown(this);
        });
    }
    //init links
    let hrefs = document.querySelectorAll('[data-href]');
    let hrefsLen = hrefs.length;
    for (let i = 0; i < hrefsLen; i++) {
        if (hrefs[i].classList.contains('notclickable') === false) {
            hrefs[i].classList.add('clickable');
        }
        let parentInit = hrefs[i].parentNode.classList.contains('noInitChilds') ? true : false;
        if (parentInit === false) {
            parentInit = hrefs[i].parentNode.parentNode.classList.contains('noInitChilds') ? true : false;
        }
        if (parentInit === true) {
            //handler on parentnode
            continue;
        }
        hrefs[i].addEventListener('click', function(event) {
            parseCmd(event, getAttDec(this, 'data-href'));
        }, false);
    }
    //do not submit forms
    const noFormSubmit = ['search', 'searchqueue', 'searchdatabase'];
    for (let i = 0; i < noFormSubmit.length; i++) {
        document.getElementById(noFormSubmit[i]).addEventListener('submit', function(event) {
            event.preventDefault();
        }, false);
    }
    //hide popover
    domCache.body.addEventListener('click', function() {
        hideMenu();
    }, false);
    //init moduls
    initGlobalModals();
    initSong();
    initHome();
    initBrowse();
    initQueue();
    initSearch();
    initScripts();
    initTrigger();
    initTimer();
    initPartitions();
    initMounts();
    initLocalplayer();
    initSettings();
    initPlayback();
    initNavs();
    initPlaylists();
    //init drag and drop
    dragAndDropTable('QueueCurrentList');
    dragAndDropTable('BrowsePlaylistsDetailList');
    dragAndDropTableHeader('QueueCurrent');
    dragAndDropTableHeader('QueueLastPlayed');
    dragAndDropTableHeader('QueueJukebox');
    dragAndDropTableHeader('Search');
    dragAndDropTableHeader('BrowseFilesystem');
    dragAndDropTableHeader('BrowsePlaylistsDetail');
    dragAndDropTableHeader('BrowseDatabaseDetail');
    //update state on window focus - browser pauses javascript
    window.addEventListener('focus', function() {
        sendAPI("MPD_API_PLAYER_STATE", {}, parseState);
    }, false);
    //global keymap
    document.addEventListener('keydown', function(event) {
        if (event.target.tagName === 'INPUT' || event.target.tagName === 'SELECT' ||
            event.target.tagName === 'TEXTAREA' || event.ctrlKey || event.altKey) {
            return;
        }
        let cmd = keymap[event.key];
        if (cmd && typeof window[cmd.cmd] === 'function') {
            if (keymap[event.key].req === undefined || settings[keymap[event.key].req] === true)
                parseCmd(event, cmd);
        }        
        
    }, false);
    //make tables navigateable by keyboard
    let tables = document.getElementsByTagName('table');
    for (let i = 0; i < tables.length; i++) {
        tables[i].setAttribute('tabindex', 0);
        tables[i].addEventListener('keydown', function(event) {
            navigateTable(this, event.key);
        }, false);
    }
    //contextmenu for tables
    tables = ['BrowseFilesystemList', 'BrowseDatabaseDetailList', 'QueueCurrentList', 'QueueLastPlayedList', 
        'QueueJukeboxList', 'SearchList', 'BrowsePlaylistsAllList', 'BrowsePlaylistsDetailList'];
    for (let i = 0; i < tables.length; i++) {
        document.getElementById(tables[i]).getElementsByTagName('tbody')[0].addEventListener('long-press', function(event) {
            if (event.target.parentNode.classList.contains('not-clickable') || getAttDec(event.target.parentNode, 'data-type') === 'parentDir') {
                return;
            }
            showMenu(event.target, event);
            event.preventDefault();
            event.stopPropagation();
        }, false);
    
        document.getElementById(tables[i]).getElementsByTagName('tbody')[0].addEventListener('contextmenu', function(event) {
            if (event.target.parentNode.classList.contains('not-clickable') || getAttDec(event.target.parentNode, 'data-type') === 'parentDir') {
                return;
            }
            showMenu(event.target, event);
            event.preventDefault();
            event.stopPropagation();
        }, false);
    }

    //websocket
    window.addEventListener('beforeunload', function() {
        webSocketClose();
    });
}

function initGlobalModals() {
    document.getElementById('modalAbout').addEventListener('shown.bs.modal', function () {
        sendAPI("MPD_API_DATABASE_STATS", {}, parseStats);
        getServerinfo();
        let list = '';
        let i = 0;
        for (let key in keymap) {
            if (i === 0 || i % 2 === 0) {
                if (i > 0) {
                    list += '</div>';
                }
                list += '<div class="row row-keymap">';
            }
            if (keymap[key].req === undefined || settings[keymap[key].req] === true) {
                list += '<div class="col col-keymap mb-1 d-flex"><div class="align-self-center key' + (keymap[key].key && keymap[key].key.length > 1 ? ' mi mi-small' : '') + 
                       '">' + (keymap[key].key !== undefined ? keymap[key].key : key ) + '</div><div class="align-self-center">' + t(keymap[key].desc) + '</div></div>';
                i++;
            }
        }
        document.getElementById('shortcutList').innerHTML = list + '</div>';
    });
    
    document.getElementById('modalUpdateDB').addEventListener('hidden.bs.modal', function () {
        document.getElementById('updateDBprogress').classList.remove('updateDBprogressAnimate');
    });
}

function initPlayback() {
    let colDropdowns = ['PlaybackColsDropdown'];
    for (let i = 0; i < colDropdowns.length; i++) {
        document.getElementById(colDropdowns[i]).addEventListener('click', function(event) {
            if (event.target.nodeName === 'BUTTON' && event.target.classList.contains('mi')) {
                event.stopPropagation();
                event.preventDefault();
                toggleBtnChk(event.target);
            }
        }, false);
    }

    document.getElementById('cardPlaybackTags').addEventListener('click', function(event) {
        if (event.target.nodeName === 'P') {
            gotoBrowse(event);
        }
    }, false);
    
    //quick plaback settings dropdown
    document.getElementById('playDropdown').parentNode.addEventListener('show.bs.dropdown', function () {
        showPlayDropdown();
    });

    document.getElementById('playDropdown').addEventListener('click', function (event) {
        event.preventDefault();
        event.stopPropagation();
    });
}

function initNavs() {
    document.getElementById('mainMenu').addEventListener('click', function(event) {
        event.preventDefault();
    }, false);

    document.getElementById('btnChVolumeDown').addEventListener('click', function(event) {
        event.stopPropagation();
    }, false);
    document.getElementById('btnChVolumeUp').addEventListener('click', function(event) {
        event.stopPropagation();
    }, false);

    domCache.volumeBar.addEventListener('click', function(event) {
        event.stopPropagation();
    }, false);
    domCache.volumeBar.addEventListener('change', function() {
        sendAPI("MPD_API_PLAYER_VOLUME_SET", {"volume": domCache.volumeBar.value});
    }, false);

    domCache.progress.addEventListener('click', function(event) {
        if (currentSong && currentSong.currentSongId >= 0 && currentSong.totalTime > 0) {
            domCache.progressBar.style.transition = 'none';
            domCache.progressBar.style.width = event.clientX + 'px';
            setTimeout(function() {
                domCache.progressBar.style.transition = progressBarTransition;
            }, 10);
            const seekVal = Math.ceil((currentSong.totalTime * event.clientX) / domCache.progress.offsetWidth);
            sendAPI("MPD_API_PLAYER_SEEK", {"songid": currentSong.currentSongId, "seek": seekVal});
        }
    }, false);

    domCache.progress.addEventListener('mousemove', function(event) {
        if ((playstate === 'pause' || playstate === 'play') && currentSong.totalTime > 0) {
            domCache.progressPos.innerText = beautifySongDuration(Math.ceil((currentSong.totalTime / event.target.offsetWidth) * event.clientX));
            domCache.progressPos.style.display = 'block';
            const w = domCache.progressPos.offsetWidth / 2;
            const posX = event.clientX < w ? event.clientX : (event.clientX < window.innerWidth - w ? event.clientX - w : event.clientX - (w * 2));
            domCache.progressPos.style.left = posX + 'px';
        }
    }, false);

    domCache.progress.addEventListener('mouseout', function() {
        domCache.progressPos.style.display = 'none';
    }, false);
    document.getElementById('navbar-main').addEventListener('click', function(event) {
        event.preventDefault();
        let href = getAttDec(event.target, 'data-href');
        if (href === null) {
            href = getAttDec(event.target.parentNode, 'data-href');
        }
        if (href !== null) {
            parseCmd(event, href);
        }
    }, false);
    
    document.getElementById('volumeMenu').parentNode.addEventListener('show.bs.dropdown', function () {
        sendAPI("MPD_API_PLAYER_OUTPUT_LIST", {}, parseOutputs);
    });

    document.getElementById('outputs').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            event.stopPropagation();
            event.preventDefault();
            sendAPI("MPD_API_PLAYER_TOGGLE_OUTPUT", {"output": getAttDec(event.target, 'data-output-id'), "state": (event.target.classList.contains('active') ? 0 : 1)});
            toggleBtn(event.target.id);
        }
        else if (event.target.nodeName === 'A') {
            event.preventDefault();
            showListOutputAttributes(getAttDec(event.target.parentNode, 'data-output-name'));
        }
    }, false);

    document.getElementById('syscmds').addEventListener('click', function(event) {
        if (event.target.nodeName === 'A') {
            parseCmd(event, getAttDec(event.target, 'data-href'));
        }
    }, false);
    
    document.getElementById('scripts').addEventListener('click', function(event) {
        if (event.target.nodeName === 'A') {
            execScript(getAttDec(event.target, 'data-href'));
        }
    }, false);
}

//Handle javascript errors
window.onerror = function(msg, url, line) {
    logError('JavaScript error: ' + msg + ' (' + url + ': ' + line + ')');
    if (settings.loglevel >= 4) {
        if (appInited === true) {
            showNotification(t('JavaScript error'), msg + ' (' + url + ': ' + line + ')', '', 'danger');
        }
        else {
            showAppInitAlert(t('JavaScript error') + ': ' + msg + ' (' + url + ': ' + line + ')');
        }
    }
    return true;
};
//Start app
appInitStart();
