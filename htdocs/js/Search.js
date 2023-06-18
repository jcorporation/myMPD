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
    const searchStrEl = document.getElementById('searchStr');
    const searchCrumbEl = document.getElementById('searchCrumb');
    setFocus(searchStrEl);
    createSearchCrumbs(app.current.search, searchStrEl, searchCrumbEl);
    
    if (app.current.search === '') {
        document.getElementById('searchStr').value = '';
    }
    if (searchStrEl.value.length >= 2 ||
        searchCrumbEl.children.length > 0)
    {
        if (app.current.sort.tag === '-') {
            app.current.sort.tag = settings.tagList.includes('Title') ? 'Title' : '-';
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
        const SearchListEl = document.getElementById('SearchList');
        elClear(SearchListEl.querySelector('tbody'));
        elClear(SearchListEl.querySelector('tfoot'));
        elDisableId('searchAddAllSongs');
        elDisableId('searchAddAllSongsBtn');
        unsetUpdateViewId('SearchList');
        setPagination(0, 0);
    }
    selectTag('searchTags', 'searchTagsDesc', app.current.filter);
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
            if (colName === null ||
                colName === 'Duration' ||
                colName.indexOf('sticker') === 0)
            {
                //by this fields can not be sorted
                return;
            }
            toggleSort(event.target, colName);
            appGoto(app.current.card, app.current.tab, app.current.view,
                app.current.offset, app.current.limit, app.current.filter, app.current.sort, '-', app.current.search);
            return;
        }
        //table body
        const target = event.target.closest('TR');
        if (checkTargetClick(target) === true) {
            clickSong(getData(target, 'uri'), event);
        }
    }, false);

    document.getElementById('searchTags').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            app.current.filter = getData(event.target, 'tag');
            doSearch(document.getElementById('searchStr').value);
        }
    }, false);

    document.getElementById('searchStr').addEventListener('keydown', function(event) {
        //handle Enter key on keydown for IME composing compatibility
        if (event.key !== 'Enter') {
            return;
        }
        clearSearchTimer();
        const value = this.value;
        if (value !== '') {
            const op = getSelectValueId('searchMatch');
            const crumbEl = document.getElementById('searchCrumb');
            crumbEl.appendChild(createSearchCrumb(app.current.filter, op, value));
            elShow(crumbEl);
            this.value = '';
        }
        else {
            searchTimer = setTimeout(function() {
                doSearch(value);
            }, searchTimerTimeout);
        }
    }, false);

    document.getElementById('searchStr').addEventListener('keyup', function(event) {
        if (ignoreKeys(event) === true) {
            return;
        }
        clearSearchTimer();
        const value = this.value;
        searchTimer = setTimeout(function() {
            doSearch(value);
        }, searchTimerTimeout);
    }, false);

    document.getElementById('searchCrumb').addEventListener('click', function(event) {
        if (event.target.nodeName === 'SPAN') {
            //remove search expression
            event.preventDefault();
            event.stopPropagation();
            event.target.parentNode.remove();
            doSearch('');
            document.getElementById('searchStr').updateBtn();
        }
        else if (event.target.nodeName === 'BUTTON') {
            //edit search expression
            event.preventDefault();
            event.stopPropagation();
            const searchStrEl = document.getElementById('searchStr');
            searchStrEl.value = unescapeMPD(getData(event.target, 'filter-value'));
            selectTag('searchTags', 'searchTagsDesc', getData(event.target, 'filter-tag'));
            document.getElementById('searchMatch').value = getData(event.target, 'filter-op');
            event.target.remove();
            app.current.filter = getData(event.target,'filter-tag');
            doSearch(searchStrEl.value);
            if (document.getElementById('searchCrumb').childElementCount === 0) {
                elHideId('searchCrumb');
            }
            searchStrEl.updateBtn();
        }
    }, false);

    document.getElementById('searchMatch').addEventListener('change', function() {
        doSearch(document.getElementById('searchStr').value);
    }, false);
}

/**
 * Searches for songs
 * @param {string} value current search input value
 * @returns {void}
 */
function doSearch(value) {
    const expression = createSearchExpression(document.getElementById('searchCrumb'), app.current.filter, getSelectValueId('searchMatch'), value);
    appGoto('Search', undefined, undefined, 0, app.current.limit, app.current.filter, app.current.sort, '-', expression, 0);
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
        row.setAttribute('tabindex', 0);
        row.setAttribute('title', rowTitle);
        setData(row, 'name', data.Title);
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
    showAddToPlaylist(['SEARCH'], app.current.search);
}
