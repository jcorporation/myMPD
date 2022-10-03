"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function initPlaylists() {
    document.getElementById('modalAddToPlaylist').addEventListener('shown.bs.modal', function () {
        if (document.getElementById('addStreamFrm').classList.contains('d-none')) {
            setFocusId('addToPlaylistPlaylist');
        }
        else {
            setFocusId('streamUrl');
            document.getElementById('streamUrl').value = '';
        }
    });

    setDataId('addToPlaylistPlaylist', 'cb-filter', 'filterPlaylistsSelect');
    setDataId('addToPlaylistPlaylist', 'cb-filter-options', [1, 'addToPlaylistPlaylist']);

    document.getElementById('searchPlaylistsDetailStr').addEventListener('keyup', function(event) {
        clearSearchTimer();
        if (event.key === 'Escape') {
            this.blur();
        }
        else {
            const value = this.value;
            searchTimer = setTimeout(function() {
                appGoto(app.current.card, app.current.tab, app.current.view,
                    0, app.current.limit, app.current.filter, app.current.sort, '-', value);
            }, searchTimerTimeout);
        }
    }, false);

    document.getElementById('searchPlaylistsListStr').addEventListener('keyup', function(event) {
        clearSearchTimer();
        if (event.key === 'Escape') {
            this.blur();
        }
        else {
            const value = this.value;
            searchTimer = setTimeout(function() {
                appGoto(app.current.card, app.current.tab, app.current.view,
                    0, app.current.limit, app.current.filter, app.current.sort, '-', value);
            }, searchTimerTimeout);
        }
    }, false);

   document.getElementById('BrowsePlaylistsListList').addEventListener('click', function(event) {
        if (event.target.nodeName === 'TD') {
            if (getData(event.target.parentNode, 'smartpls-only') === false) {
                clickPlaylist(getData(event.target.parentNode, 'uri'), getData(event.target.parentNode, 'name'));
            }
            else {
                showNotification(tn('Playlist is empty'), '', 'playlist', 'warn')
            }
        }
        else if (event.target.nodeName === 'A') {
            //action td
            handleActionTdClick(event);
        }
    }, false);

    document.getElementById('BrowsePlaylistsDetailList').addEventListener('click', function(event) {
        if (event.target.parentNode.parentNode.nodeName === 'TFOOT') {
            return;
        }
        if (event.target.nodeName === 'TD') {
            clickSong(getData(event.target.parentNode, 'uri'), getData(event.target.parentNode, 'name'));
        }
        else if (event.target.nodeName === 'A') {
            //action td
            handleActionTdClick(event);
        }
    }, false);
}

function parsePlaylistsList(obj) {
    if (checkResultId(obj, 'BrowsePlaylistsListList') === false) {
        return;
    }

    const rowTitle = webuiSettingsDefault.clickPlaylist.validValues[settings.webuiSettings.clickPlaylist];
    updateTable(obj, 'BrowsePlaylistsList', function(row, data) {
        setData(row, 'uri', data.uri);
        setData(row, 'type', data.Type);
        setData(row, 'name', data.name);
        setData(row, 'smartpls-only', data.smartplsOnly);
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
            pEl.actionTd.cloneNode(true)
        );
    });
}

function parsePlaylistsDetail(obj) {
    const table = document.getElementById('BrowsePlaylistsDetailList');
    const tfoot = table.getElementsByTagName('tfoot')[0];
    const colspan = settings.colsBrowsePlaylistsDetail.length + 1;

    if (checkResultId(obj, 'BrowsePlaylistsDetailList') === false) {
        return;
    }

    if (isMPDplaylist(obj.result.plist) === false ||
        obj.result.smartpls === true)
    {
        setDataId('BrowsePlaylistsDetailList', 'ro', 'true');
        elHideId('playlistContentBtns');
        elShowId('smartPlaylistContentBtns');
    }
    else {
        setDataId('BrowsePlaylistsDetailList', 'ro', 'false');
        elShowId('playlistContentBtns');
        elHideId('smartPlaylistContentBtns');
    }

    setData(table, 'playlistlength', obj.result.totalEntities);
    setData(table, 'uri', obj.result.plist);
    setData(table, 'type', obj.result.smartpls === true ? 'smartpls' : 'plist');
    table.getElementsByTagName('caption')[0].textContent =
        (obj.result.smartpls === true ? tn('Smart playlist') : tn('Playlist')) + ': ' + obj.result.plist;
    const rowTitle = webuiSettingsDefault.clickSong.validValues[settings.webuiSettings.clickSong];

    elReplaceChild(tfoot,
        elCreateNode('tr', {},
            elCreateNode('td', {"colspan": colspan},
                elCreateNodes('small', {}, [
                    elCreateTextTn('span', {}, 'Num songs', obj.result.totalEntities), 
                    elCreateText('span', {}, smallSpace + nDash + smallSpace + beautifyDuration(obj.result.totalTime))
                ])
            )
        )
    );

    updateTable(obj, 'BrowsePlaylistsDetail', function(row, data) {
        row.setAttribute('id', 'playlistTrackId' + data.Pos);
        row.setAttribute('draggable', 'true');
        row.setAttribute('tabindex', 0);
        setData(row, 'type', data.Type);
        setData(row, 'uri', data.uri);
        setData(row, 'name', data.Title);
        setData(row, 'songpos', data.Pos);
        row.setAttribute('title', tn(rowTitle));
    });
}

//eslint-disable-next-line no-unused-vars
function playlistDetails(uri) {
    setUpdateViewId('BrowsePlaylistsListList');
    appGoto('Browse', 'Playlists', 'Detail', 0, undefined, uri, '-', '-', '');
}

//eslint-disable-next-line no-unused-vars
function playlistShuffle() {
    setUpdateViewId('BrowsePlaylistsDetailList');
    sendAPI("MYMPD_API_PLAYLIST_CONTENT_SHUFFLE", {
        "plist": getDataId('BrowsePlaylistsDetailList', 'uri')
    });
}

//eslint-disable-next-line no-unused-vars
function playlistSort(tag) {
    setUpdateViewId('BrowsePlaylistsDetailList');
    sendAPI("MYMPD_API_PLAYLIST_CONTENT_SORT", {
        "plist": getDataId('BrowsePlaylistsDetailList', 'uri'),
        "tag": tag
    });
}

//eslint-disable-next-line no-unused-vars
function updateSmartPlaylists(force) {
    sendAPI("MYMPD_API_SMARTPLS_UPDATE_ALL", {
        "force": force
    });
}

//eslint-disable-next-line no-unused-vars
function removeFromPlaylist(mode, plist, start, end) {
    switch(mode) {
        case 'range':
            sendAPI("MYMPD_API_PLAYLIST_CONTENT_RM_RANGE", {
                "plist": plist,
                "start": start,
                "end": end
            });
            break;
        case 'single':
            sendAPI("MYMPD_API_PLAYLIST_CONTENT_RM_SONG", {
                "plist": plist,
                "pos": start
            });
            break;
        default:
            return;
    }
    setUpdateViewId('BrowsePlaylistsDetailList');
}

function parseSmartPlaylist(obj) {
    document.getElementById('saveSmartPlaylistName').value = obj.result.plist;
    document.getElementById('saveSmartPlaylistType').value = tn(obj.result.type);
    document.getElementById('saveSmartPlaylistSort').value = obj.result.sort;
    setDataId('saveSmartPlaylistType', 'value', obj.result.type);
    elHideId('saveSmartPlaylistSearch');
    elHideId('saveSmartPlaylistSticker');
    elHideId('saveSmartPlaylistNewest');

    switch(obj.result.type) {
        case 'search':
            elShowId('saveSmartPlaylistSearch');
            document.getElementById('inputSaveSmartPlaylistExpression').value = obj.result.expression;
            break;
        case 'sticker':
            elShowId('saveSmartPlaylistSticker');
            document.getElementById('selectSaveSmartPlaylistSticker').value = obj.result.sticker;
            document.getElementById('inputSaveSmartPlaylistStickerMaxentries').value = obj.result.maxentries;
            document.getElementById('inputSaveSmartPlaylistStickerMinvalue').value = obj.result.minvalue;
            break;
        case 'newest':
            elShowId('saveSmartPlaylistNewest');
            document.getElementById('inputSaveSmartPlaylistNewestTimerange').value = obj.result.timerange / 24 / 60 / 60;
            break;
    }
    cleanupModalId('modalSaveSmartPlaylist');
    uiElements.modalSaveSmartPlaylist.show();
}

//eslint-disable-next-line no-unused-vars
function saveSmartPlaylist() {
    cleanupModalId('modalSaveSmartPlaylist');

    const name = document.getElementById('saveSmartPlaylistName').value;
    const type = getDataId('saveSmartPlaylistType', 'value');
    const sort = getSelectValueId('saveSmartPlaylistSort');
    if (validatePlname(name) === false) {
        document.getElementById('saveSmartPlaylistName').classList.add('is-invalid');
        return;
    }

    switch(type) {
        case 'search':
            sendAPI("MYMPD_API_SMARTPLS_SEARCH_SAVE", {
                "plist": name,
                "expression": document.getElementById('inputSaveSmartPlaylistExpression').value,
                "sort": sort
            }, saveSmartPlaylistClose, true);
            break;
        case 'sticker': {
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
            break;
        }
        case 'newest': {
            const timerangeEl = document.getElementById('inputSaveSmartPlaylistNewestTimerange');
            if (!validateInt(timerangeEl)) {
                return;
            }
            sendAPI("MYMPD_API_SMARTPLS_NEWEST_SAVE", {
                "plist": name,
                "timerange": Number(timerangeEl.value) * 60 * 60 * 24,
                "sort": sort
            }, saveSmartPlaylistClose, true);
            break;
        }
        default:
            document.getElementById('saveSmartPlaylistType').classList.add('is-invalid');
    }
}

function saveSmartPlaylistClose(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        uiElements.modalSaveSmartPlaylist.hide();
        showNotification(tn('Saved smart playlist'), '', 'playlist', 'info');
    }
}

//eslint-disable-next-line no-unused-vars
function addSmartpls(type) {
    const obj = {"jsonrpc": "2.0", "id": 0, "result": {"method": "MYMPD_API_SMARTPLS_GET"}};
    switch(type) {
        case 'mostPlayed':
            obj.result.plist = settings.smartplsPrefix + (settings.smartplsPrefix !== '' ? '-' : '') + 'mostPlayed';
            obj.result.type = 'sticker';
            obj.result.sticker = 'playCount';
            obj.result.maxentries = 200;
            obj.result.minvalue = 10;
            break;
        case 'newest':
            obj.result.plist = settings.smartplsPrefix + (settings.smartplsPrefix !== '' ? '-' : '') + 'newestSongs';
            obj.result.type = 'newest';
            //14 days
            obj.result.timerange = 1209600;
            break;
        case 'bestRated':
            obj.result.plist = settings.smartplsPrefix + (settings.smartplsPrefix !== '' ? '-' : '') + 'bestRated';
            obj.result.type = 'sticker';
            obj.result.sticker = 'like';
            obj.result.maxentries = 200;
            obj.result.minvalue = 2;
            break;
    }
    parseSmartPlaylist(obj);
}

//eslint-disable-next-line no-unused-vars
function deletePlaylists() {
    sendAPI("MYMPD_API_PLAYLIST_RM_ALL", {
        "type": getSelectValueId('selectDeletePlaylists')
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
        //set input element values
        selectEl.value = selectedPlaylist === 'Database' ? tn('Database'): selectedPlaylist;
        setData(selectEl, 'value', selectedPlaylist);
    }
    elClear(selectEl.filterResult);
    if (playlistSelectId === 'selectJukeboxPlaylist' ||
        playlistSelectId === 'selectAddToQueuePlaylist' ||
        playlistSelectId === 'selectTimerPlaylist')
    {
        selectEl.addFilterResult('Database', 'Database');
    }

    for (let i = 0; i < obj.result.returnedEntities; i++) {
        selectEl.addFilterResultPlain(obj.result.data[i].uri);
        if (obj.result.data[i].uri === selectedPlaylist) {
            selectEl.filterResult.lastChild.classList.add('active');
        }
    }
}

//eslint-disable-next-line no-unused-vars
function showAddToPlaylistCurrentSong() {
    const uri = getDataId('currentTitle', 'uri');
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
    cleanupModalId('modalAddToPlaylist');
    document.getElementById('addToPlaylistUri').value = uri;
    document.getElementById('addToPlaylistSearch').value = searchstr;
    document.getElementById('addToPlaylistPlaylist').value = '';
    document.getElementById('addToPlaylistPlaylist').filterInput.value = '';
    document.getElementById('addToPlaylistPosAppend').checked = 'checked';
    const streamUrl = document.getElementById('streamUrl');
    setFocus(streamUrl);
    streamUrl.value = '';
    if (uri === 'STREAM') {
        //add stream
        toggleAddToPlaylistFrm(document.getElementById('toggleAddToPlaylistQueue'));
        elShowId('addStreamFrm');
        document.getElementById('addToPlaylistCaption').textContent = tn('Add stream');
    }
    else {
        //add to playlist
        toggleAddToPlaylistFrm(document.getElementById('toggleAddToPlaylistPlaylist'));
        document.getElementById('addToPlaylistCaption').textContent = tn('Add to playlist');
    }
    uiElements.modalAddToPlaylist.show();
    if (features.featPlaylists) {
        filterPlaylistsSelect(1, 'addToPlaylistPlaylist', '');
    }
}

//eslint-disable-next-line no-unused-vars
function toggleAddToPlaylistFrm(target) {
    toggleBtnGroup(target);
    if (target.getAttribute('id') === 'toggleAddToPlaylistPlaylist') {
        //add to playlist
        elShowId('addToPlaylistFrm');
        elShowId('addToPlaylistPosInsertFirstRow');
        elHideId('addToPlaylistPosInsertRow');
        elHideId('addToPlaylistPosAppendPlayRow');
        elHideId('addToPlaylistPosReplacePlayRow');
    }
    else {
        //add to queue
        elHideId('addToPlaylistFrm');
        elHideId('addToPlaylistPosInsertFirstRow');
        elShowId('addToPlaylistPosInsertRow');
        elShowId('addToPlaylistPosAppendPlayRow');
        elShowId('addToPlaylistPosReplacePlayRow');
    }
}

//eslint-disable-next-line no-unused-vars
function addToPlaylist() {
    cleanupModalId('modalAddToPlaylist');
    let uri = document.getElementById('addToPlaylistUri').value;
    const mode = getRadioBoxValueId('addToPlaylistPos');
    let type;
    switch(uri) {
        case 'SEARCH':
        case 'ALBUM':
            uri = document.getElementById('addToPlaylistSearch').value;
            type = 'search';
            break;
        case 'STREAM': {
            const streamUrlEl = document.getElementById('streamUrl');
            if (validateStream(streamUrlEl) === false) {
                return;
            }
            uri = streamUrlEl.value;
            type = 'stream';
            break;
        }
        default:
            //files and dirs
            type = 'song';
    }

    if (document.getElementById('addToPlaylistFrm').classList.contains('d-none') === false) {
        //add to playlist
        const plistEl = document.getElementById('addToPlaylistPlaylist');
        if (validatePlnameEl(plistEl) === false) {
            return;
        }
        switch(mode) {
            case 'append':
                appendPlaylist(type, uri, plistEl.value, addToPlaylistClose);
                break;
            case 'insertFirst':
                insertPlaylist(type, uri, plistEl.value, 0, addToPlaylistClose);
                break;
            case 'replace':
                replacePlaylist(type, uri, plistEl.value, addToPlaylistClose);
                break;
        }
    }
    else {
        //add to queue
        switch(mode) {
            case 'append':
                appendQueue(type, uri, addToPlaylistClose);
                break;
            case 'appendPlay':
                appendPlayQueue(type, uri, addToPlaylistClose);
                break;
            case 'insertAfterCurrent':
                insertAfterCurrentQueue(type, uri,addToPlaylistClose);
                break;
            case 'insertPlayAfterCurrent':
                insertPlayAfterCurrentQueue(type, uri, addToPlaylistClose);
                break;
            case 'replace':
                replaceQueue(type, uri, addToPlaylistClose);
                break;
            case 'replacePlay':
                replacePlayQueue(type, uri, addToPlaylistClose);
                break;
        }
    }
}

function addToPlaylistClose(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        uiElements.modalAddToPlaylist.hide();
    }
}

function appendPlaylist(type, uri, plist, callback) {
    switch(type) {
        case 'song':
        case 'stream':
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
            break;
    }
}

function insertPlaylist(type, uri, plist, to, callback) {
    switch(type) {
        case 'song':
        case 'stream':
        case 'dir':
            sendAPI("MYMPD_API_PLAYLIST_CONTENT_INSERT_URI", {
                "uri": uri,
                "plist": plist,
                "to": to
            }, callback, true);
            break;
        case 'search':
            sendAPI("MYMPD_API_PLAYLIST_CONTENT_INSERT_SEARCH", {
                "expression": uri,
                "plist": plist,
                "to": to
            }, callback, true);
            break;
    }
}

function replacePlaylist(type, uri, plist, callback) {
    switch(type) {
        case 'song':
        case 'stream':
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
            break;
    }
}

//eslint-disable-next-line no-unused-vars
function showRenamePlaylist(from) {
    cleanupModalId('modalRenamePlaylist');
    document.getElementById('renamePlaylistFrom').value = from;
    document.getElementById('renamePlaylistTo').value = '';
    uiElements.modalRenamePlaylist.show();
}

//eslint-disable-next-line no-unused-vars
function renamePlaylist() {
    cleanupModalId('modalRenamePlaylist');

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
    setUpdateViewId('BrowsePlaylistsDetailList');
    updateSmartPlaylist(getDataId('BrowsePlaylistsDetailList', 'uri'));
}

//eslint-disable-next-line no-unused-vars
function editSmartPlaylistClick() {
    showSmartPlaylist(getDataId('BrowsePlaylistsDetailList', 'uri'));
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
    const plist = getDataId('BrowsePlaylistsDetailList', 'uri');
    showConfirm(tn('Do you really want to clear the playlist?', {"playlist": plist}), tn('Yes, clear it'), function() {
        sendAPI("MYMPD_API_PLAYLIST_CONTENT_CLEAR", {
            "plist": plist
        });
        setUpdateViewId('BrowsePlaylistsDetailList');
    });
}

function playlistMoveSong(from, to) {
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

//eslint-disable-next-line no-unused-vars
function currentPlaylistToQueue(action) {
    const uri = getDataId('BrowsePlaylistsDetailList', 'uri');
    const type = getDataId('BrowsePlaylistsDetailList', 'type');
    switch(action) {
        case 'appendQueue':
            appendQueue(type, uri);
            break;
        case 'appendPlayQueue':
            appendPlayQueue(type, uri);
            break;
        case 'insertAfterCurrentQueue':
            insertAfterCurrentQueue(type, uri, 0, 1, false);
            break;
        case 'replaceQueue':
            replaceQueue(type, uri);
            break;
        case 'replacePlayQueue':
            replacePlayQueue(type, uri);
            break;
        case 'addToHome':
            addPlistToHome(uri, type, uri);
            break;
    }
}
