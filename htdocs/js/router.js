"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module router_js */

/**
 * Shows the current view and highlights the navbar icon
 * @returns {void}
 */
function appPrepare() {
    if (app.current.card !== app.last.card ||
        app.current.tab !== app.last.tab ||
        app.current.view !== app.last.view)
    {
        //Hide all cards + nav
        for (let i = 0; i < domCache.navbarBtnsLen; i++) {
            domCache.navbarBtns[i].classList.remove('active');
        }
        for (const card of allCards) {
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
        //highlight active navbar icon
        let nav = elGetById('nav' + app.current.card + app.current.tab);
        if (nav) {
            nav.classList.add('active');
        }
        else {
            nav = elGetById('nav' + app.current.card);
            if (nav) {
                elGetById('nav' + app.current.card).classList.add('active');
            }
        }
    }
    const list = elGetById(app.id + 'List');
    if (list) {
        setUpdateView(list);
    }
}

/**
 * Calculates the location hash and calls appRoute
 * @param {string} card card element name
 * @param {string} [tab] tab element name
 * @param {string} [view] view element name
 * @param {number} [offset] list offset
 * @param {number} [limit] list limit
 * @param {string | object} [filter] filter
 * @param {object} [sort] sort object
 * @param {string} [tag] tag name
 * @param {string | object} [search] search object or string
 * @param {number} [newScrollPos] new scrolling position
 * @param {boolean} [append] Append the result to current result
 * @returns {void}
 */
function appGoto(card, tab, view, offset, limit, filter, sort, tag, search, newScrollPos, append) {
    //old app
    const oldptr = app.cards[app.current.card].offset !== undefined
        ? app.cards[app.current.card]
        : app.cards[app.current.card].tabs[app.current.tab].offset !== undefined
            ? app.cards[app.current.card].tabs[app.current.tab]
            : app.cards[app.current.card].tabs[app.current.tab].views[app.current.view];

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

    //overwrite view for jukebox queue view
    if (card === 'Queue' &&
        tab === 'Jukebox')
    {
        view = settings.partition.jukeboxMode === 'album'
            ? 'Album'
            : 'Song';
    }

    //get ptr to new app
    const ptr = app.cards[card].offset !== undefined
        ? app.cards[card]
        : app.cards[card].tabs[tab].offset !== undefined
            ? app.cards[card].tabs[tab]
            : app.cards[card].tabs[tab].views[view];

    //save scrollPos of old app
    if (oldptr !== ptr) {
        oldptr.scrollPos = getScrollPosY();
    }

    //set options to default, if not defined
    if (isDefined(offset) === false) { offset = ptr.offset; }
    if (isDefined(limit) === false)  { limit = ptr.limit; }
    if (isDefined(filter) === false) { filter = ptr.filter; }
    if (isDefined(sort) === false)   { sort = ptr.sort; }
    if (isDefined(tag) === false)    { tag = ptr.tag; }
    if (isDefined(search) === false) { search = ptr.search; }
    //enforce number type
    offset = Number(offset);
    limit = Number(limit);
    //set new scrollpos
    if (newScrollPos !== undefined) {
        ptr.scrollPos = newScrollPos;
    }
    //build hash
    app.goto = true;
    const newHash = myEncodeURIComponent(
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
    if (location.hash !== '#' + newHash) {
        location.hash = newHash;
    }
    appRoute(card, tab, view, offset, limit, filter, sort, tag, search, append);
}

/**
 * Checks if obj is string or object
 * @param {string | object} obj object to check
 * @returns {boolean} true if obj is object or string, else false
 */
function isArrayOrString(obj) {
    if (typeof obj === 'string') {
        return true;
    }
    return Array.isArray(obj);
}

/**
 * Returns the default startup view
 * @returns {string} Default startup view
 */
function defaultStartupView() {
    return features.featHome === true
        ? 'Home'
        : 'Playback';
}

/**
 * Checks and sets the startup view of myMPD
 * @returns {Array} Startup path (card/tab/view)
 */
function startupView() {
    if (settings.webuiSettings.startupView === undefined) {
        // Set default startup view
        settings.webuiSettings.startupView = defaultStartupView();
    }
    else if (settings.webuiSettings.startupView === 'Home' &&
        features.featHome === false)
    {
        // Home feature is disabled
        settings.webuiSettings.startupView = 'Playback';
    }
    else {
        // Check for valid startup view
        const path = settings.webuiSettings.startupView.split('/');
        try {
            if ((path.length === 1 && app.cards[path[0]] === undefined) ||
                (path.length === 2 && app.cards[path[0]].tabs[path[1]] === undefined) ||
                (path.length === 3 && app.cards[path[0]].tabs[path[1]].views[path[2]] === undefined) ||
                path.length === 0 ||
                path.length > 3)
            {
                settings.webuiSettings.startupView = defaultStartupView();
            }
        }
        catch(error) {
            logError("Invalid startupview");
            logError(error);
            settings.webuiSettings.startupView = defaultStartupView();
        }
    }

    return settings.webuiSettings.startupView.split('/');
}

/**
 * Executes the actions after the view is shown
 * @param {string} [card] card element name
 * @param {string} [tab] tab element name
 * @param {string} [view] view element name
 * @param {number} [offset] list offset
 * @param {number} [limit] list limit
 * @param {string | object} [filter] filter
 * @param {object} [sort] sort object
 * @param {string} [tag] tag name
 * @param {string | object} [search] search object or string
 * @param {boolean} [append] Append the result to current result
 * @returns {void}
 */
function appRoute(card, tab, view, offset, limit, filter, sort, tag, search, append) {
    if (settingsParsed === 'false') {
        appInitStart();
        return;
    }
    if (isDefined(card) === false) {
        const hash = location.hash.match(/^#(.*)$/);
        let jsonHash = null;
        if (hash !== null) {
            try {
                jsonHash = JSON.parse(decodeURIComponent(hash[1]));
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
                logDebug(error);
            }
        }
        if (jsonHash === null) {
            appPrepare();
            const path = startupView();
            // @ts-ignore
            appGoto(...path);
            return;
        }
    }
    else {
        app.current.card = card;
        app.current.tab = tab;
        app.current.view = view;
        if (append === true) {
            // In append mode (endless scrolling) the limit is the offset we get results for.
            // This will be switch back below, after fetching the results.
            app.current.offset = limit;
            app.current.limit = settings.webuiSettings.maxElementsPerPage;
        }
        else {
            app.current.offset = offset;
            app.current.limit = limit;
        }
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
    //ptr.offset = app.current.offset;
    //ptr.limit = app.current.limit;
    
    //get last scrolling position
    app.current.scrollPos = ptr.scrollPos;
    appPrepare();

    switch(app.id) {
        case 'Home':                      handleHome(); break;
        case 'Playback':                  handlePlayback(); break;
        case 'QueueCurrent':              handleQueueCurrent(); break;
        case 'QueueLastPlayed':           handleQueueLastPlayed(); break;
        case 'QueueJukeboxSong':          handleQueueJukeboxSong(); break;
        case 'QueueJukeboxAlbum':         handleQueueJukeboxAlbum(); break;
        case 'BrowsePlaylistList':        handleBrowsePlaylistList(); break;
        case 'BrowsePlaylistDetail':      handleBrowsePlaylistDetail(); break;
        case 'BrowseFilesystem':          handleBrowseFilesystem(); break;
        case 'BrowseDatabaseTagList':     handleBrowseDatabaseTagList(); break;
        case 'BrowseDatabaseAlbumList':   handleBrowseDatabaseAlbumList(); break;
        case 'BrowseDatabaseAlbumDetail': handleBrowseDatabaseAlbumDetail(); break;
        case 'BrowseRadioFavorites':      handleBrowseRadioFavorites(); break;
        case 'BrowseRadioWebradiodb':     handleBrowseRadioWebradiodb(); break;
        case 'Search':                    handleSearch(); break;
        default: {
            const path = startupView();
            // @ts-ignore
            appGoto(...path);
        }
    }

    //Save app options
    if (append === true) {
        app.current.offset = offset;
        app.current.limit = limit;
    }
    ptr.filter = app.current.filter;
    ptr.sort = app.current.sort;
    ptr.tag = app.current.tag;
    ptr.search = app.current.search;
    ptr.offset = app.current.offset;
    ptr.limit = app.current.limit;

    // Set last active view
    app.last.card = app.current.card;
    app.last.tab = app.current.tab;
    app.last.view = app.current.view;

    // Trigger background change
    if (settings.webuiSettings.dynamicBackground === 'trigger') {
        setBackgroundImage(domCache.body, currentSongObj.uri);
    }
}

/**
 * Emulates the browser back button
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function historyBack() {
    history.back();
}
