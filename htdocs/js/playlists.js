"use strict";
// SPDX-License-Identifier: GPL-2.0-or-later
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
    
    document.getElementById('addToPlaylistPlaylist').addEventListener('change', function () {
        if (getSelectValue(this) === 'new') {
            document.getElementById('addToPlaylistNewPlaylistDiv').classList.remove('hide');
            document.getElementById('addToPlaylistNewPlaylist').focus();
        }
        else {
            document.getElementById('addToPlaylistNewPlaylistDiv').classList.add('hide');
        }
    }, false);

    document.getElementById('searchPlaylistsDetailStr').addEventListener('keyup', function(event) {
        if (event.key === 'Escape') {
            this.blur();
        }
        else {
            appGoto(app.current.app, app.current.tab, app.current.view, 
                '0', app.current.limit, app.current.filter, app.current.sort, '-', this.value);
        }
    }, false);
    
    document.getElementById('searchPlaylistsListStr').addEventListener('keyup', function(event) {
        if (event.key === 'Escape') {
            this.blur();
        }
        else {
            appGoto(app.current.app, app.current.tab, app.current.view, 
                '0', app.current.limit, app.current.filter, app.current.sort, '-', this.value);
        }
    }, false);
    
   document.getElementById('BrowsePlaylistsListList').addEventListener('click', function(event) {
        if (event.target.nodeName === 'TD') {
            if (getCustomDomProperty(event.target.parentNode, 'data-smartpls-only') === false) {
                clickPlaylist(getCustomDomProperty(event.target.parentNode, 'data-uri'), getCustomDomProperty(event.target.parentNode, 'data-name'));
            }
            else {
                showNotification(tn('Playlist is empty'), '', 'playlist', 'warn')
            }
        }
        else if (event.target.nodeName === 'A') {
            showMenu(event.target, event);
        }
    }, false);

    document.getElementById('BrowsePlaylistsDetailList').addEventListener('click', function(event) {
        if (event.target.parentNode.parentNode.nodeName === 'TFOOT') {
            return;
        }
        if (event.target.nodeName === 'TD') {
            clickSong(getCustomDomProperty(event.target.parentNode, 'data-uri'), getCustomDomProperty(event.target.parentNode, 'data-name'));
        }
        else if (event.target.nodeName === 'A') {
            showMenu(event.target, event);
        }
    }, false);
}

function parsePlaylistsList(obj) {
    if (checkResult(obj, 'BrowsePlaylistsList', 3) === false) {
        return;
    }

    const rowTitle = webuiSettingsDefault.clickPlaylist.validValues[settings.webuiSettings.clickPlaylist];
    updateTable(obj, 'BrowsePlaylistsList', function(row, data) {
        setCustomDomProperty(row, 'data-uri', data.uri);
        setCustomDomProperty(row, 'data-type', data.Type);
        setCustomDomProperty(row, 'data-name', data.name);
        setCustomDomProperty(row, 'data-smartpls-only', data.smartplsOnly);
        row.setAttribute('title', t(rowTitle));
    }, function(row, data) {
        row.innerHTML = '<td data-col="Type"><span class="mi">' + (data.Type === 'smartpls' ? 'queue_music' : 'list') + '</span></td>' +
            '<td>' + e(data.name) + '</td>' +
            '<td>'+ localeDate(data.lastModified) + '</td>' +
            '<td data-col="Action"><a href="#" class="mi color-darkgrey">' + ligatureMore + '</a></td>';
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

    if (obj.result.plist.indexOf('.') > -1 || obj.result.smartpls === true) {
        setCustomDomProperty(document.getElementById('BrowsePlaylistsDetailList'), 'data-ro', 'true');
        document.getElementById('playlistContentBtns').classList.add('hide');
        document.getElementById('smartPlaylistContentBtns').classList.remove('hide');
    }
    else {
        setCustomDomProperty(document.getElementById('BrowsePlaylistsDetailList'), 'data-ro', 'false');
        document.getElementById('playlistContentBtns').classList.remove('hide');
        document.getElementById('smartPlaylistContentBtns').classList.add('hide');
    }
    setCustomDomProperty(document.getElementById('BrowsePlaylistsDetailList'), 'data-uri', obj.result.plist);
    document.getElementById('BrowsePlaylistsDetailList').getElementsByTagName('caption')[0].innerHTML = 
        (obj.result.smartpls === true ? t('Smart playlist') : t('Playlist'))  + ': ' + obj.result.plist;
    const rowTitle = webuiSettingsDefault.clickSong.validValues[settings.webuiSettings.clickSong];
    tfoot.innerHTML = '<tr><td colspan="' + colspan + '"><small>' + t('Num songs', obj.result.totalEntities) + '&nbsp;&ndash;&nbsp;' + beautifyDuration(obj.result.totalTime) + '</small></td></tr>';
    
    updateTable(obj, 'BrowsePlaylistsDetail', function(row, data) {
        row.setAttribute('id','playlistTrackId' + data.Pos);
        row.setAttribute('draggable', 'true');
        row.setAttribute('tabindex', 0);
        setCustomDomProperty(row, 'data-type', data.Type);
        setCustomDomProperty(row, 'data-uri', data.uri);
        setCustomDomProperty(row, 'data-name', data.Title);
        setCustomDomProperty(row, 'data-songpos', data.Pos);
        row.setAttribute('title', t(rowTitle));
    });
}

//eslint-disable-next-line no-unused-vars
function playlistDetails(uri) {
    document.getElementById('BrowsePlaylistsListList').classList.add('opacity05');
    appGoto('Browse', 'Playlists', 'Detail', '0', undefined, uri, '-', '-', '');
}

//eslint-disable-next-line no-unused-vars
function playlistShuffle() {
    sendAPI("MYMPD_API_PLAYLIST_SHUFFLE", {
        "plist": getCustomDomProperty('BrowsePlaylistsDetailList', 'data-uri')
    });
    document.getElementById('BrowsePlaylistsDetailList').classList.add('opacity05');    
}

//eslint-disable-next-line no-unused-vars
function playlistSort(tag) {
    sendAPI("MYMPD_API_PLAYLIST_SORT", {
        "plist": getCustomDomProperty('BrowsePlaylistsDetailList', 'data-uri'),
        "tag": tag
    });
    document.getElementById('BrowsePlaylistsDetailList').classList.add('opacity05');    
}

function getAllPlaylists(obj, playlistSelect, playlistValue) {
    let playlists = '';
    if (playlistSelect === 'addToPlaylistPlaylist') {
        playlists = '<option value=""></option><option value="new">' + t('New playlist') + '</option>';
    }
    else if (playlistSelect === 'selectJukeboxPlaylist' || 
             playlistSelect === 'selectAddToQueuePlaylist' ||
             playlistSelect === 'selectTimerPlaylist') 
    {
        playlists = '<option value="Database">' + t('Database') + '</option>';
    }

    for (let i = 0; i < obj.result.returnedEntities; i++) {
        if (playlistSelect === 'addToPlaylistPlaylist' && obj.result.data[i].Type === 'smartpls') {
            continue;
        }
        playlists += '<option value="' + e(obj.result.data[i].uri) + '"';
        if (playlistValue !== null && obj.result.data[i].uri === playlistValue) {
            playlists += ' selected';
        }
        playlists += '>' + e(obj.result.data[i].uri) + '</option>';
    }
    
    document.getElementById(playlistSelect).innerHTML = playlists;
}

//eslint-disable-next-line no-unused-vars
function updateSmartPlaylists(force) {
    sendAPI("MPDWORKER_API_SMARTPLS_UPDATE_ALL", {
        "force":force
    });
}

//eslint-disable-next-line no-unused-vars
function removeFromPlaylist(plist, pos) {
    pos--;
    sendAPI("MYMPD_API_PLAYLIST_RM_SONG", {
        "plist": plist,
        "pos": pos
    });
    document.getElementById('BrowsePlaylistsDetailList').classList.add('opacity05');    
}

//eslint-disable-next-line no-unused-vars
function toggleAddToPlaylistFrm() {
    const btn = document.getElementById('toggleAddToPlaylistBtn');
    toggleBtn('toggleAddToPlaylistBtn');
    if (btn.classList.contains('active')) {
        document.getElementById('addToPlaylistFrm').classList.remove('hide');
        document.getElementById('addStreamFooter').classList.add('hide');
        document.getElementById('addToPlaylistFooter').classList.remove('hide');
    }    
    else {
        document.getElementById('addToPlaylistFrm').classList.add('hide');
        document.getElementById('addStreamFooter').classList.remove('hide');
        document.getElementById('addToPlaylistFooter').classList.add('hide');
    }
}

function parseSmartPlaylist(obj) {
    const nameEl = document.getElementById('saveSmartPlaylistName');
    nameEl.value = obj.result.plist;
    removeIsInvalid(document.getElementById('modalSaveSmartPlaylist'));
    document.getElementById('saveSmartPlaylistType').value = t(obj.result.type);
    setCustomDomProperty(document.getElementById('saveSmartPlaylistType'), 'data-value', obj.result.type);
    document.getElementById('saveSmartPlaylistSearch').classList.add('hide');
    document.getElementById('saveSmartPlaylistSticker').classList.add('hide');
    document.getElementById('saveSmartPlaylistNewest').classList.add('hide');
    
    if (obj.result.type === 'search') {
        document.getElementById('saveSmartPlaylistSearch').classList.remove('hide');
        document.getElementById('inputSaveSmartPlaylistExpression').value = obj.result.expression;
    }
    else if (obj.result.type === 'sticker') {
        document.getElementById('saveSmartPlaylistSticker').classList.remove('hide');
        document.getElementById('selectSaveSmartPlaylistSticker').value = obj.result.sticker;
        document.getElementById('inputSaveSmartPlaylistStickerMaxentries').value = obj.result.maxentries;
        document.getElementById('inputSaveSmartPlaylistStickerMinvalue').value = obj.result.minvalue;
    }
    else if (obj.result.type === 'newest') {
        document.getElementById('saveSmartPlaylistNewest').classList.remove('hide');
        document.getElementById('inputSaveSmartPlaylistNewestTimerange').value = obj.result.timerange / 24 / 60 / 60;
    }
    hideModalAlert();
    uiElements.modalSaveSmartPlaylist.show();
    nameEl.focus();
}

//eslint-disable-next-line no-unused-vars
function saveSmartPlaylist() {
    const name = document.getElementById('saveSmartPlaylistName').value;
    const type = getCustomDomProperty('saveSmartPlaylistType', 'data-value');
    const sort = getSelectValue('saveSmartPlaylistSort');
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
            "sticker": getSelectValue('selectSaveSmartPlaylistSticker'),
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
        "type": getSelectValue('selectDeletePlaylists')
    }, function() {
        btnWaiting(document.getElementById('btnDeletePlaylists'), false);
    });
}

//eslint-disable-next-line no-unused-vars
function showAddToPlaylistCurrentSong() {
    const uri = getCustomDomProperty('currentTitle', 'data-uri');
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
    elClear(document.getElementById('addToPlaylistPlaylist'));
    document.getElementById('addToPlaylistNewPlaylist').value = '';
    document.getElementById('addToPlaylistNewPlaylistDiv').classList.add('hide');
    toggleBtn('toggleAddToPlaylistBtn', 0);
    const streamUrl = document.getElementById('streamUrl');
    streamUrl.focus();
    streamUrl.value = '';
    removeIsInvalid(document.getElementById('modalAddToPlaylist'));
    if (uri !== 'stream') {
        document.getElementById('addStreamFooter').classList.add('hide');
        document.getElementById('addStreamFrm').classList.add('hide');
        document.getElementById('addToPlaylistFooter').classList.remove('hide');
        document.getElementById('addToPlaylistFrm').classList.remove('hide');
        document.getElementById('addToPlaylistCaption').textContent = t('Add to playlist');
    }
    else {
        document.getElementById('addStreamFooter').classList.remove('hide');
        document.getElementById('addStreamFrm').classList.remove('hide');
        document.getElementById('addToPlaylistFooter').classList.add('hide');
        document.getElementById('addToPlaylistFrm').classList.add('hide');
        document.getElementById('addToPlaylistCaption').textContent = t('Add stream');
    }
    uiElements.modalAddToPlaylist.show();
    if (features.featPlaylists) {
        sendAPI("MYMPD_API_PLAYLIST_LIST", {"searchstr": "", "offset": 0, "limit": 0}, function(obj) {
            getAllPlaylists(obj, 'addToPlaylistPlaylist');
        });
    }
}

//eslint-disable-next-line no-unused-vars
function addToPlaylist() {
    let uri = decodeURI(document.getElementById('addToPlaylistUri').value);
    if (uri === 'stream') {
        uri = document.getElementById('streamUrl').value;
        if (uri === '' || uri.indexOf('http') === -1) {
            document.getElementById('streamUrl').classList.add('is-invalid');
            return;
        }
    }
    let plist = getSelectValue('addToPlaylistPlaylist');
    if (plist === 'new') {
        const newPl = document.getElementById('addToPlaylistNewPlaylist').value;
        if (validatePlname(newPl) === true) {
            plist = newPl;
        }
        else {
            document.getElementById('addToPlaylistNewPlaylist').classList.add('is-invalid');
            return;
        }
    }
    else if (plist === '') {
        document.getElementById('addToPlaylistPlaylist').classList.add('is-invalid');
        return;
    }

    if (uri === 'SEARCH') {
        addAllFromSearchPlist(plist, null, false, addToPlaylistClose);
    }
    else if (uri === 'ALBUM') {
        const expression = document.getElementById('addToPlaylistSearch').value;
        addAllFromSearchPlist(plist, expression, false, addToPlaylistClose);
    }
    else if (uri === 'DATABASE') {
        addAllFromBrowseDatabasePlist(plist, addToPlaylistClose);
    }
    else {
        sendAPI("MYMPD_API_PLAYLIST_ADD_URI", {
            "uri": uri,
            "plist": plist
        }, addToPlaylistClose, true);
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
        "plist": getCustomDomProperty('BrowsePlaylistsDetailList', 'data-uri')
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
    const plist = getCustomDomProperty('BrowsePlaylistsDetailList', 'data-uri');
    showConfirm(tn('Do you really want to clear the playlist?', {"playlist": plist}), tn('Yes, clear it'), function() {
        sendAPI("MYMPD_API_PLAYLIST_CLEAR", {
            "plist": plist
        });
        document.getElementById('BrowsePlaylistsDetailList').classList.add('opacity05');
    });
}

function playlistMoveTrack(from, to) {
    sendAPI("MYMPD_API_PLAYLIST_MOVE_SONG", {
        "plist": app.current.filter,
        "from": from,
        "to": to
    });
}

//eslint-disable-next-line no-unused-vars
function addSelectedItemToPlaylist() {
    const item = document.activeElement;
    if (item) {
        if (item.parentNode.parentNode.id === 'BrowsePlaylistsListList') {
            return;
        }
        showAddToPlaylist(getCustomDomProperty(item, 'data-uri'), '');
    }
}
