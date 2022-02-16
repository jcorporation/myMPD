"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/* eslint-enable no-unused-vars */
function appPrepare(scrollPos) {
    if (app.current.card !== app.last.card ||
        app.current.tab !== app.last.tab ||
        app.current.view !== app.last.view)
    {
        //Hide all cards + nav
        for (let i = 0; i < domCache.navbarBtnsLen; i++) {
            domCache.navbarBtns[i].classList.remove('active');
        }
        const cards = ['cardHome', 'cardPlayback', 'cardSearch',
            'cardQueue', 'tabQueueCurrent', 'tabQueueLastPlayed', 'tabQueueJukebox',
            'cardBrowse', 'tabBrowseFilesystem',
            'tabBrowseRadio', 'viewBrowseRadioFavorites', 'viewBrowseRadioWebradiodb', 'viewBrowseRadioRadiobrowser',
            'tabBrowsePlaylists', 'viewBrowsePlaylistsDetail', 'viewBrowsePlaylistsList',
            'tabBrowseDatabase', 'viewBrowseDatabaseDetail', 'viewBrowseDatabaseList'];
        for (const card of cards) {
            elHideId(card);
        }
        //show active card
        elShowId('card' + app.current.card);
        //show active tab
        if (app.current.tab !== undefined &&
            app.current.tab !== '')
        {
            elShowId('tab' + app.current.card + app.current.tab);
        }
        //show active view
        if (app.current.view !== undefined &&
            app.current.view !== '')
        {
            elShowId('view' + app.current.card + app.current.tab + app.current.view);
        }
        //show active navbar icon
        let nav = document.getElementById('nav' + app.current.card + app.current.tab);
        if (nav) {
            nav.classList.add('active');
        }
        else {
            nav = document.getElementById('nav' + app.current.card);
            if (nav) {
                document.getElementById('nav' + app.current.card).classList.add('active');
            }
        }
    }
    const list = document.getElementById(app.id + 'List');
    if (list) {
        scrollToPosY(list.parentNode, scrollPos);
        list.classList.add('opacity05');
    }
}

function appGoto(card, tab, view, offset, limit, filter, sort, tag, search, newScrollPos) {
    //old app
    const oldptr = app.cards[app.current.card].offset !== undefined ? app.cards[app.current.card] :
        app.cards[app.current.card].tabs[app.current.tab].offset !== undefined ? app.cards[app.current.card].tabs[app.current.tab] :
            app.cards[app.current.card].tabs[app.current.tab].views[app.current.view];

    //get default active tab or view from state
    if (app.cards[card].tabs) {
        if (tab === undefined) {
            tab = app.cards[card].active;
        }
        if (app.cards[card].tabs[tab].views) {
            if (view === undefined) {
                view = app.cards[card].tabs[tab].active;
            }
        }
    }

    //get ptr to new app
    const ptr = app.cards[card].offset !== undefined ? app.cards[card] :
                app.cards[card].tabs[tab].offset !== undefined ? app.cards[card].tabs[tab] :
                app.cards[card].tabs[tab].views[view];

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
    //enforce number type
    offset = Number(offset);
    limit = Number(limit);
    //set new scrollpos
    if (newScrollPos !== undefined) {
        ptr.scrollPos = newScrollPos;
    }
    //build hash
    app.goto = true;
    location.hash = myEncodeURIComponent(
        JSON.stringify({
            "card": card,
            "tab": tab,
            "view": view,
            "offset": offset,
            "limit": limit,
            "filter": filter,
            "sort": sort,
            "tag": tag,
            "search": search
        })
    );
    appRoute(card, tab, view, offset, limit, filter, sort, tag, search);
}

function isArrayOrString(obj) {
    if (typeof obj === 'string') {
        return true;
    }
    return Array.isArray(obj);
}

function appRoute(card, tab, view, offset, limit, filter, sort, tag, search) {
    if (settingsParsed === false) {
        appInitStart();
        return;
    }
    if (card === undefined || card === null) {
        const hash = location.hash.match(/^#(.*)$/);
        let jsonHash = null;
        if (hash !== null) {
            try {
                jsonHash = JSON.parse(myDecodeURIComponent(hash[1]));
                app.current.card = isArrayOrString(jsonHash.card) ? jsonHash.card : undefined;
                app.current.tab = typeof jsonHash.tab === 'string' ? jsonHash.tab : undefined;
                app.current.view = typeof jsonHash.view === 'string' ? jsonHash.view : undefined;
                app.current.offset = typeof jsonHash.offset === 'number' ? jsonHash.offset : '';
                app.current.limit = typeof jsonHash.limit === 'number' ? jsonHash.limit : '';
                app.current.filter = jsonHash.filter;
                app.current.sort = jsonHash.sort;
                app.current.tag = isArrayOrString(jsonHash.tag) ? jsonHash.tag : '';
                app.current.search = isArrayOrString(jsonHash.search) ? jsonHash.search : '';
            }
            catch(error) {
                //do nothing
            }
        }
        if (jsonHash === null) {
            appPrepare(0);
            let initialStartupView = settings.webuiSettings.uiStartupView;
            if (initialStartupView === undefined ||
                initialStartupView === null)
            {
                initialStartupView = features.featHome === true ? 'Home' : 'Playback';
            }
            const path = initialStartupView.split('/');
            appGoto(...path);
            return;
        }
    }
    else {
        app.current.card = card;
        app.current.tab = tab;
        app.current.view = view;
        app.current.offset = offset;
        app.current.limit = limit;
        app.current.filter = filter;
        app.current.sort = sort;
        app.current.tag = tag;
        app.current.search = search;
    }
    app.id = app.current.card +
        (app.current.tab === undefined ? '' : app.current.tab) +
        (app.current.view === undefined ? '' : app.current.view);

    //get ptr to app options and set active tab/view
    let ptr;
    if (app.cards[app.current.card].offset !== undefined) {
        ptr = app.cards[app.current.card];
    }
    else if (app.cards[app.current.card].tabs[app.current.tab].offset !== undefined) {
        ptr = app.cards[app.current.card].tabs[app.current.tab];
        app.cards[app.current.card].active = app.current.tab;
    }
    else if (app.cards[app.current.card].tabs[app.current.tab].views[app.current.view].offset !== undefined) {
        ptr = app.cards[app.current.card].tabs[app.current.tab].views[app.current.view];
        app.cards[app.current.card].active = app.current.tab;
        app.cards[app.current.card].tabs[app.current.tab].active = app.current.view;
    }
    //set app options
    ptr.offset = app.current.offset;
    ptr.limit = app.current.limit;
    ptr.filter = app.current.filter;
    ptr.sort = app.current.sort;
    ptr.tag = app.current.tag;
    ptr.search = app.current.search;
    app.current.scrollPos = ptr.scrollPos;

    appPrepare(app.current.scrollPos);

    switch(app.id) {
        case 'Home': {
            const list = document.getElementById('HomeList');
            list.classList.remove('opacity05');
            setScrollViewHeight(list);
            sendAPI("MYMPD_API_HOME_LIST", {}, parseHome);
            break;
        }
        case 'Playback': {
            const list = document.getElementById('PlaybackList');
            list.classList.remove('opacity05');
            setScrollViewHeight(list);
            sendAPI("MYMPD_API_PLAYER_CURRENT_SONG", {}, songChange);
            break;
        }
        case 'QueueCurrent': {
            document.getElementById('searchQueueStr').focus();
            if (features.featAdvqueue) {
                createSearchCrumbs(app.current.search, document.getElementById('searchQueueStr'), document.getElementById('searchQueueCrumb'));
            }
            else if (document.getElementById('searchQueueStr').value === '' &&
                app.current.search !== '')
            {
                document.getElementById('searchQueueStr').value = app.current.search;
            }
            if (app.current.search === '') {
                document.getElementById('searchQueueStr').value = '';
            }
            if (features.featAdvqueue === true) {
                if (app.current.sort.tag === '-') {
                    app.current.sort.tag = 'Priority';
                }
                sendAPI("MYMPD_API_QUEUE_SEARCH_ADV", {
                    "offset": app.current.offset,
                    "limit": app.current.limit,
                    "sort": app.current.sort.tag,
                    "sortdesc": app.current.sort.desc,
                    "expression": app.current.search,
                    "cols": settings.colsSearchFetch
                }, parseQueue, true);
                if (app.current.filter === 'prio') {
                    elShowId('priorityMatch');
                    document.getElementById('searchQueueMatch').value = '>=';
                }
                else {
                    if (getSelectValueId('searchQueueMatch') === '>=') {
                        document.getElementById('searchQueueMatch').value = 'contains';
                    }
                    elHideId('priorityMatch');
                }
            }
            else if (document.getElementById('searchQueueStr').value.length >= 2 ||
                     document.getElementById('searchQueueCrumb').children.length > 0)
            {
                sendAPI("MYMPD_API_QUEUE_SEARCH", {
                    "offset": app.current.offset,
                    "limit": app.current.limit,
                    "filter": app.current.filter,
                    "searchstr": app.current.search,
                    "cols": settings.colsSearchFetch
                }, parseQueue, true);
            }
            else {
                sendAPI("MYMPD_API_QUEUE_LIST", {
                    "offset": app.current.offset,
                    "limit": app.current.limit,
                    "cols": settings.colsSearchFetch
                }, parseQueue, true);
            }
            selectTag('searchQueueTags', 'searchQueueTagsDesc', app.current.filter);
            break;
        }
        case 'QueueLastPlayed': {
            sendAPI("MYMPD_API_QUEUE_LAST_PLAYED", {
                "offset": app.current.offset,
                "limit": app.current.limit,
                "cols": settings.colsQueueLastPlayedFetch,
                "searchstr": app.current.search
            }, parseLastPlayed, true);
            const searchQueueLastPlayedStrEl = document.getElementById('searchQueueLastPlayedStr');
            if (searchQueueLastPlayedStrEl.value === '' &&
                app.current.search !== '')
            {
                searchQueueLastPlayedStrEl.value = app.current.search;
            }
            break;
        }
        case 'QueueJukebox': {
            sendAPI("MYMPD_API_JUKEBOX_LIST", {
                "offset": app.current.offset,
                "limit": app.current.limit,
                "cols": settings.colsQueueJukeboxFetch,
                "searchstr": app.current.search
            }, parseJukeboxList, true);
            const searchQueueJukeboxStrEl = document.getElementById('searchQueueJukeboxStr');
            if (searchQueueJukeboxStrEl.value === '' &&
                app.current.search !== '')
            {
                searchQueueJukeboxStrEl.value = app.current.search;
            }
            break;
        }
        case 'BrowsePlaylistsList': {
            sendAPI("MYMPD_API_PLAYLIST_LIST", {
                "offset": app.current.offset,
                "limit": app.current.limit,
                "searchstr": app.current.search,
                "type": 0
            }, parsePlaylistsList, true);
            const searchPlaylistsStrEl = document.getElementById('searchPlaylistsListStr');
            if (searchPlaylistsStrEl.value === '' &&
                app.current.search !== '')
            {
                searchPlaylistsStrEl.value = app.current.search;
            }
            break;
        }
        case 'BrowsePlaylistsDetail': {
            sendAPI("MYMPD_API_PLAYLIST_CONTENT_LIST", {
                "offset": app.current.offset,
                "limit": app.current.limit,
                "searchstr": app.current.search,
                "plist": app.current.filter,
                "cols": settings.colsBrowsePlaylistsDetailFetch
            }, parsePlaylistsDetail, true);
            const searchPlaylistsStrEl = document.getElementById('searchPlaylistsDetailStr');
            if (searchPlaylistsStrEl.value === '' &&
                app.current.search !== '')
            {
                searchPlaylistsStrEl.value = app.current.search;
            }
            break;
        }
        case 'BrowseFilesystem': {
            if (app.current.tag === '-') {
                //default type is dir
                app.current.tag = 'dir';
            }
            sendAPI("MYMPD_API_DATABASE_FILESYSTEM_LIST", {
                "offset": app.current.offset,
                "limit": app.current.limit,
                "path": (app.current.search ? app.current.search : "/"),
                "searchstr": (app.current.filter !== '-' ? app.current.filter : ''),
                "type": app.current.tag,
                "cols": settings.colsBrowseFilesystemFetch
            }, parseFilesystem, true);
            //Don't add all songs from root
            if (app.current.search === '') {
                elHideId('BrowseFilesystemAddAllSongsGrp');
                if (features.featHome === true) {
                    elShowId('BrowseFilesystemAddToHome');
                }
            }
            else {
                elShowId('BrowseFilesystemAddAllSongsGrp');
                elHideId('BrowseFilesystemAddToHome');
            }
            //Create breadcrumb
            const crumbEl = document.getElementById('BrowseBreadcrumb');
            elClear(crumbEl);
            const home = elCreateText('a', {"class": ["mi"]}, 'home');
            setData(home, 'uri', '');
            crumbEl.appendChild(elCreateNode('li', {"class": ["breadcrumb-item"]}, home));

            const pathArray = app.current.search.split('/');
            const pathArrayLen = pathArray.length;
            let fullPath = '';
            for (let i = 0; i < pathArrayLen; i++) {
                if (pathArrayLen - 1 === i) {
                    crumbEl.appendChild(elCreateText('li', {"class": ["breadcrumb-item", "active"]}, pathArray[i]));
                    break;
                }
                fullPath += pathArray[i];
                const a = elCreateText('a', {"href": "#"}, pathArray[i]);
                setData(a, 'uri', fullPath);
                crumbEl.appendChild(elCreateNode('li', {"class": ["breadcrumb-item"]}, a));
                fullPath += '/';
            }
            const searchFilesystemStrEl = document.getElementById('searchFilesystemStr');
            searchFilesystemStrEl.value = app.current.filter === '-' ? '' :  app.current.filter;
            break;
        }
        case 'BrowseDatabaseList': {
            selectTag('searchDatabaseTags', 'searchDatabaseTagsDesc', app.current.filter);
            selectTag('BrowseDatabaseByTagDropdown', 'btnBrowseDatabaseByTagDesc', app.current.tag);
            let tsort = app.current.sort;
            let sortdesc = false;
            if (tsort.charAt(0) === '-') {
                sortdesc = true;
                tsort = tsort.substr(1);
                toggleBtnChkId('databaseSortDesc', true);
            }
            else {
                toggleBtnChkId('databaseSortDesc', false);
            }
            selectTag('databaseSortTags', undefined, tsort);
            if (app.current.tag === 'Album') {
                createSearchCrumbs(app.current.search, document.getElementById('searchDatabaseStr'), document.getElementById('searchDatabaseCrumb'));
                if (app.current.search === '') {
                    document.getElementById('searchDatabaseStr').value = '';
                }
                elShowId('searchDatabaseMatch');
                elEnableId('btnDatabaseSortDropdown');
                elEnableId('btnDatabaseSearchDropdown');
                sendAPI("MYMPD_API_DATABASE_ALBUMS_GET", {
                    "offset": app.current.offset,
                    "limit": app.current.limit,
                    "expression": app.current.search,
                    "sort": tsort,
                    "sortdesc": sortdesc
                }, parseDatabase, true);
            }
            else {
                elHideId('searchDatabaseCrumb');
                elHideId('searchDatabaseMatch');
                elDisableId('btnDatabaseSortDropdown');
                elDisableId('btnDatabaseSearchDropdown');
                document.getElementById('searchDatabaseStr').value = app.current.search;
                sendAPI("MYMPD_API_DATABASE_TAG_LIST", {
                    "offset": app.current.offset,
                    "limit": app.current.limit,
                    "searchstr": app.current.search,
                    "tag": app.current.tag
                }, parseDatabase, true);
            }
            break;
        }
        case 'BrowseDatabaseDetail': {
            if (app.current.filter === 'Album') {
                sendAPI("MYMPD_API_DATABASE_TAG_ALBUM_TITLE_LIST", {
                    "album": app.current.tag,
                    "albumartist": app.current.search,
                    "cols": settings.colsBrowseDatabaseDetailFetch
                }, parseAlbumDetails, true);
            }
            //more detail views coming
            break;
        }
        case 'BrowseRadioFavorites': {
            sendAPI("MYMPD_API_WEBRADIO_FAVORITE_LIST", {
                "offset": app.current.offset,
                "limit": app.current.limit,
                "searchstr": app.current.search
            }, parseRadioFavoritesList, true);
            break;
        }
        case 'BrowseRadioWebradiodb': {
            if (webradioDb === null) {
                //fetch webradiodb database
                getWebradiodb();
                break;
            }
            setDataId('filterWebradiodbGenre', 'value', app.current.filter.genre);
            document.getElementById('filterWebradiodbGenre').value = app.current.filter.genre;
            setDataId('filterWebradiodbCountry', 'value', app.current.filter.country);
            document.getElementById('filterWebradiodbCountry').value = app.current.filter.country;
            setDataId('filterWebradiodbLanguage', 'value', app.current.filter.language);
            document.getElementById('filterWebradiodbLanguage').value = app.current.filter.language;

            const result = searchWebradiodb(app.current.search, app.current.filter.genre,
                app.current.filter.country, app.current.filter.language, app.current.sort,
                app.current.offset, app.current.limit);
            parseSearchWebradiodb(result);
            break;
        }
        case 'BrowseRadioRadiobrowser': {
            document.getElementById('inputRadiobrowserTags').value = app.current.filter.tags;
            document.getElementById('inputRadiobrowserCountry').value = app.current.filter.country;
            document.getElementById('inputRadiobrowserLanguage').value = app.current.filter.language;
            if (app.current.search === '') {
                sendAPI("MYMPD_API_CLOUD_RADIOBROWSER_NEWEST", {
                    "offset": app.current.offset,
                    "limit": app.current.limit,
                }, parseRadiobrowserList, true);
            }
            else {
                sendAPI("MYMPD_API_CLOUD_RADIOBROWSER_SEARCH", {
                    "offset": app.current.offset,
                    "limit": app.current.limit,
                    "tags": app.current.filter.tags,
                    "country": app.current.filter.country,
                    "language": app.current.filter.language,
                    "searchstr": app.current.search
                }, parseRadiobrowserList, true);
            }
            break;
        }
        case 'Search': {
            document.getElementById('searchStr').focus();
            if (features.featAdvsearch) {
                createSearchCrumbs(app.current.search, document.getElementById('searchStr'), document.getElementById('searchCrumb'));
            }
            else if (document.getElementById('searchStr').value === '' &&
                app.current.search !== '')
            {
                document.getElementById('searchStr').value = app.current.search;
            }
            if (app.current.search === '') {
                document.getElementById('searchStr').value = '';
            }
            if (document.getElementById('searchStr').value.length >= 2 ||
                document.getElementById('searchCrumb').children.length > 0)
            {
                if (features.featAdvsearch === true) {
                    if (app.current.sort.tag === '-') {
                        app.current.sort.tag = settings.tagList.includes('Title') ? 'Title' : '-';
                    }
                    sendAPI("MYMPD_API_DATABASE_SEARCH_ADV", {
                        "offset": app.current.offset,
                        "limit": app.current.limit,
                        "sort": app.current.sort.tag,
                        "sortdesc": app.current.sort.desc,
                        "expression": app.current.search,
                        "cols": settings.colsSearchFetch
                    }, parseSearch, true);
                }
                else {
                    sendAPI("MYMPD_API_DATABASE_SEARCH", {
                        "offset": app.current.offset,
                        "limit": app.current.limit,
                        "filter": app.current.filter,
                        "searchstr": app.current.search,
                        "cols": settings.colsSearchFetch
                    }, parseSearch, true);
                }
            }
            else {
                elClear(document.getElementById('SearchList').getElementsByTagName('tbody')[0]);
                elClear(document.getElementById('SearchList').getElementsByTagName('tfoot')[0]);
                elDisableId('searchAddAllSongs');
                elDisableId('searchAddAllSongsBtn');
                document.getElementById('SearchList').classList.remove('opacity05');
                setPagination(0, 0);
            }
            selectTag('searchTags', 'searchTagsDesc', app.current.filter);
            break;
        }
        default:
            let initialStartupView = settings.webuiSettings.uiStartupView;
            if (initialStartupView === undefined ||
                initialStartupView === null)
            {
                initialStartupView = features.featHome === true ? 'Home' : 'Playback';
            }
            const path = initialStartupView.split('/');
            appGoto(...path);
    }

    app.last.card = app.current.card;
    app.last.tab = app.current.tab;
    app.last.view = app.current.view;
}

function showAppInitAlert(text) {
    const spa = document.getElementById('splashScreenAlert');
    elClear(spa);
    spa.appendChild(elCreateText('p', {"class": ["text-danger"]}, tn(text)));
    const btn = elCreateText('button', {"class": ["btn", "btn-danger"]}, tn('Reload'));
    btn.addEventListener('click', function() {
        clearAndReload();
    }, false);
    spa.appendChild(elCreateNode('p', {}, btn));
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
        elShowId('nav-add2homescreen');
    });

    document.getElementById('nav-add2homescreen').addEventListener('click', function(event) {
        // Hide our user interface that shows our A2HS button
        elHide(event.target);
        // Show the prompt
        deferredA2HSprompt.prompt();
        // Wait for the user to respond to the prompt
        deferredA2HSprompt.userChoice.then((choiceResult) => {
            logDebug(choiceResult.outcome === 'accepted' ? 'User accepted the A2HS prompt' : 'User dismissed the A2HS prompt');
            deferredA2HSprompt = null;
        });
    });

    window.addEventListener('appinstalled', function() {
        showNotification(tn('myMPD installed as app'), '', 'general', 'info');
    });
}

function appInitStart() {
    //add app routing event handler
    window.addEventListener('hashchange', function() {
        if (app.goto === false) {
            appRoute();
        }
        else {
            app.goto = false;
        }
    }, false);

    //webapp manifest shortcuts
    const params = new URLSearchParams(window.location.search);
    const action = params.get('action');
    if (action === 'clickPlay') {
        clickPlay();
    }
    else if (action === 'clickStop') {
        clickStop();
    }
    else if (action === 'clickNext') {
        clickNext();
    }

    //update table height on window resize
    window.addEventListener('resize', function() {
        const list = document.getElementById(app.id + 'List');
        if (list) {
            setScrollViewHeight(list);
        }
    }, false);

    //set initial scale
    if (isMobile === true) {
        scale = localStorage.getItem('scale-ratio');
        if (scale === null) {
            scale = '1.0';
        }
        setViewport(false);
        domCache.body.classList.add('mobile');
    }
    else {
        const ms = document.getElementsByClassName('featMobile');
        for (const m of ms) {
            elHide(m);
        }
        domCache.body.classList.add('not-mobile');
    }

    subdir = window.location.pathname.replace('/index.html', '').replace(/\/$/, '');
    i18nHtml(document.getElementById('splashScreenAlert'));

    //set loglevel
    if (debugMode === true) {
        settings.loglevel = 4;
    }
    //register serviceworker
    if ('serviceWorker' in navigator &&
        window.location.protocol === 'https:' &&
        debugMode === false)
    {
        window.addEventListener('load', function() {
            navigator.serviceWorker.register('sw.js', {scope: subdir + '/'}).then(function(registration) {
                //Registration was successful
                logDebug('ServiceWorker registration successful.');
                registration.update();
            }, function(err) {
                //Registration failed
                logError('ServiceWorker registration failed: ' + err);
            });
        });
    }

    //show splash screen
    elShowId('splashScreen');
    domCache.body.classList.add('overflow-hidden');
    document.getElementById('splashScreenAlert').textContent = tn('Fetch myMPD settings');

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
            document.getElementById('splashScreenAlert').textContent = tn('Applying settings');
            document.getElementById('splashScreen').classList.add('hide-fade');
            setTimeout(function() {
                elHideId('splashScreen');
                document.getElementById('splashScreen').classList.remove('hide-fade');
                domCache.body.classList.remove('overflow-hidden');
            }, 500);
            appInit();
            appInited = true;
            appRoute();
            logDebug('Startup duration: ' + (Date.now() - startTime) + 'ms');
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
            toggleCollapseArrow(this);
        }, false);
    }
    //align dropdowns
    const dropdowns = document.querySelectorAll('.dropdown-toggle');
    for (const dropdown of dropdowns) {
        dropdown.parentNode.addEventListener('show.bs.dropdown', function (event) {
            alignDropdown(event.target);
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
            parseCmd(event, getData(this, 'href'));
        }, false);
    }
    //hide popover
    domCache.body.addEventListener('click', function() {
        hidePopover();
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
    initSettings();
    initPlayback();
    initNavs();
    initPlaylists();
    initOutputs();
    initWebradio();
    //init drag and drop
    for (const table of ['QueueCurrentList', 'BrowsePlaylistsDetailList']) {
        dragAndDropTable(table);
    }
    const dndTableHeader = [
        'QueueCurrent',
        'QueueLastPlayed',
        'QueueJukebox',
        'Search',
        'BrowseFilesystem',
        'BrowsePlaylistsDetail',
        'BrowseDatabaseDetail',
        'BrowseRadioWebradiodb',
        'BrowseRadioRadiobrowser'
    ];
    for (const table of dndTableHeader) {
        dragAndDropTableHeader(table);
    }
    //init custom elements
    initElements(domCache.body);
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
            if (keymap[event.key].req === undefined || settings[keymap[event.key].req] === true) {
                parseCmd(event, cmd);
            }
            event.stopPropagation();
        }

    }, false);
    //contextmenu for tables
    const tables = ['BrowseFilesystemList', 'BrowseDatabaseDetailList', 'QueueCurrentList', 'QueueLastPlayedList',
        'QueueJukeboxList', 'SearchList', 'BrowsePlaylistsListList', 'BrowsePlaylistsDetailList',
        'BrowseRadioRadiobrowserList', 'BrowseRadioWebradiodbList'];
    for (const tableName of tables) {
        document.getElementById(tableName).getElementsByTagName('tbody')[0].addEventListener('long-press', function(event) {
            if (event.target.parentNode.classList.contains('not-clickable') ||
                event.target.parentNode.parentNode.classList.contains('not-clickable') ||
                getData(event.target.parentNode, 'type') === 'parentDir')
            {
                return;
            }
            showPopover(event);
        }, false);

        document.getElementById(tableName).getElementsByTagName('tbody')[0].addEventListener('contextmenu', function(event) {
            if (event.target.parentNode.classList.contains('not-clickable') ||
                event.target.parentNode.parentNode.classList.contains('not-clickable') ||
                getData(event.target.parentNode, 'type') === 'parentDir')
            {
                return;
            }
            showPopover(event);
        }, false);
    }

    //websocket
    window.addEventListener('beforeunload', function() {
        webSocketClose();
    });
}

function initGlobalModals() {
    const tab = document.getElementById('tabShortcuts');
    elClear(tab);
    const keys = Object.keys(keymap).sort((a, b) => {
        return keymap[a].order - keymap[b].order
    });
    for (const key of keys) {
        if (keymap[key].cmd === undefined) {
            tab.appendChild(
                elCreateNode('div', {"class": ["row", "mb-2", "mt-3"]},
                    elCreateNode('div', {"class": ["col-12"]},
                        elCreateText('h5', {}, tn(keymap[key].desc))
                    )
                )
            );
            tab.appendChild(elCreateEmpty('div', {"class": ["row"]}));
            continue;
        }
        const col = elCreateEmpty('div', {"class": ["col", "col-6", "mb-3", "align-items-center"]});
        const k = elCreateText('div', {"class": ["key", "float-start"]}, (keymap[key].key !== undefined ? keymap[key].key : key));
        if (keymap[key].key && keymap[key].key.length > 1) {
            k.classList.add('mi', 'mi-small');
        }
        col.appendChild(k);
        col.appendChild(elCreateText('div', {}, tn(keymap[key].desc)));
        tab.lastChild.appendChild(col);
    }

    document.getElementById('modalAbout').addEventListener('shown.bs.modal', function () {
        sendAPI("MYMPD_API_DATABASE_STATS", {}, parseStats);
        getServerinfo();
    }, false);

    document.getElementById('modalUpdateDB').addEventListener('hidden.bs.modal', function() {
        document.getElementById('updateDBprogress').classList.remove('updateDBprogressAnimate');
    }, false);

    document.getElementById('modalEnterPin').addEventListener('shown.bs.modal', function() {
        document.getElementById('inputPinModal').focus();
    }, false);

    document.getElementById('inputPinModal').addEventListener('keyup', function(event) {
        if (event.key === 'Enter') {
            document.getElementById('modalEnterPinEnterBtn').click();
        }
    }, false);
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
        if (event.target.nodeName === 'P' ||
            event.target.nodeName === 'SPAN')
        {
            gotoBrowse(event);
        }
    }, false);
}

function initNavs() {
    document.getElementById('volumeBar').addEventListener('change', function() {
        sendAPI("MYMPD_API_PLAYER_VOLUME_SET", {"volume": Number(document.getElementById('volumeBar').value)});
    }, false);

    domCache.progress.addEventListener('click', function(event) {
        if (currentState.currentSongId >= 0 &&
            currentState.totalTime > 0)
        {
            const seekVal = Math.ceil((currentState.totalTime * event.clientX) / domCache.progress.offsetWidth);
            sendAPI("MYMPD_API_PLAYER_SEEK_CURRENT", {
                "seek": seekVal,
                "relative": false
            });
        }
    }, false);

    domCache.progress.addEventListener('mousemove', function(event) {
        if ((currentState.state === 'pause' || currentState.state === 'play') &&
            currentState.totalTime > 0)
        {
            domCache.progressPos.textContent = beautifySongDuration(Math.ceil((currentState.totalTime / event.target.offsetWidth) * event.clientX));
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

    const navbarMain = document.getElementById('navbar-main');
    navbarMain.addEventListener('click', function(event) {
        event.preventDefault();
        if (event.target.nodeName === 'DIV') {
            return;
        }
        const target = event.target.nodeName === 'A' ? event.target : event.target.parentNode;
        const href = getData(target, 'href');
        parseCmd(event, href);
    }, false);

    navbarMain.addEventListener('contextmenu', function(event) {
        if (event.target.getAttribute('data-popover') === null &&
            event.target.parentNode.getAttribute('data-popover') === null)
        {
            return;
        }
        showPopover(event);
    }, false);
    navbarMain.addEventListener('long-press', function(event) {
        if (event.target.getAttribute('data-popover') === null &&
            event.target.parentNode.getAttribute('data-popover') === null)
        {
            return;
        }
        showPopover(event);
    }, false);

    document.getElementById('scripts').addEventListener('click', function(event) {
        event.preventDefault();
        if (event.target.nodeName === 'A') {
            execScript(getData(event.target, 'href'));
        }
    }, false);

    //update list of notifications
    document.getElementById('menu-notifications').addEventListener('show.bs.collapse', function() {
        showMessages();
    }, false);
    document.getElementById('offcanvasMenu').addEventListener('show.bs.offcanvas', function() {
        showMessages();
    }, false);
}

//Handle javascript errors
if (debugMode === false) {
    window.onerror = function(msg, url, line) {
        logError('JavaScript error: ' + msg + ' (' + url + ': ' + line + ')');
        if (settings.loglevel >= 4) {
            if (appInited === true) {
                showNotification(tn('JavaScript error'), msg + ' (' + url + ': ' + line + ')', 'general', 'error');
            }
            else {
                showAppInitAlert(tn('JavaScript error') + ': ' + msg + ' (' + url + ': ' + line + ')');
            }
        }
        return true;
    };
}

//allow service worker registration
if (window.trustedTypes && window.trustedTypes.createPolicy) {
    window.trustedTypes.createPolicy('default', {
        createScriptURL(dirty) {
            if (dirty === 'sw.js') {
                return 'sw.js'
            }
            throw new Error('Script not allowed: ' + dirty);
       }
    });
}

//Start app
appInitStart();
