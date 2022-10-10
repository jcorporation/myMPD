"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/**
 * Shows the current view and highlights the navbar icon
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
        //highlight active navbar icon
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
        setUpdateView(list);
    }
}

/**
 * Calculates the location hash and calls appRoute
 * @param {String} card 
 * @param {String} [tab]
 * @param {String} [view]
 * @param {Number} [offset]
 * @param {Number} [limit]
 * @param {String | Object} [filter]
 * @param {Object} [sort]
 * @param {String} [tag]
 * @param {String | Object} [search]
 * @param {Number} [newScrollPos]
 */
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
        oldptr.scrollPos = getScrollPosY();
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
    //enforce sort, migration from pre 9.4.0 releases
    if (typeof sort === 'string') {
        sort = {
            "tag": sort,
            "desc": false
        };
    }
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

/**
 * Checks if obj is string or object
 * @param {String | Object} obj 
 * @returns {Boolean} true if obj is object or string, else false
 */
function isArrayOrString(obj) {
    if (typeof obj === 'string') {
        return true;
    }
    return Array.isArray(obj);
}

/**
 * Executes the actions after the view is shown
 * @param {String} [card]
 * @param {String} [tab]
 * @param {String} [view]
 * @param {Number} [offset]
 * @param {Number} [limit]
 * @param {String | Object} [filter]
 * @param {Object} [sort]
 * @param {String} [tag]
 * @param {String} [search]
 */
function appRoute(card, tab, view, offset, limit, filter, sort, tag, search) {
    if (settingsParsed === 'false') {
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
            appPrepare();
            let initialStartupView = settings.webuiSettings.uiStartupView;
            if (initialStartupView === undefined ||
                initialStartupView === null)
            {
                initialStartupView = features.featHome === true ? 'Home' : 'Playback';
            }
            const path = initialStartupView.split('/');
            // @ts-ignore
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
    appPrepare();

    switch(app.id) {
        case 'Home': {
            handleHome();
            break;
        }
        case 'Playback': {
            handlePlayback();
            break;
        }
        case 'QueueCurrent': {
            handleQueueCurrent();
            break;
        }
        case 'QueueLastPlayed': {
            handleQueueLastPlayed();
            break;
        }
        case 'QueueJukebox': {
            handleQueueJukebox();
            break;
        }
        case 'BrowsePlaylistsList': {
            handleBrowsePlaylistsList();
            break;
        }
        case 'BrowsePlaylistsDetail': {
            handleBrowsePlaylistsDetail();
            break;
        }
        case 'BrowseFilesystem': {
            handleBrowseFilesystem();
            break;
        }
        case 'BrowseDatabaseList': {
            handleBrowseDatabaseList();
            break;
        }
        case 'BrowseDatabaseDetail': {
            handleBrowseDatabaseDetail();
            break;
        }
        case 'BrowseRadioFavorites': {
            handleBrowseRadioFavorites();
            break;
        }
        case 'BrowseRadioWebradiodb': {
            handleBrowseRadioWebradiodb();
            break;
        }
        case 'BrowseRadioRadiobrowser': {
            handleBrowseRadioRadiobrowser();
            break;
        }
        case 'Search': {
            handleSearch();
            break;
        }
        default: {
            let initialStartupView = settings.webuiSettings.uiStartupView;
            if (initialStartupView === undefined ||
                initialStartupView === null)
            {
                initialStartupView = features.featHome === true ? 'Home' : 'Playback';
            }
            const path = initialStartupView.split('/');
            // @ts-ignore
            appGoto(...path);
        }
    }

    app.last.card = app.current.card;
    app.last.tab = app.current.tab;
    app.last.view = app.current.view;
}
