"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module Search_js */

/**
 * Handler for song search
 * @returns {void}
 */
function handleSearch() {
    handleSearchExpression('Search');
    const searchStrEl = document.getElementById(app.id + 'SearchStr');
    const searchCrumbEl = document.getElementById(app.id + 'SearchCrumb');
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
        const SearchListEl = document.getElementById('SearchList');
        elClear(SearchListEl.querySelector('tbody'));
        elClear(SearchListEl.querySelector('tfoot'));
        elDisableId('searchAddAllSongs');
        elDisableId('searchAddAllSongsBtn');
        unsetUpdateViewId('SearchList');
        setPagination(0, 0);
    }
}

/**
 * Initialization function for the search elements
 * @returns {void}
 */
function initSearch() {
    document.getElementById('SearchList').addEventListener('click', function(event) {
        //select mode
        if (selectRow(event) === true) {
            return;
        }
        //action td
        if (event.target.nodeName === 'A') {
            handleActionTdClick(event);
            return;
        }
        //table header
        if (event.target.nodeName === 'TH') {
            const colName = event.target.getAttribute('data-col');
            if (isColSortable('Search', colName) === false) {
                //by this fields can not be sorted
                return;
            }
            toggleSort(event.target, colName);
            appGoto(app.current.card, app.current.tab, app.current.view,
                app.current.offset, app.current.limit, app.current.filter, app.current.sort, '', app.current.search);
            return;
        }
        //table body
        const target = event.target.closest('TR');
        if (target === null) {
            return;
        }
        if (target.parentNode.nodeName === 'TBODY' &&
            checkTargetClick(target) === true)
        {
            clickSong(getData(target, 'uri'), event);
        }
    }, false);

    initSearchExpression('Search', doSearch);
}

/**
 * Searches for songs
 * @param {string} value current search input value
 * @returns {void}
 */
function doSearch(value) {
    const expression = createSearchExpression(document.getElementById(app.id + 'SearchCrumb'), app.current.filter, getSelectValueId(app.id + 'SearchMatch'), value);
    appGoto('Search', undefined, undefined, 0, app.current.limit, app.current.filter, app.current.sort, '', expression, 0);
}

/**
 * Parses the MYMPD_API_DATABASE_SEARCH jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parseSearch(obj) {
    const table = document.getElementById('SearchList');
    const tfoot = table.querySelector('tfoot');
    elClear(tfoot);

    if (checkResultId(obj, 'SearchList') === false) {
        return;
    }

    if (obj.result.returnedEntities > 0) {
        elEnableId('searchAddAllSongs');
        elEnableId('searchAddAllSongsBtn');
    }
    else {
        elDisableId('searchAddAllSongs');
        elDisableId('searchAddAllSongsBtn');
    }

    const rowTitle = webuiSettingsDefault.clickSong.validValues[settings.webuiSettings.clickSong];

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
 * @param {string} type one of: search, dir
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addAllFromSearch(mode, type) {
    switch(mode) {
        case 'append':
            appendQueue(type, [app.current.search]);
            break;
        case 'appendPlay':
            appendPlayQueue(type, [app.current.search]);
            break;
        case 'insertAfterCurrent':
            insertAfterCurrentQueue(type, [app.current.search]);
            break;
        case 'insertPlayAfterCurrent':
            insertPlayAfterCurrentQueue(type, [app.current.search]);
            break;
        case 'replace':
            replaceQueue(type, [app.current.search]);
            break;
        case 'replacePlay':
            replacePlayQueue(type, [app.current.search]);
            break;
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
