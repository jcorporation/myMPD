"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

function search(x) {
    if (settings.featAdvsearch) {
        let expression = '(';
        let crumbs = domCache.searchCrumb.children;
        for (let i = 0; i < crumbs.length; i++) {
            expression += '(' + decodeURI(crumbs[i].getAttribute('data-filter')) + ')';
            if (x !== '') expression += ' AND ';
        }
        if (x !== '') {
            let match = document.getElementById('searchMatch');
            expression += '(' + app.current.filter + ' ' + match.options[match.selectedIndex].value + ' \'' + x +'\'))';
        }
        else
            expression += ')';
        if (expression.length <= 2)
            expression = '';
        appGoto('Search', undefined, undefined, '0/' + app.current.filter + '/' + app.current.sort + '/' + encodeURI(expression));
    }
    else
        appGoto('Search', undefined, undefined, '0/' + app.current.filter + '/' + app.current.sort + '/' + x);
}

function parseSearch(obj) {
    document.getElementById('panel-heading-search').innerText = gtPage('Num songs', obj.result.returnedEntities, obj.result.totalEntities);
    document.getElementById('cardFooterSearch').innerText = gtPage('Num songs', obj.result.returnedEntities, obj.result.totalEntities);
    
    if (obj.result.returnedEntities > 0) {
        document.getElementById('searchAddAllSongs').removeAttribute('disabled');
        document.getElementById('searchAddAllSongsBtn').removeAttribute('disabled');
    } 
    else {
        document.getElementById('searchAddAllSongs').setAttribute('disabled', 'disabled');
        document.getElementById('searchAddAllSongsBtn').setAttribute('disabled', 'disabled');
    }
    parseFilesystem(obj);
}

function saveSearchAsSmartPlaylist() {
    parseSmartPlaylist({"jsonrpc":"2.0","id":0,"result":{"method":"MPD_API_SMARTPLS_GET", 
        "playlist":"",
        "type":"search",
        "tag": app.current.filter,
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
            "cols": settings.colsSearch, 
            "replace": replace});
    }
    else {
        sendAPI("MPD_API_DATABASE_SEARCH", {"plist": plist, 
            "filter": app.current.filter, 
            "searchstr": searchstr,
            "offset": 0, 
            "cols": settings.colsSearch, 
            "replace": replace});
    }
}
