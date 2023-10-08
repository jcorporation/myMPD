"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module viewSearch_js */

/**
 * Handler for song search
 * @returns {void}
 */
function handleSearch() {
    handleSearchExpression('Search');
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
            "cols": settings.colsSearchFetch
        }, parseSearch, true);
    }
    else {
        // clear list if no search is defined
        const SearchListEl = elGetById('SearchList');
        elClear(SearchListEl.querySelector('tbody'));
        elClear(SearchListEl.querySelector('tfoot'));
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
    elGetById('SearchList').addEventListener('click', function(event) {
        const target = tableClickHandler(event);
        if (target !== null) {
            clickSong(getData(target, 'uri'), event);
        }
    }, false);

    initSearchExpression('Search');
}

/**
 * Parses the MYMPD_API_DATABASE_SEARCH jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parseSearch(obj) {
    const table = elGetById('SearchList');
    const tfoot = table.querySelector('tfoot');
    elClear(tfoot);

    if (checkResultId(obj, 'SearchList') === false) {
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

    const rowTitle = settingsWebuiFields.clickSong.validValues[settings.webuiSettings.clickSong];

    updateTable(obj, 'Search', function(row, data) {
        setData(row, 'type', data.Type);
        setData(row, 'uri', data.uri);
        setData(row, 'name', data.Title);
        row.setAttribute('tabindex', 0);
        row.setAttribute('title', rowTitle);
    });

    if (obj.result.totalEntities > 0) {
        const colspan = settings.colsSearch.length + 1;
        tfoot.appendChild(
            elCreateNode('tr', {"class": ["not-clickable"]},
                elCreateTextTnNr('td', {"colspan": colspan}, 'Num songs', obj.result.totalEntities)
            )
        );
    }
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
        "sort": "",
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
            appendQueue('search', [app.current.search]);
            break;
        case 'appendPlay':
            appendPlayQueue('search', [app.current.search]);
            break;
        case 'insertAfterCurrent':
            insertAfterCurrentQueue('search', [app.current.search]);
            break;
        case 'insertPlayAfterCurrent':
            insertPlayAfterCurrentQueue('search', [app.current.search]);
            break;
        case 'replace':
            replaceQueue('search', [app.current.search]);
            break;
        case 'replacePlay':
            replacePlayQueue('search', [app.current.search]);
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
    showAddToPlaylist('search', [app.current.search]);
}
