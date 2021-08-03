"use strict";
// SPDX-License-Identifier: GPL-2.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/* eslint-enable no-unused-vars */
function appPrepare(scrollPos) {
    if (app.current.app !== app.last.app || app.current.tab !== app.last.tab || app.current.view !== app.last.view) {
        //Hide all cards + nav
        for (let i = 0; i < domCache.navbarBtnsLen; i++) {
            domCache.navbarBtns[i].classList.remove('active');
        }
        const cards = ['cardHome', 'cardPlayback', 'cardSearch',
            'cardQueue', 'tabQueueCurrent', 'tabQueueLastPlayed', 'tabQueueJukebox', 
            'cardBrowse', 'tabBrowseFilesystem', 
            'tabBrowsePlaylists', 'viewBrowsePlaylistsDetail', 'viewBrowsePlaylistsList', 
            'tabBrowseDatabase', 'viewBrowseDatabaseDetail', 'viewBrowseDatabaseList'];
        for (const card of cards) {
            document.getElementById(card).classList.add('hide');
        }
        //show active card
        document.getElementById('card' + app.current.app).classList.remove('hide');
        //show active tab
        if (app.current.tab !== undefined) {
            document.getElementById('tab' + app.current.app + app.current.tab).classList.remove('hide');
        }
        //show active view
        if (app.current.view !== undefined) {
            document.getElementById('view' + app.current.app + app.current.tab + app.current.view).classList.remove('hide');
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
    //old app
    const oldptr = app.apps[app.current.app].offset !== undefined ? app.apps[app.current.app] :
        app.apps[app.current.app].tabs[app.current.tab].offset !== undefined ? app.apps[app.current.app].tabs[app.current.tab] :
            app.apps[app.current.app].tabs[app.current.tab].views[app.current.view];

    //get default active tab or view from state
    if (app.apps[card].tabs) {
        if (tab === undefined) {
            tab = app.apps[card].active;
        }
        if (app.apps[card].tabs[tab].views) {
            if (view === undefined) {
                view = app.apps[card].tabs[tab].active;
            }
        }
    }

    //get ptr to new app
    const ptr = app.apps[card].offset !== undefined ? app.apps[card] :
                app.apps[card].tabs[tab].offset !== undefined ? app.apps[card].tabs[tab] :
                app.apps[card].tabs[tab].views[view];
                
    //save scrollPos of old app
    if (oldptr !== ptr) {
        oldptr.scrollPos = document.body.scrollTop ? document.body.scrollTop : document.documentElement.scrollTop;
    }
    
    //set options to default, if not defined
    if (offset === null || offset === undefined) { offset = ptr.offset; }
    if (limit === null || limit === undefined)   { limit = ptr.limit; }
    if (filter === null || filter === undefined) { filter = ptr.filter; }
    if (sort === null || sort === undefined)     { sort = ptr.sort; }
    if (tag === null || tag === undefined)       { tag = ptr.tag; }
    if (search === null || search === undefined) { search = ptr.search; }
    //set new scrollpos
    if (newScrollPos !== undefined) {
        ptr.scrollPos = newScrollPos;
    }
    //build hash
    location.hash = '/' + encodeURIComponent(card) + (tab === undefined ? '' : '/' + encodeURIComponent(tab) + 
        (view === undefined ? '' : '/' + encodeURIComponent(view))) + '!' + 
        encodeURIComponent(offset) + '/' + encodeURIComponent(limit) + '/' + encodeURIComponent(filter) + '/' + 
        encodeURIComponent(sort) + '/' + encodeURIComponent(tag) + '/' + encodeURIComponent(search);
}

function appRoute() {
    //called on hash change
    if (settingsParsed === false) {
        appInitStart();
        return;
    }
    const params = location.hash.match(/^#\/(\w+)\/?(\w+)?\/?(\w+)?!(\d+)\/(\d+)\/([^/]+)\/([^/]+)\/([^/]+)\/(.*)$/);
    if (params) {
        app.current.app = decodeURIComponent(params[1]);
        app.current.tab = params[2] !== undefined ? decodeURIComponent(params[2]) : undefined;
        app.current.view = params[3] !== undefined ? decodeURIComponent(params[3]) : undefined;
        app.current.offset = Number(decodeURIComponent(params[4]));
        app.current.limit = Number(decodeURIComponent(params[5]));
        app.current.filter = decodeURIComponent(params[6]);
        app.current.sort = decodeURIComponent(params[7]);
        app.current.tag = decodeURIComponent(params[8]);
        app.current.search = decodeURIComponent(params[9]);

        //get ptr to app options and set active tab/view        
        let ptr;
        if (app.apps[app.current.app].offset !== undefined) {
            ptr = app.apps[app.current.app];
        }
        else if (app.apps[app.current.app].tabs[app.current.tab].offset !== undefined) {
            ptr = app.apps[app.current.app].tabs[app.current.tab];
            app.apps[app.current.app].active = app.current.tab;
        }
        else if (app.apps[app.current.app].tabs[app.current.tab].views[app.current.view].offset !== undefined) {
            ptr = app.apps[app.current.app].tabs[app.current.tab].views[app.current.view];
            app.apps[app.current.app].active = app.current.tab;
            app.apps[app.current.app].tabs[app.current.tab].active = app.current.view;
        }
        //set app options
        ptr.offset = app.current.offset;
        ptr.limit = app.current.limit;
        ptr.filter = app.current.filter;
        ptr.sort = app.current.sort;
        ptr.tag = app.current.tag;
        ptr.search = app.current.search;
        app.current.scrollPos = ptr.scrollPos;
    }
    else {
        appPrepare(0);
        if (features.featHome === true) {
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
        sendAPI("MYMPD_API_PLAYER_CURRENT_SONG", {}, songChange);
    }    
    else if (app.current.app === 'Queue' && app.current.tab === 'Current' ) {
        selectTag('searchqueuetags', 'searchqueuetagsdesc', app.current.filter);
        getQueue();
    }
    else if (app.current.app === 'Queue' && app.current.tab === 'LastPlayed') {
        sendAPI("MYMPD_API_QUEUE_LAST_PLAYED", {
            "offset": app.current.offset,
            "limit": app.current.limit,
            "cols": settings.colsQueueLastPlayed
        }, parseLastPlayed);
    }
    else if (app.current.app === 'Queue' && app.current.tab === 'Jukebox') {
        sendAPI("MYMPD_API_JUKEBOX_LIST", {
            "offset": app.current.offset,
            "limit": app.current.limit,
            "cols": settings.colsQueueJukebox
        }, parseJukeboxList);
    }
    else if (app.current.app === 'Browse' && app.current.tab === 'Playlists' && app.current.view === 'List') {
        sendAPI("MYMPD_API_PLAYLIST_LIST", {
            "offset": app.current.offset,
            "limit": app.current.limit,
            "searchstr": app.current.search
        }, parsePlaylistsList);
        const searchPlaylistsStrEl = document.getElementById('searchPlaylistsListStr');
        if (searchPlaylistsStrEl.value === '' && app.current.search !== '') {
            searchPlaylistsStrEl.value = app.current.search;
        }
    }
    else if (app.current.app === 'Browse' && app.current.tab === 'Playlists' && app.current.view === 'Detail') {
        sendAPI("MYMPD_API_PLAYLIST_CONTENT_LIST", {
            "offset": app.current.offset,
            "limit": app.current.limit,
            "searchstr": app.current.search,
            "plist": app.current.filter,
            "cols": settings.colsBrowsePlaylistsDetail
        }, parsePlaylistsDetail);
        const searchPlaylistsStrEl = document.getElementById('searchPlaylistsDetailStr');
        if (searchPlaylistsStrEl.value === '' && app.current.search !== '') {
            searchPlaylistsStrEl.value = app.current.search;
        }
    }    
    else if (app.current.app === 'Browse' && app.current.tab === 'Filesystem') {
        sendAPI("MYMPD_API_DATABASE_FILESYSTEM_LIST", {
            "offset": app.current.offset,
            "limit": app.current.limit,
            "path": (app.current.search ? app.current.search : "/"), 
            "searchstr": (app.current.filter !== '-' ? app.current.filter : ''),
            "cols": settings.colsBrowseFilesystem
        }, parseFilesystem, true);
        //Don't add all songs from root
        if (app.current.search) {
            elEnable('BrowseFilesystemAddAllSongs');
            elEnable('BrowseFilesystemAddAllSongsBtn');
        }
        else {
            elDisable('BrowseFilesystemAddAllSongs');
            elDisable('BrowseFilesystemAddAllSongsBtn');
        }
        //Create breadcrumb
        let breadcrumbs = '<li class="breadcrumb-item"><a data-uri="" class="text-body mi">home</a></li>';
        const pathArray = app.current.search.split('/');
        const pathArrayLen = pathArray.length;
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
            elEnable('btnDatabaseSortDropdown');
            elEnable('btnDatabaseSearchDropdown');
            sendAPI("MYMPD_API_DATABASE_GET_ALBUMS", {
                "offset": app.current.offset,
                "limit": app.current.limit,
                "expression": app.current.search, 
                "sort": sort,
                "sortdesc": sortdesc
            }, parseDatabase);
        }
        else {
            document.getElementById('searchDatabaseCrumb').classList.add('hide');
            document.getElementById('searchDatabaseMatch').classList.add('hide');
            elDisable('btnDatabaseSortDropdown');
            elDisable('btnDatabaseSearchDropdown');
            document.getElementById('searchDatabaseStr').value = app.current.search;
            sendAPI("MYMPD_API_DATABASE_TAG_LIST", {
                "offset": app.current.offset,
                "limit": app.current.limit,
                "searchstr": app.current.search, 
                "tag": app.current.tag
            }, parseDatabase);
        }
    }
    else if (app.current.app === 'Browse' && app.current.tab === 'Database' && app.current.view === 'Detail') {
        if (app.current.filter === 'Album') {
            const cols = settings.colsBrowseDatabaseDetail.slice();
            if (cols.includes('Disc') === false) {
                cols.push('Disc');
            }
            sendAPI("MYMPD_API_DATABASE_TAG_ALBUM_TITLE_LIST", {
                "album": app.current.tag,
                "albumartist": app.current.search,
                "cols": cols
            }, parseAlbumDetails);
        }    
    }
    else if (app.current.app === 'Search') {
        document.getElementById('searchstr').focus();
        if (features.featAdvsearch) {
            createSearchCrumbs(app.current.search, document.getElementById('searchstr'), document.getElementById('searchCrumb'));
        }
        else if (document.getElementById('searchstr').value === '' && app.current.search !== '') {
            document.getElementById('searchstr').value = app.current.search;
        }
        
        if (app.last.app !== app.current.app && app.current.search !== '') {
            document.getElementById('SearchList').getElementsByTagName('tbody')[0].innerHTML=
                '<tr><td><span class="mi">search</span></td>' +
                '<td colspan="' + settings['cols' + app.current.app].length + '">' + t('Searching...') + '</td></tr>';
        }

        if (document.getElementById('searchstr').value.length >= 2 || document.getElementById('searchCrumb').children.length > 0) {
            if (features.featAdvsearch) {
                let sort = app.current.sort;
                let sortdesc = false;
                if (sort === '-') {
                    sort = settings.tagList.includes('Title') ? 'Title' : '-';
                    setCustomDomProperty(document.getElementById('SearchList'), 'data-sort', sort);
                }
                else if (sort.indexOf('-') === 0) {
                    sortdesc = true;
                    sort = sort.substring(1);
                }
                sendAPI("MYMPD_API_DATABASE_SEARCH_ADV", {
                    "plist": "",
                    "offset": app.current.offset,
                    "limit": app.current.limit,
                    "sort": sort,
                    "sortdesc": sortdesc,
                    "expression": app.current.search,
                    "cols": settings.colsSearchActions,
                    "replace": false
                }, parseSearch);
            }
            else {
                sendAPI("MYMPD_API_DATABASE_SEARCH", {
                    "plist": "",
                    "offset": app.current.offset,
                    "limit": app.current.limit,
                    "filter": app.current.filter,
                    "searchstr": app.current.search,
                    "cols": settings.colsSearchActions,
                    "replace": false
                }, parseSearch);
            }
        }
        else {
            elClear(document.getElementById('SearchList').getElementsByTagName('tbody')[0]);
            elDisable('searchAddAllSongs');
            elDisable('searchAddAllSongsBtn');
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
            if (choiceResult.outcome === 'accepted') {
                logDebug('User accepted the A2HS prompt');
            }
            else {
                logDebug('User dismissed the A2HS prompt');
            }
            deferredA2HSprompt = null;
        });
    });
    
    window.addEventListener('appinstalled', function() {
        showNotification(t('myMPD installed as app'), '', 'general', 'info');
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
        const ms = document.getElementsByClassName('featMobile');
        for (const m of ms) {
            m.classList.add('hide');
        }        
    }

    subdir = window.location.pathname.replace('/index.html', '').replace(/\/$/, '');
    i18nHtml(document.getElementById('splashScreenAlert'));
    
    //set loglevel
    const script = document.getElementsByTagName("script")[0].src.replace(/^.*[/]/, '');
    if (script !== 'combined.js') {
        settings.loglevel = 4;
    }
    //register serviceworker
    if ('serviceWorker' in navigator && window.location.protocol === 'https:' && 
        window.location.hostname !== 'localhost' && script === 'combined.js')
    {
        window.addEventListener('load', function() {
            navigator.serviceWorker.register('/sw.js', {scope: '/'}).then(function(registration) {
                //Registration was successful
                logInfo('ServiceWorker registration successful.');
                registration.update();
            }, function(err) {
                //Registration failed
                logError('ServiceWorker registration failed: ' + err);
            });
        });
    }

    //show splash screen
    document.getElementById('splashScreen').classList.remove('hide');
    domCache.body.classList.add('overflow-hidden');
    document.getElementById('splashScreenAlert').textContent = t('Fetch myMPD settings');
    
    //init add to home screen feature
    a2hsInit();

    //initialize app
    appInited = false;
    settingsParsed = 'no';
    sendAPI("MYMPD_API_SETTINGS_GET", {}, function(obj) {
        parseSettings(obj, true);
        if (settingsParsed === 'parsed') {
            //connect to websocket in background
            setTimeout(function() {
                webSocketConnect();
            }, 0);
            //app initialized
            document.getElementById('splashScreenAlert').textContent = t('Applying settings');
            document.getElementById('splashScreen').classList.add('hide-fade');
            setTimeout(function() {
                document.getElementById('splashScreen').classList.add('hide');
                document.getElementById('splashScreen').classList.remove('hide-fade');
                domCache.body.classList.remove('overflow-hidden');
            }, 500);
            appInit();
            appInited = true;
            appRoute();
            logInfo('Startup duration: ' + (Date.now() - startTime) + 'ms');
        }
    }, true);
}

function appInit() {
    //collaps arrows for submenus
    const collapseArrows = document.querySelectorAll('.subMenu');
    for (const collapseArrow of collapseArrows) {
        collapseArrow.addEventListener('click', function(event) {
            event.stopPropagation();
            event.preventDefault();
            const icon = this.getElementsByTagName('span')[0];
            icon.textContent = icon.textContent === 'keyboard_arrow_right' ? 'keyboard_arrow_down' : 'keyboard_arrow_right';
            event.stopPropagation();
        }, false);
    }    
    //align dropdowns
    const dropdowns = document.querySelectorAll('.dropdown-toggle');
    for (const dropdown of dropdowns) {
        dropdown.parentNode.addEventListener('show.bs.dropdown', function () {
            alignDropdown(this);
        });
    }
    //init links
    const hrefs = document.querySelectorAll('[data-href]');
    for (const href of hrefs) {
        if (href.classList.contains('notclickable') === false) {
            href.classList.add('clickable');
        }
        let parentInit = href.parentNode.classList.contains('noInitChilds') ? true : false;
        if (parentInit === false) {
            parentInit = href.parentNode.parentNode.classList.contains('noInitChilds') ? true : false;
        }
        if (parentInit === true) {
            //handler on parentnode
            continue;
        }
        href.addEventListener('click', function(event) {
            parseCmd(event, getCustomDomProperty(this, 'data-href'));
        }, false);
    }
    //do not submit forms
    const noFormSubmit = ['search', 'searchqueue', 'searchdatabase'];
    for (const formName of noFormSubmit) {
        document.getElementById(formName).addEventListener('submit', function(event) {
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
    initJukebox();
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
        logDebug('Browser tab gots the focus -> update player state');
        sendAPI("MYMPD_API_PLAYER_STATE", {}, parseState);
    }, false);
    //global keymap
    document.addEventListener('keydown', function(event) {
        if (event.target.tagName === 'INPUT' || event.target.tagName === 'SELECT' ||
            event.target.tagName === 'TEXTAREA' || event.ctrlKey || event.altKey || event.metaKey) {
            return;
        }
        const cmd = keymap[event.key];
        if (cmd && typeof window[cmd.cmd] === 'function') {
            if (keymap[event.key].req === undefined || settings[keymap[event.key].req] === true)
                parseCmd(event, cmd);
        }        
        
    }, false);
    //make tables navigateable by keyboard
    let tables = document.getElementsByTagName('table');
    for (const tableName of tables) {
        tableName.setAttribute('tabindex', 0);
        tableName.addEventListener('keydown', function(event) {
            navigateTable(this, event.key);
        }, false);
    }
    //contextmenu for tables
    tables = ['BrowseFilesystemList', 'BrowseDatabaseDetailList', 'QueueCurrentList', 'QueueLastPlayedList', 
        'QueueJukeboxList', 'SearchList', 'BrowsePlaylistsListList', 'BrowsePlaylistsDetailList'];
    for (const tableName of tables) {
        document.getElementById(tableName).getElementsByTagName('tbody')[0].addEventListener('long-press', function(event) {
            if (event.target.parentNode.classList.contains('not-clickable') || getCustomDomProperty(event.target.parentNode, 'data-type') === 'parentDir') {
                return;
            }
            showMenu(event.target, event);
            event.preventDefault();
            event.stopPropagation();
        }, false);
    
        document.getElementById(tableName).getElementsByTagName('tbody')[0].addEventListener('contextmenu', function(event) {
            if (event.target.parentNode.classList.contains('not-clickable') || getCustomDomProperty(event.target.parentNode, 'data-type') === 'parentDir') {
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
        sendAPI("MYMPD_API_DATABASE_STATS", {}, parseStats);
        getServerinfo();
        let list = '';
        let i = 0;
        for (const key in keymap) {
            if (i % 2 === 0) {
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
    document.getElementById('PlaybackColsDropdown').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON' && event.target.classList.contains('mi')) {
            event.stopPropagation();
            event.preventDefault();
            toggleBtnChk(event.target);
        }
    }, false);

    document.getElementById('cardPlaybackTags').addEventListener('click', function(event) {
        if (event.target.nodeName === 'P') {
            gotoBrowse(event);
        }
    }, false); 
}

function initNavs() {
    //do not hide volume menu on click on volume change buttons
    for (const elName of ['btnChVolumeDown', 'btnChVolumeUp', 'volumeBar']) {
        document.getElementById(elName).addEventListener('click', function(event) {
            event.stopPropagation();
        }, false);
    }

    //do not switch to first view by clicking on main menu logo
    document.getElementById('mainMenu').addEventListener('click', function(event) {
        event.preventDefault();
    }, false);
    
    //hides main menu after opening the modal
    document.getElementById('mainMenuDropdown').addEventListener('click', function() {
        uiElements.dropdownMainMenu.hide();
    }, false);

    document.getElementById('volumeBar').addEventListener('change', function() {
        sendAPI("MYMPD_API_PLAYER_VOLUME_SET", {"volume": Number(document.getElementById('volumeBar').value)});
    }, false);

    domCache.progress.addEventListener('click', function(event) {
        if (currentSong && currentSong.currentSongId >= 0 && currentSong.totalTime > 0) {
            const seekVal = Math.ceil((currentSong.totalTime * event.clientX) / domCache.progress.offsetWidth);
            sendAPI("MYMPD_API_PLAYER_SEEK_CURRENT", {"seek": seekVal, "relative": false});
            domCache.progressBar.style.transition = 'none';
            domCache.progressBar.offsetHeight;
            domCache.progressBar.style.width = event.clientX + 'px';
            setTimeout(function() {
                domCache.progressBar.style.transition = progressBarTransition;
                domCache.progressBar.offsetHeight;
            }, 1000);
        }
    }, false);

    domCache.progress.addEventListener('mousemove', function(event) {
        if ((playstate === 'pause' || playstate === 'play') && currentSong.totalTime > 0) {
            domCache.progressPos.textContent = beautifySongDuration(Math.ceil((currentSong.totalTime / event.target.offsetWidth) * event.clientX));
            domCache.progressPos.style.display = 'block';
            const w = domCache.progressPos.offsetWidth / 2;
            const posX = event.clientX < w ? event.clientX : (event.clientX < window.innerWidth - w ? event.clientX - w : event.clientX - (w * 2));
            domCache.progressPos.style.left = posX + 'px';
        }
    }, false);

    domCache.progress.addEventListener('mouseout', function() {
        domCache.progressPos.style.display = 'none';
    }, false);

    domCache.progressBar.style.transition = progressBarTransition;

    document.getElementById('navbar-main').addEventListener('click', function(event) {
        event.preventDefault();
        const target = event.target.nodeName === 'A' ? event.target : event.target.parentNode;
        const href = getCustomDomProperty(target, 'data-href');
        parseCmd(event, href);
    }, false);
    
    document.getElementById('volumeMenu').parentNode.addEventListener('show.bs.dropdown', function () {
        sendAPI("MYMPD_API_PLAYER_OUTPUT_LIST", {}, parseOutputs);
    });

    document.getElementById('outputs').addEventListener('click', function(event) {
        if (event.target.nodeName === 'A') {
            event.preventDefault();
            showListOutputAttributes(getCustomDomProperty(event.target.parentNode, 'data-output-name'));
        }
        else {
            const target = event.target.nodeName === 'BUTTON' ? event.target : event.target.parentNode;
            event.stopPropagation();
            event.preventDefault();
            sendAPI("MYMPD_API_PLAYER_OUTPUT_TOGGLE", {"outputId": getCustomDomProperty(target, 'data-output-id'), "state": (target.classList.contains('active') ? 0 : 1)});
            toggleBtn(target.id);
        }
    }, false);

    document.getElementById('scripts').addEventListener('click', function(event) {
        if (event.target.nodeName === 'A') {
            execScript(getCustomDomProperty(event.target, 'data-href'));
        }
    }, false);
}

//Handle javascript errors
window.onerror = function(msg, url, line) {
    logError('JavaScript error: ' + msg + ' (' + url + ': ' + line + ')');
    if (settings.loglevel >= 4) {
        if (appInited === true) {
            showNotification(t('JavaScript error'), msg + ' (' + url + ': ' + line + ')', 'general', 'error');
        }
        else {
            showAppInitAlert(t('JavaScript error') + ': ' + msg + ' (' + url + ': ' + line + ')');
        }
    }
    return true;
};

//Start app
appInitStart();
