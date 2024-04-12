"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module viewSearch_js */

/**
 * Handler for song search
 * @returns {void}
 */
function handleSearch() {
    handleSearchExpression('Search');
    toggleBtnChkId('SearchSortDesc', app.current.sort.desc);
    selectTag('SearchSortTagsList', undefined, app.current.sort.tag);
    const searchStrEl = elGetById(app.id + 'SearchStr');
    const searchCrumbEl = elGetById(app.id + 'SearchCrumb');
    if (searchStrEl.value.length >= 2 ||
        searchCrumbEl.children.length > 0)
    {
        if (app.current.sort.tag === '') {
            app.current.sort.tag = settings.tagList.includes('Title')
                ? 'Title'
                : '';
        }
        sendAPI("MYMPD_API_DATABASE_SEARCH", {
            "offset": app.current.offset,
            "limit": app.current.limit,
            "sort": app.current.sort.tag,
            "sortdesc": app.current.sort.desc,
            "expression": app.current.search,
            "fields": settings.viewSearchFetch.fields
        }, parseSearch, true);
    }
    else {
        // clear list if no search is defined
        const SearchListEl = elGetById('SearchList');
        if (settings['view' + app.id].mode === 'table') {
            elClear(SearchListEl.querySelector('tbody'));
            elClear(SearchListEl.querySelector('tfoot'));
        }
        else {
            elClear(SearchListEl);
        }
        elDisableId('SearchAddAllSongsBtn');
        elDisableId('SearchAddAllSongsDropdownBtn');
        unsetUpdateViewId('SearchList');
        setPagination(0, 0);
    }
}

/**
 * Initialization function for the search elements
 * @returns {void}
 */
function initViewSearch() {
    initSortBtns('Search');
    initSearchExpression('Search');
}

/**
 * Click event handler for last played
 * @param {MouseEvent} event click event
 * @param {HTMLElement} target calculated target
 * @returns {void}
 */
function viewSearchListClickHandler(event, target) {
    clickSong(getData(target, 'uri'), event);
}

/**
 * Parses the MYMPD_API_DATABASE_SEARCH jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parseSearch(obj) {
    const table = elGetById('SearchList');
    if (checkResult(obj, table, undefined) === false) {
        return;
    }

    if (obj.result.returnedEntities > 0) {
        elEnableId('SearchAddAllSongsBtn');
        elEnableId('SearchAddAllSongsDropdownBtn');
    }
    else {
        elDisableId('SearchAddAllSongsBtn');
        elDisableId('SearchAddAllSongsDropdownBtn');
    }

    if (settings['view' + app.id].mode === 'table') {
        const tfoot = table.querySelector('tfoot');
        elClear(tfoot);
        const rowTitle = settingsWebuiFields.clickSong.validValues[settings.webuiSettings.clickSong];
        updateTable(obj, app.id, function(row, data) {
            setData(row, 'type', data.Type);
            setData(row, 'uri', data.uri);
            setData(row, 'name', data.Title);
            row.setAttribute('title', rowTitle);
        });

        if (obj.result.totalEntities > 0) {
            addTblFooter(tfoot,
                elCreateTextTnNr('span', {}, 'Num songs', obj.result.totalEntities)
            );
        }
        return;
    }
    updateGrid(obj, app.id, function(card, data) {
        setData(card, 'type', data.Type);
        setData(card, 'uri', data.uri);
        setData(card, 'name', data.Title);
    });
}

/**
 * Saves the current search as a smart playlist
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function saveSearchAsSmartPlaylist() {
    parseSmartPlaylist({"jsonrpc":"2.0","id":0,"result":{"method":"MYMPD_API_SMARTPLS_GET",
        "plist": "",
        "type": "search",
        "sort": app.current.sort.tag,
        "sortdesc": app.current.sort.desc,
        "maxentries": 0,
        "expression": app.current.search
    }});
}

/**
 * Appends the current search to the queue
 * @param {string} mode one of: append, appendPlay, insertAfterCurrent, insertPlayAfterCurrent, replace, replacePlay
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addAllFromSearch(mode) {
    switch(mode) {
        case 'append':
            appendQueue('search', [app.current.search, app.current.sort.tag, app.current.sort.desc]);
            break;
        case 'appendPlay':
            appendPlayQueue('search', [app.current.search, app.current.sort.tag, app.current.sort.desc]);
            break;
        case 'insertAfterCurrent':
            insertAfterCurrentQueue('search', [app.current.search, app.current.sort.tag, app.current.sort.desc]);
            break;
        case 'insertPlayAfterCurrent':
            insertPlayAfterCurrentQueue('search', [app.current.search, app.current.sort.tag, app.current.sort.desc]);
            break;
        case 'replace':
            replaceQueue('search', [app.current.search, app.current.sort.tag, app.current.sort.desc]);
            break;
        case 'replacePlay':
            replacePlayQueue('search', [app.current.search, app.current.sort.tag, app.current.sort.desc]);
            break;
        default:
            logError('Invalid mode: ' + mode);
    }
}

/**
 * Adds the current search to a playlist
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showAddToPlaylistCurrentSearch() {
    showAddToPlaylist('search', [app.current.search, app.current.sort.tag, app.current.sort.desc]);
}
