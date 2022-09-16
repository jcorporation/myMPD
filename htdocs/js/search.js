"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function initSearch() {
    document.getElementById('SearchList').addEventListener('click', function(event) {
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
        const target = getParent(event.target, 'TR');
        if (target !== null) {
            clickSong(getData(target, 'uri'));
        }
    }, false);

    document.getElementById('searchTags').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            app.current.filter = getData(event.target, 'tag');
            doSearch(document.getElementById('searchStr').value);
        }
    }, false);

    document.getElementById('searchStr').addEventListener('keyup', function(event) {
        clearSearchTimer();
        const value = this.value;
        if (event.key === 'Escape') {
            this.blur();
        }
        else if (event.key === 'Enter')
        {
            if (value !== '') {
                const op = getSelectValueId('searchMatch');
                document.getElementById('searchCrumb').appendChild(createSearchCrumb(app.current.filter, op, value));
                elShowId('searchCrumb');
                this.value = '';
            }
            else {
                searchTimer = setTimeout(function() {
                    doSearch(value);
                }, searchTimerTimeout);
            }
        }
        else {
             searchTimer = setTimeout(function() {
                 doSearch(value);
             }, searchTimerTimeout);
         }
    }, false);

    document.getElementById('searchCrumb').addEventListener('click', function(event) {
        if (event.target.nodeName === 'SPAN') {
            //remove search expression
            event.preventDefault();
            event.stopPropagation();
            event.target.parentNode.remove();
            doSearch('');
        }
        else if (event.target.nodeName === 'BUTTON') {
            //edit search expression
            event.preventDefault();
            event.stopPropagation();
            document.getElementById('searchStr').value = unescapeMPD(getData(event.target, 'filter-value'));
            selectTag('searchTags', 'searchTagsDesc', getData(event.target, 'filter-tag'));
            document.getElementById('searchMatch').value = getData(event.target, 'filter-op');
            event.target.remove();
            app.current.filter = getData(event.target,'filter-tag');
            doSearch(document.getElementById('searchStr').value);
            if (document.getElementById('searchCrumb').childElementCount === 0) {
                elHideId('searchCrumb');
            }
        }
    }, false);

    document.getElementById('searchMatch').addEventListener('change', function() {
        doSearch(document.getElementById('searchStr').value);
    }, false);
}

function doSearch(x) {
    const expression = createSearchExpression(document.getElementById('searchCrumb'), app.current.filter, getSelectValueId('searchMatch'), x);
    appGoto('Search', undefined, undefined, 0, app.current.limit, app.current.filter, app.current.sort, '-', expression, 0);
}

function parseSearch(obj) {
    const table = document.getElementById('SearchList');
    const tfoot = table.getElementsByTagName('tfoot')[0];
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
            elCreateNode('tr', {},
                elCreateText('td', {"colspan": colspan}, tn('Num songs', obj.result.totalEntities))
            )
        );
    }
}

//eslint-disable-next-line no-unused-vars
function saveSearchAsSmartPlaylist() {
    parseSmartPlaylist({"jsonrpc":"2.0","id":0,"result":{"method":"MYMPD_API_SMARTPLS_GET",
        "plist": "",
        "type": "search",
        "sort": "",
        "expression": app.current.search
    }});
}

//eslint-disable-next-line no-unused-vars
function addAllFromSearch(mode, type) {
    switch(mode) {
        case 'append':
            appendQueue(type, app.current.search);
            break;
        case 'appendPlay':
            appendPlayQueue(type, app.current.search);
            break;
        case 'insertAfterCurrent':
            insertAfterCurrentQueue(type, app.current.search);
            break;
        case 'insertPlayAfterCurrent':
            insertPlayAfterCurrentQueue(type, app.current.search);
            break;
        case 'replace':
            replaceQueue(type, app.current.search, false);
            break;
        case 'replacePlay':
            replacePlayQueue(type, app.current.search, true);
            break;
    }
}
