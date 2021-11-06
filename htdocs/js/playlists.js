"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function initPlaylists() {
    document.getElementById('modalAddToPlaylist').addEventListener('shown.bs.modal', function () {
        if (!document.getElementById('addStreamFrm').classList.contains('hide')) {
            document.getElementById('streamUrl').focus();
            document.getElementById('streamUrl').value = '';
        }
        else {
            document.getElementById('addToPlaylistPlaylist').focus();
        }
    });
    
    setDataId('addToPlaylistPlaylist', 'data-cb-filter', 'filterPlaylistsSelect');
    setDataId('addToPlaylistPlaylist', 'data-cb-filter-options', [1, 'addToPlaylistPlaylist']);

    document.getElementById('searchPlaylistsDetailStr').addEventListener('keyup', function(event) {
        if (event.key === 'Escape') {
            this.blur();
        }
        else {
            appGoto(app.current.card, app.current.tab, app.current.view, 
                0, app.current.limit, app.current.filter, app.current.sort, '-', this.value);
        }
    }, false);
    
    document.getElementById('searchPlaylistsListStr').addEventListener('keyup', function(event) {
        if (event.key === 'Escape') {
            this.blur();
        }
        else {
            appGoto(app.current.card, app.current.tab, app.current.view, 
                0, app.current.limit, app.current.filter, app.current.sort, '-', this.value);
        }
    }, false);
    
   document.getElementById('BrowsePlaylistsListList').addEventListener('click', function(event) {
        if (event.target.nodeName === 'TD') {
            if (getData(event.target.parentNode, 'data-smartpls-only') === false) {
                clickPlaylist(getData(event.target.parentNode, 'data-uri'), getData(event.target.parentNode, 'data-name'));
            }
            else {
                showNotification(tn('Playlist is empty'), '', 'playlist', 'warn')
            }
        }
        else if (event.target.nodeName === 'A') {
            showPopover(event);
        }
    }, false);

    document.getElementById('BrowsePlaylistsDetailList').addEventListener('click', function(event) {
        if (event.target.parentNode.parentNode.nodeName === 'TFOOT') {
            return;
        }
        if (event.target.nodeName === 'TD') {
            clickSong(getData(event.target.parentNode, 'data-uri'), getData(event.target.parentNode, 'data-name'));
        }
        else if (event.target.nodeName === 'A') {
            showPopover(event);
        }
    }, false);
}

function parsePlaylistsList(obj) {
    if (checkResult(obj, 'BrowsePlaylistsList', 3) === false) {
        return;
    }

    const rowTitle = webuiSettingsDefault.clickPlaylist.validValues[settings.webuiSettings.clickPlaylist];
    updateTable(obj, 'BrowsePlaylistsList', function(row, data) {
        setData(row, 'data-uri', data.uri);
        setData(row, 'data-type', data.Type);
        setData(row, 'data-name', data.name);
        setData(row, 'data-smartpls-only', data.smartplsOnly);
        row.setAttribute('title', tn(rowTitle));
    }, function(row, data) {
        row.appendChild(
            elCreateNode('td', {"data-col": "Type"},
                elCreateText('span', {"class": ["mi"]}, (data.Type === 'smartpls' ? 'queue_music' : 'list'))
            )
        );
        row.appendChild(
            elCreateText('td', {}, data.name)
        );
        row.appendChild(
            elCreateText('td', {}, localeDate(data.lastModified))
        );
        row.appendChild(
            elCreateNode('td', {},
                elCreateText('a', {"data-col": "Action", "href": "#", "class": ["mi", "color-darkgrey"], "title": tn('Actions')}, ligatureMore)
            )
        );
    });
}

function parsePlaylistsDetail(obj) {
    const table = document.getElementById('BrowsePlaylistsDetailList');
    const tfoot = table.getElementsByTagName('tfoot')[0];
    const colspan = settings.colsBrowsePlaylistsDetail.length + 1;

    if (checkResult(obj, 'BrowsePlaylistsDetail', colspan) === false) {
        elClear(tfoot);
        return;
    }

    if (isMPDplaylist(obj.result.plist) === false || obj.result.smartpls === true) {
        setDataId('BrowsePlaylistsDetailList', 'data-ro', 'true');
        elHideId('playlistContentBtns');
        elShowId('smartPlaylistContentBtns');
    }
    else {
        setDataId('BrowsePlaylistsDetailList', 'data-ro', 'false');
        elShowId('playlistContentBtns');
        elHideId('smartPlaylistContentBtns');
    }
    setDataId('BrowsePlaylistsDetailList', 'data-uri', obj.result.plist);
    document.getElementById('BrowsePlaylistsDetailList').getElementsByTagName('caption')[0].textContent = 
        (obj.result.smartpls === true ? tn('Smart playlist') : tn('Playlist'))  + ': ' + obj.result.plist;
    const rowTitle = webuiSettingsDefault.clickSong.validValues[settings.webuiSettings.clickSong];

    elReplaceChild(tfoot, elCreateNode('tr', {}, 
        elCreateNode('td', {"colspan": colspan}, 
            elCreateText('small', {}, tn('Num songs', obj.result.totalEntities) + ' - ' + beautifyDuration(obj.result.totalTime))))
    );
    
    updateTable(obj, 'BrowsePlaylistsDetail', function(row, data) {
        row.setAttribute('id', 'playlistTrackId' + data.Pos);
        row.setAttribute('draggable', 'true');
        row.setAttribute('tabindex', 0);
        setData(row, 'data-type', data.Type);
        setData(row, 'data-uri', data.uri);
        setData(row, 'data-name', data.Title);
        setData(row, 'data-songpos', data.Pos);
        row.setAttribute('title', tn(rowTitle));
    });
}

//eslint-disable-next-line no-unused-vars
function playlistDetails(uri) {
    document.getElementById('BrowsePlaylistsListList').classList.add('opacity05');
    appGoto('Browse', 'Playlists', 'Detail', 0, undefined, uri, '-', '-', '');
}

//eslint-disable-next-line no-unused-vars
function playlistShuffle() {
    sendAPI("MYMPD_API_PLAYLIST_CONTENT_SHUFFLE", {
        "plist": getDataId('BrowsePlaylistsDetailList', 'data-uri')
    });
    document.getElementById('BrowsePlaylistsDetailList').classList.add('opacity05');    
}

//eslint-disable-next-line no-unused-vars
function playlistSort(tag) {
    sendAPI("MYMPD_API_PLAYLIST_CONTENT_SORT", {
        "plist": getDataId('BrowsePlaylistsDetailList', 'data-uri'),
        "tag": tag
    });
    document.getElementById('BrowsePlaylistsDetailList').classList.add('opacity05');    
}

//eslint-disable-next-line no-unused-vars
function updateSmartPlaylists(force) {
    sendAPI("MYMPD_API_SMARTPLS_UPDATE_ALL", {
        "force":force
    });
}

//eslint-disable-next-line no-unused-vars
function removeFromPlaylist(mode, plist, start, end) {
    if (mode === 'range') {
        sendAPI("MYMPD_API_PLAYLIST_CONTENT_RM_RANGE", {
            "plist": plist,
            "start": start,
            "end": end
        });
    }
    else if (mode === 'single') {
        sendAPI("MYMPD_API_PLAYLIST_CONTENT_RM_SONG", {
            "plist": plist,
            "pos": start
        });
    }
    document.getElementById('BrowsePlaylistsDetailList').classList.add('opacity05');    
}

function parseSmartPlaylist(obj) {
    const nameEl = document.getElementById('saveSmartPlaylistName');
    nameEl.value = obj.result.plist;
    removeIsInvalid(document.getElementById('modalSaveSmartPlaylist'));
    document.getElementById('saveSmartPlaylistType').value = tn(obj.result.type);
    setDataId('saveSmartPlaylistType', 'data-value', obj.result.type);
    elHideId('saveSmartPlaylistSearch');
    elHideId('saveSmartPlaylistSticker');
    elHideId('saveSmartPlaylistNewest');
    
    if (obj.result.type === 'search') {
        elShowId('saveSmartPlaylistSearch');
        document.getElementById('inputSaveSmartPlaylistExpression').value = obj.result.expression;
    }
    else if (obj.result.type === 'sticker') {
        elShowId('saveSmartPlaylistSticker');
        document.getElementById('selectSaveSmartPlaylistSticker').value = obj.result.sticker;
        document.getElementById('inputSaveSmartPlaylistStickerMaxentries').value = obj.result.maxentries;
        document.getElementById('inputSaveSmartPlaylistStickerMinvalue').value = obj.result.minvalue;
    }
    else if (obj.result.type === 'newest') {
        elShowId('saveSmartPlaylistNewest');
        document.getElementById('inputSaveSmartPlaylistNewestTimerange').value = obj.result.timerange / 24 / 60 / 60;
    }
    hideModalAlert();
    uiElements.modalSaveSmartPlaylist.show();
    nameEl.focus();
}

//eslint-disable-next-line no-unused-vars
function saveSmartPlaylist() {
    const name = document.getElementById('saveSmartPlaylistName').value;
    const type = getDataId('saveSmartPlaylistType', 'data-value');
    const sort = getSelectValueId('saveSmartPlaylistSort');
    if (validatePlname(name) === false) {
        document.getElementById('saveSmartPlaylistName').classList.add('is-invalid');
        return;
    }

    if (type === 'search') {
        sendAPI("MYMPD_API_SMARTPLS_SEARCH_SAVE", {
            "plist": name, 
            "expression": document.getElementById('inputSaveSmartPlaylistExpression').value,
            "sort": sort
        }, saveSmartPlaylistClose, true);
    }
    else if (type === 'sticker') {
        const maxentriesEl = document.getElementById('inputSaveSmartPlaylistStickerMaxentries');
        if (!validateInt(maxentriesEl)) {
            return;
        }
        const minvalueEl = document.getElementById('inputSaveSmartPlaylistStickerMinvalue');
        if (!validateInt(minvalueEl)) {
            return;
        }
        sendAPI("MYMPD_API_SMARTPLS_STICKER_SAVE", {
            "plist": name,
            "sticker": getSelectValueId('selectSaveSmartPlaylistSticker'),
            "maxentries": Number(maxentriesEl.value), 
            "minvalue": Number(minvalueEl.value),
            "sort": sort
        }, saveSmartPlaylistClose, true);
    }
    else if (type === 'newest') {
        const timerangeEl = document.getElementById('inputSaveSmartPlaylistNewestTimerange');
        if (!validateInt(timerangeEl)) {
            return;
        }
        sendAPI("MYMPD_API_SMARTPLS_NEWEST_SAVE", {
            "plist": name,
            "timerange": Number(timerangeEl.value) * 60 * 60 * 24,
            "sort": sort
        }, saveSmartPlaylistClose, true);
    }
    else {
        document.getElementById('saveSmartPlaylistType').classList.add('is-invalid');
    }
}

function saveSmartPlaylistClose(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        hideModalAlert();
        uiElements.modalSaveSmartPlaylist.hide();
        showNotification(tn('Saved smart playlist'), '', 'playlist', 'info');
    }
}

//eslint-disable-next-line no-unused-vars
function addSmartpls(type) {
    const obj = {"jsonrpc": "2.0", "id": 0, "result": {"method": "MYMPD_API_SMARTPLS_GET"}};
    if (type === 'mostPlayed') {
        obj.result.plist = settings.smartplsPrefix + (settings.smartplsPrefix !== '' ? '-' : '') + 'mostPlayed';
        obj.result.type = 'sticker';
        obj.result.sticker = 'playCount';
        obj.result.maxentries = 200;
        obj.result.minvalue = 10;
    }
    else if (type === 'newest') {
        obj.result.plist = settings.smartplsPrefix + (settings.smartplsPrefix !== '' ? '-' : '') + 'newestSongs';
        obj.result.type = 'newest';
        //14 days
        obj.result.timerange = 1209600;
    }
    else if (type === 'bestRated') {
        obj.result.plist = settings.smartplsPrefix + (settings.smartplsPrefix !== '' ? '-' : '') + 'bestRated';
        obj.result.type = 'sticker';
        obj.result.sticker = 'like';
        obj.result.maxentries = 200;
        obj.result.minvalue = 2;
    }
    parseSmartPlaylist(obj);
}

//eslint-disable-next-line no-unused-vars
function deletePlaylists() {
    btnWaiting(document.getElementById('btnDeletePlaylists'), true);
    sendAPI("MYMPD_API_PLAYLIST_RM_ALL", {
        "type": getSelectValueId('selectDeletePlaylists')
    }, function() {
        btnWaiting(document.getElementById('btnDeletePlaylists'), false);
    });
}

function filterPlaylistsSelect(type, elId, searchstr, selectedPlaylist) {
    sendAPI("MYMPD_API_PLAYLIST_LIST", {
        "searchstr": searchstr,
        "offset": 0,
        "limit": settings.webuiSettings.uiMaxElementsPerPage,
        "type": type
    }, function(obj) {
        populatePlaylistSelect(obj, elId, selectedPlaylist);
    });
}

//populates the custom input element mympd-select-search
function populatePlaylistSelect(obj, playlistSelectId, selectedPlaylist) {
    const selectEl = document.getElementById(playlistSelectId);
    if (selectedPlaylist !== undefined) {
        selectEl.value = selectedPlaylist;
    }
    elClear(selectEl.filterResult);
    if (playlistSelectId === 'selectJukeboxPlaylist' || 
             playlistSelectId === 'selectAddToQueuePlaylist' ||
             playlistSelectId === 'selectTimerPlaylist') 
    {
        selectEl.filterResult.appendChild(elCreateText('option', {"value": "Database"}, tn('Database')));
    }

    for (let i = 0; i < obj.result.returnedEntities; i++) {
        selectEl.filterResult.appendChild(elCreateText('option', {"value": obj.result.data[i].uri}, obj.result.data[i].uri));
        if (obj.result.data[i].uri === selectedPlaylist) {
            selectEl.filterResult.lastChild.setAttribute('selected', 'selected');
        }
    }
}

//eslint-disable-next-line no-unused-vars
function showAddToPlaylistCurrentSong() {
    const uri = getDataId('currentTitle', 'data-uri');
    if (uri !== '') {
        showAddToPlaylist(uri, '');
    }
}

//eslint-disable-next-line no-unused-vars
function showAddToPlaylistCurrentSearch() {
    showAddToPlaylist('SEARCH', app.current.search);
}

//eslint-disable-next-line no-unused-vars
function showAddToPlaylistFromFilesystem() {
    showAddToPlaylist(app.current.search, '');
}

function showAddToPlaylist(uri, searchstr) {
    document.getElementById('addToPlaylistUri').value = uri;
    document.getElementById('addToPlaylistSearch').value = searchstr;
    document.getElementById('addToPlaylistPlaylist').value = '';
    document.getElementById('addToPlaylistPlaylist').filterInput.value = '';
    document.getElementById('addToPlaylistPosAppend').checked = 'checked';
    toggleBtnId('toggleAddToPlaylistBtn', 0);
    const streamUrl = document.getElementById('streamUrl');
    streamUrl.focus();
    streamUrl.value = '';
    removeIsInvalid(document.getElementById('modalAddToPlaylist'));
    if (uri !== 'stream') {
        elHideId('addStreamFrm');
        elShowId('addToPlaylistFrm');
        elHideId('addToPlaylistPosInsertRow');
        document.getElementById('addToPlaylistCaption').textContent = tn('Add to playlist');
    }
    else {
        elShowId('addStreamFrm');
        elHideId('addToPlaylistFrm');
        elShowId('addToPlaylistPosInsertRow');
        document.getElementById('addToPlaylistCaption').textContent = tn('Add stream');
    }
    uiElements.modalAddToPlaylist.show();
    if (features.featPlaylists) {
        filterPlaylistsSelect(1, 'addToPlaylistPlaylist', '');
    }
}

//eslint-disable-next-line no-unused-vars
function toggleAddToPlaylistFrm() {
    const btn = document.getElementById('toggleAddToPlaylistBtn');
    toggleBtn(toggleAddToPlaylistBtn);
    if (btn.classList.contains('active')) {
        //add to playlist
        elShowId('addToPlaylistFrm');
        document.getElementById('addToPlaylistPosInsert').nextElementSibling.textContent = tn('Insert at start of playlist');
    }    
    else {
        //add to queue
        elHideId('addToPlaylistFrm');
        document.getElementById('addToPlaylistPosInsert').nextElementSibling.textContent = tn('Insert after current playing song');
    }
}

//eslint-disable-next-line no-unused-vars
function addToPlaylist() {
    let uri = document.getElementById('addToPlaylistUri').value;
    const mode = getRadioBoxValueId('addToPlaylistPos');
    let type;
    switch(uri) {
        case 'SEARCH':
            uri = app.current.search;
            type = 'search';
            break;
        case 'ALBUM':
            uri = document.getElementById('addToPlaylistSearch').value;
            type = 'search';
            break;
        case 'stream':
            uri = document.getElementById('streamUrl').value;
            if (uri === '' || isStreamUri(uri) === false) {
                document.getElementById('streamUrl').classList.add('is-invalid');
                return;
            }
            type = 'song'
        default:
            type = 'song';
    }

    if (document.getElementById('toggleAddToPlaylistBtn').classList.contains('active')) {
        //add to playlist
        const plist = document.getElementById('addToPlaylistPlaylist').value;
        if (validatePlnameEl(plist) === false) {
            return;
        }
        switch(mode) {
            case 'append': 
                appendPlaylist(type, uri, plist, addToPlaylistClose);
                break;
            case 'insert':
                insertPlaylist(type, uri, plist, 0, 1, addToPlaylistClose);
                break;
            case 'replace':
                replacePlaylist(type, uri, plist, addToPlaylistClose);
        }
    }
    else {
        //add to queue
        switch(mode) {
            case 'append': 
                appendQueue(type, uri, addToPlaylistClose);
                break;
            case 'insert':
                insertQueue(type, uri, 0, 1, addToPlaylistClose);
                break;
            case 'replace':
                replaceQueue(type, uri, addToPlaylistClose);
        }
    }
}

function addToPlaylistClose(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        hideModalAlert();
        uiElements.modalAddToPlaylist.hide();
    }
}

function appendPlaylist(type, uri, plist, callback) {
    switch(type) {
        case 'song':
        case 'dir':
            sendAPI("MYMPD_API_PLAYLIST_CONTENT_APPEND_URI", {
                "uri": uri,
                "plist": plist
            }, callback, true);
            break;
        case 'search':
            sendAPI("MYMPD_API_PLAYLIST_CONTENT_APPEND_SEARCH", {
                "expression": uri,
                "plist": plist
            }, callback, true);
    }
}

function insertPlaylist(type, uri, plist, to, whence, callback) {
    switch(type) {
        case 'song':
        case 'dir':
            sendAPI("MYMPD_API_PLAYLIST_CONTENT_INSERT_URI", {
                "uri": uri,
                "plist": plist,
                "to": to,
                "whence": whence
            }, callback, true);
            break;
//        case 'search':
//not implemented by mpd
//            sendAPI("MYMPD_API_PLAYLIST_CONTENT_INSERT_SEARCH", {
//                "uri": uri,
//                "plist": plist,
//                "to": to
//            }, callback, true);
//            break;
    }
}

function replacePlaylist(type, uri, plist, callback) {
    switch(type) {
        case 'song':
        case 'dir':
            sendAPI("MYMPD_API_PLAYLIST_CONTENT_REPLACE_URI", {
                "uri": uri,
                "plist": plist
            }, callback, true);
            break;
        case 'search':
            sendAPI("MYMPD_API_PLAYLIST_CONTENT_REPLACE_SEARCH", {
                "expression": uri,
                "plist": plist
            }, callback, true);
    }
}

//eslint-disable-next-line no-unused-vars
function showRenamePlaylist(from) {
    removeIsInvalid(document.getElementById('modalRenamePlaylist'));
    hideModalAlert();
    uiElements.modalRenamePlaylist.show();
    document.getElementById('renamePlaylistFrom').value = from;
    document.getElementById('renamePlaylistTo').value = '';
}

//eslint-disable-next-line no-unused-vars
function renamePlaylist() {
    const from = document.getElementById('renamePlaylistFrom').value;
    const to = document.getElementById('renamePlaylistTo').value;
    if (to !== from && validatePlname(to) === true) {
        sendAPI("MYMPD_API_PLAYLIST_RENAME", {
            "plist": from, 
            "newName": to
        }, renamePlaylistClose, true);
    }
    else {
        document.getElementById('renamePlaylistTo').classList.add('is-invalid');
    }
}

function renamePlaylistClose(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        hideModalAlert();
        uiElements.modalRenamePlaylist.hide();
    }
}

//eslint-disable-next-line no-unused-vars
function showSmartPlaylist(plist) {
    sendAPI("MYMPD_API_SMARTPLS_GET", {
        "plist": plist
    }, parseSmartPlaylist);
}

//eslint-disable-next-line no-unused-vars
function updateSmartPlaylist(plist) {
    sendAPI("MYMPD_API_SMARTPLS_UPDATE", {
        "plist": plist
    });
}

//eslint-disable-next-line no-unused-vars
function updateSmartPlaylistClick() {
    sendAPI("MYMPD_API_SMARTPLS_UPDATE", {
        "plist": getDataId('BrowsePlaylistsDetailList', 'data-uri')
    });
    document.getElementById('BrowsePlaylistsDetailList').classList.add('opacity05');    
}

//eslint-disable-next-line no-unused-vars
function showDelPlaylist(plist, smartplsOnly) {
    showConfirm(tn('Do you really want to delete the playlist?', {"playlist": plist}), tn('Yes, delete it'), function() {
        sendAPI("MYMPD_API_PLAYLIST_RM", {
            "plist": plist,
            "smartplsOnly": smartplsOnly
        });
    });
}

//eslint-disable-next-line no-unused-vars
function showClearPlaylist() {
    const plist = getDataId('BrowsePlaylistsDetailList', 'data-uri');
    showConfirm(tn('Do you really want to clear the playlist?', {"playlist": plist}), tn('Yes, clear it'), function() {
        sendAPI("MYMPD_API_PLAYLIST_CONTENT_CLEAR", {
            "plist": plist
        });
        document.getElementById('BrowsePlaylistsDetailList').classList.add('opacity05');
    });
}

function playlistMoveTrack(from, to) {
    sendAPI("MYMPD_API_PLAYLIST_CONTENT_MOVE_SONG", {
        "plist": app.current.filter,
        "from": from,
        "to": to
    });
}

function isMPDplaylist(uri) {
    if (uri.indexOf('/') > -1 ||
        uri.indexOf('.m3u') > -1 ||
        uri.indexOf('.pls') > -1)
    {
        return false;
    }
    return true;
}
