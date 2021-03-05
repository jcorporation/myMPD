"use strict";
// SPDX-License-Identifier: GPL-2.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function initSearch() {
    document.getElementById('SearchList').addEventListener('click', function(event) {
        if (event.target.nodeName === 'TD') {
            clickSong(getAttDec(event.target.parentNode, 'data-uri'), getAttDec(event.target.parentNode, 'data-name'));
        }
        else if (event.target.nodeName === 'A') {
            showMenu(event.target, event);
        }
    }, false);
    
    document.getElementById('searchtags').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            app.current.filter = getAttDec(event.target, 'data-tag');
            doSearch(document.getElementById('searchstr').value);
        }
    }, false);

    document.getElementById('searchstr').addEventListener('keyup', function(event) {
        if (event.key === 'Escape') {
            this.blur();
        }
        else if (event.key === 'Enter' && settings.featAdvsearch) {
            if (this.value !== '') {
                const op = getSelectValue(document.getElementById('searchMatch'));
                document.getElementById('searchCrumb').appendChild(createSearchCrumb(app.current.filter, op, this.value));
                document.getElementById('searchCrumb').classList.remove('hide');
                this.value = '';
            }
            else {
                doSearch(this.value);
            }
        }
        else {
            doSearch(this.value);
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
            document.getElementById('searchstr').value = unescapeMPD(getAttDec(event.target, 'data-filter-value'));
            selectTag('searchtags', 'searchtagsdesc', getAttDec(event.target, 'data-filter-tag'));
            document.getElementById('searchMatch').value = getAttDec(event.target, 'data-filter-op');
            event.target.remove();
            app.current.filter = getAttDec(event.target,'data-filter-tag');
            doSearch(document.getElementById('searchstr').value);
            if (document.getElementById('searchCrumb').childElementCount === 0) {
                document.getElementById('searchCrumb').classList.add('hide');
            }
        }
    }, false);

    document.getElementById('searchMatch').addEventListener('change', function() {
        doSearch(document.getElementById('searchstr').value);
    }, false);
    
    document.getElementById('SearchList').getElementsByTagName('tr')[0].addEventListener('click', function(event) {
        if (settings.featAdvsearch === false || event.target.nodeName !== 'TH' ||
            event.target.innerHTML === '') {
            return;
        }
        let col = event.target.getAttribute('data-col');
        if (col === 'Duration' || col.indexOf('sticker') === 0) {
            return;
        }
        let sortcol = app.current.sort;
        let sortdesc = true;
                
        if (sortcol === col || sortcol === '-' + col) {
            if (sortcol.indexOf('-') === 0) {
                sortdesc = true;
                col = sortcol.substring(1);
            }
            else {
                sortdesc = false;
            }
        }
        if (sortdesc === false) {
            sortcol = '-' + col;
            sortdesc = true;
        }
        else {
            sortdesc = false;
            sortcol = col;
        }
                
        const s = document.getElementById('SearchList').getElementsByClassName('sort-dir');
        for (let i = 0; i < s.length; i++) {
            s[i].remove();
        }
        app.current.sort = sortcol;
        event.target.innerHTML = t(col) + '<span class="sort-dir mi pull-right">' + 
            (sortdesc === true ? 'arrow_drop_up' : 'arrow_drop_down') + '</span>';
        appGoto(app.current.app, app.current.tab, app.current.view,
            app.current.offset, app.current.limit, app.current.filter,  app.current.sort, '-', app.current.search);
    }, false);
}

function doSearch(x) {
    if (settings.featAdvsearch) {
        const expression = createSearchExpression(document.getElementById('searchCrumb'), app.current.filter, getSelectValue('searchMatch'), x);
        appGoto('Search', undefined, undefined, '0', app.current.limit, app.current.filter, app.current.sort, '-', expression);
    }
    else {
        appGoto('Search', undefined, undefined, '0', app.current.limit, app.current.filter, app.current.sort, '-', x);
    }
}

function parseSearch(obj) {
    if (obj.result.returnedEntities > 0) {
        enableEl('searchAddAllSongs');
        enableEl('searchAddAllSongsBtn');
    } 
    else {
        disableEl('searchAddAllSongs');
        disableEl('searchAddAllSongsBtn');
    }

    updateTable(obj, 'Search', function(row, data) {
        setAttEnc(row, 'data-type', data.Type);
        setAttEnc(row, 'data-uri', data.uri);
        row.setAttribute('tabindex', 0);
        if (settings.featTags === true && settings.featAdvsearch === true) {
            //add artist and album information for album actions
            if (data.Album !== undefined) {
                setAttEnc(row, 'data-album', data.Album);
            }
            if (data[tagAlbumArtist] !== undefined) {
                setAttEnc(row, 'data-albumartist', data[tagAlbumArtist]);
            }
        }
        setAttEnc(row, 'data-name', data.Title);
    });
}

//eslint-disable-next-line no-unused-vars
function saveSearchAsSmartPlaylist() {
    parseSmartPlaylist({"jsonrpc":"2.0","id":0,"result":{"method":"MPD_API_SMARTPLS_GET", 
        "playlist":"",
        "type":"search",
        "tag": settings.featAdvsearch === true ? 'expression' : app.current.filter,
        "searchstr": app.current.search}});
}

function addAllFromSearchPlist(plist, searchstr, replace) {
    if (searchstr === null) {
        searchstr = app.current.search;    
    }
    if (settings.featAdvsearch) {
        sendAPI("MPD_API_DATABASE_SEARCH_ADV", {"plist": plist, 
            "sort": "", 
            "sortdesc": false, 
            "expression": searchstr,
            "offset": 0,
            "limit": 0,
            "cols": settings.colsSearch, 
            "replace": replace});
    }
    else {
        sendAPI("MPD_API_DATABASE_SEARCH", {"plist": plist, 
            "filter": app.current.filter, 
            "searchstr": searchstr,
            "offset": 0,
            "limit": 0, 
            "cols": settings.colsSearch, 
            "replace": replace});
    }
}
