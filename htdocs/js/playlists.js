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
            clickPlaylist(getAttDec(event.target.parentNode, 'data-uri'), getAttDec(event.target.parentNode, 'data-name'));
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
            clickSong(getAttDec(event.target.parentNode, 'data-uri'), getAttDec(event.target.parentNode, 'data-name'));
        }
        else if (event.target.nodeName === 'A') {
            showMenu(event.target, event);
        }
    }, false);
}

function parsePlaylistsList(obj) {
    const rowTitle = advancedSettingsDefault.clickPlaylist.validValues[settings.advanced.clickPlaylist];
    updateTable(obj, app.current.app + app.current.tab + app.current.view, function(row, data) {
        setAttEnc(row, 'data-uri', data.uri);
        setAttEnc(row, 'data-type', data.Type);
        setAttEnc(row, 'data-name', data.name);
        row.setAttribute('title', t(rowTitle));
    }, function(row, data) {
        row.innerHTML = '<td data-col="Type"><span class="mi">' + (data.Type === 'smartpls' ? 'queue_music' : 'list') + '</span></td>' +
            '<td>' + e(data.name) + '</td>' +
            '<td>'+ localeDate(data.last_modified) + '</td>' +
            '<td data-col="Action"><a href="#" class="mi color-darkgrey">' + ligatureMore + '</a></td>';
    });
}

function parsePlaylistsDetail(obj) {
    if (obj.result.uri.indexOf('.') > -1 || obj.result.smartpls === true) {
        setAttEnc(document.getElementById('BrowsePlaylistsDetailList'), 'data-ro', 'true');
        document.getElementById('playlistContentBtns').classList.add('hide');
        document.getElementById('smartPlaylistContentBtns').classList.remove('hide');
    }
    else {
        setAttEnc(document.getElementById('BrowsePlaylistsDetailList'), 'data-ro', 'false');
        document.getElementById('playlistContentBtns').classList.remove('hide');
        document.getElementById('smartPlaylistContentBtns').classList.add('hide');
    }
    setAttEnc(document.getElementById('BrowsePlaylistsDetailList'), 'data-uri', obj.result.uri);
    document.getElementById('BrowsePlaylistsDetailList').getElementsByTagName('caption')[0].innerHTML = 
        (obj.result.smartpls === true ? t('Smart playlist') : t('Playlist'))  + ': ' + obj.result.uri;
    const rowTitle = advancedSettingsDefault.clickSong.validValues[settings.advanced.clickSong];
    const table = document.getElementById('BrowsePlaylistsDetailList');
    const tfoot = table.getElementsByTagName('tfoot')[0];
    const colspan = settings.colsBrowsePlaylistsDetail.length;
    tfoot.innerHTML = '<tr><td colspan="' + (colspan + 1) + '"><small>' + t('Num songs', obj.result.totalEntities) + '&nbsp;&ndash;&nbsp;' + beautifyDuration(obj.result.totalTime) + '</small></td></tr>';
    updateTable(obj, app.current.app + app.current.tab + app.current.view, function(row, data) {
        row.setAttribute('id','playlistTrackId' + data.Pos);
        setAttEnc(row, 'data-type', data.Type);
        setAttEnc(row, 'data-uri', data.uri);
        setAttEnc(row, 'data-name', data.Title);
        setAttEnc(row, 'data-songpos', data.Pos);
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
        "uri": getAttDec(document.getElementById('BrowsePlaylistsDetailList'), 'data-uri')
    });
    document.getElementById('BrowsePlaylistsDetailList').classList.add('opacity05');    
}

//eslint-disable-next-line no-unused-vars
function playlistSort(tag) {
    sendAPI("MYMPD_API_PLAYLIST_SORT", {
        "uri": getAttDec(document.getElementById('BrowsePlaylistsDetailList'), 'data-uri'),
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
    sendAPI("MPDWORKER_API_SMARTPLS_UPDATE_ALL", {"force":force});
}

//eslint-disable-next-line no-unused-vars
function removeFromPlaylist(uri, pos) {
    pos--;
    sendAPI("MYMPD_API_PLAYLIST_RM_TRACK", {"uri": uri, "track": pos});
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
    nameEl.value = obj.result.playlist;
    removeIsInvalid(document.getElementById('modalSaveSmartPlaylist'));
    document.getElementById('saveSmartPlaylistType').value = t(obj.result.type);
    setAttEnc(document.getElementById('saveSmartPlaylistType'), 'data-value', obj.result.type);
    document.getElementById('saveSmartPlaylistSearch').classList.add('hide');
    document.getElementById('saveSmartPlaylistSticker').classList.add('hide');
    document.getElementById('saveSmartPlaylistNewest').classList.add('hide');
    let tagList;
    if (settings.featTags) {
        tagList = '<option value="any">' + t('Any Tag') + '</option>';
    }
    tagList += '<option value="filename">' + t('Filename') + '</option>';
    for (let i = 0; i < settings.searchtags.length; i++) {
        tagList += '<option value="' + settings.searchtags[i] + '">' + t(settings.searchtags[i]) + '</option>';
    }
    const elSelectSaveSmartPlaylistTag = document.getElementById('selectSaveSmartPlaylistTag');
    elSelectSaveSmartPlaylistTag.innerHTML = tagList;
    if (obj.result.type === 'search') {
        document.getElementById('saveSmartPlaylistSearch').classList.remove('hide');
        document.getElementById('selectSaveSmartPlaylistTag').value = obj.result.tag;
        document.getElementById('inputSaveSmartPlaylistSearchstr').value = obj.result.searchstr;
        if (settings.featAdvsearch === true && obj.result.tag === 'expression') {
            elSelectSaveSmartPlaylistTag.parentNode.parentNode.classList.add('hide');
            elSelectSaveSmartPlaylistTag.innerHTML = '<option value="expression">expression</option>';
            elSelectSaveSmartPlaylistTag.value = 'expression';
        }
        else {
            document.getElementById('selectSaveSmartPlaylistTag').parentNode.parentNode.classList.remove('hide');
        }
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
    uiElements.modalSaveSmartPlaylist.show();
    nameEl.focus();
}

//eslint-disable-next-line no-unused-vars
function saveSmartPlaylist() {
    const name = document.getElementById('saveSmartPlaylistName').value;
    const type = getAttDec(document.getElementById('saveSmartPlaylistType'), 'data-value');
    const sort = getSelectValue('saveSmartPlaylistSort');
    if (validatePlname(name) === true) {
        if (type === 'search') {
            sendAPI("MYMPD_API_SMARTPLS_SAVE", {
                "type": type, 
                "playlist": name, 
                "tag": getSelectValue('selectSaveSmartPlaylistTag'), 
                "searchstr": document.getElementById('inputSaveSmartPlaylistSearchstr').value,
                "sort": sort
            });
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
            sendAPI("MYMPD_API_SMARTPLS_SAVE", {
                "type": type, 
                "playlist": name,
                "sticker": getSelectValue('selectSaveSmartPlaylistSticker'),
                "maxentries": parseInt(maxentriesEl.value), 
                "minvalue": parseInt(minvalueEl.value),
                "sort": sort
            });
        }
        else if (type === 'newest') {
            const timerangeEl = document.getElementById('inputSaveSmartPlaylistNewestTimerange');
            if (!validateInt(timerangeEl)) {
                return;
            }
            sendAPI("MYMPD_API_SMARTPLS_SAVE", {
                "type": type,
                "playlist": name,
                "timerange": parseInt(timerangeEl.value) * 60 * 60 * 24,
                "sort": sort
            });
        }
        else {
            document.getElementById('saveSmartPlaylistType').classList.add('is-invalid');
            return;
        }
        uiElements.modalSaveSmartPlaylist.hide();
        showNotification(t('Saved smart playlist %{name}', {"name": name}), '', 'playlist', 'info');
    }
    else {
        document.getElementById('saveSmartPlaylistName').classList.add('is-invalid');
    }
}

//eslint-disable-next-line no-unused-vars
function addSmartpls(type) {
    const obj = {"jsonrpc": "2.0", "id": 0, "result": {"method": "MYMPD_API_SMARTPLS_GET"}};
    if (type === 'mostPlayed') {
        obj.result.playlist = settings.smartplsPrefix + (settings.smartplsPrefix !== '' ? '-' : '') + 'mostPlayed';
        obj.result.type = 'sticker';
        obj.result.sticker = 'playCount';
        obj.result.maxentries = 200;
        obj.result.minvalue = 10;
    }
    else if (type === 'newest') {
        obj.result.playlist = settings.smartplsPrefix + (settings.smartplsPrefix !== '' ? '-' : '') + 'newestSongs';
        obj.result.type = 'newest';
        obj.result.timerange = 14 * 24 * 60 * 60;
    }
    else if (type === 'bestRated') {
        obj.result.playlist = settings.smartplsPrefix + (settings.smartplsPrefix !== '' ? '-' : '') + 'bestRated';
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
    sendAPI("MYMPD_API_PLAYLIST_RM_ALL", {"type": getSelectValue('selectDeletePlaylists')}, function() {
        btnWaiting(document.getElementById('btnDeletePlaylists'), false);
    });
}

//eslint-disable-next-line no-unused-vars
function showAddToPlaylistCurrentSong() {
    const uri = getAttDec(document.getElementById('currentTitle'), 'data-uri');
    if (uri !== '') {
        showAddToPlaylist(uri, '');
    }
}

//eslint-disable-next-line no-unused-vars
function showAddToPlaylistCurrentSearch() {
    showAddToPlaylist(app.current.search, '');
}

function showAddToPlaylist(uri, searchstr) {
    document.getElementById('addToPlaylistUri').value = uri;
    document.getElementById('addToPlaylistSearch').value = searchstr;
    document.getElementById('addToPlaylistPlaylist').innerHTML = '';
    document.getElementById('addToPlaylistNewPlaylist').value = '';
    document.getElementById('addToPlaylistNewPlaylistDiv').classList.add('hide');
    toggleBtn('toggleAddToPlaylistBtn',0);
    const streamUrl = document.getElementById('streamUrl');
    streamUrl.focus();
    streamUrl.value = '';
    removeIsInvalid(document.getElementById('modalAddToPlaylist'));
    if (uri !== 'stream') {
        document.getElementById('addStreamFooter').classList.add('hide');
        document.getElementById('addStreamFrm').classList.add('hide');
        document.getElementById('addToPlaylistFooter').classList.remove('hide');
        document.getElementById('addToPlaylistFrm').classList.remove('hide');
        document.getElementById('addToPlaylistCaption').innerText = t('Add to playlist');
    }
    else {
        document.getElementById('addStreamFooter').classList.remove('hide');
        document.getElementById('addStreamFrm').classList.remove('hide');
        document.getElementById('addToPlaylistFooter').classList.add('hide');
        document.getElementById('addToPlaylistFrm').classList.add('hide');
        document.getElementById('addToPlaylistCaption').innerText = t('Add stream');
    }
    uiElements.modalAddToPlaylist.show();
    if (settings.featPlaylists) {
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
    if (plist !== '') {
        if (uri === 'SEARCH') {
            addAllFromSearchPlist(plist, null, false);
        }
        else if (uri === 'ALBUM') {
            const expression = document.getElementById('addToPlaylistSearch').value;
            addAllFromSearchPlist(plist, expression, false);
        }
        else if (uri === 'DATABASE') {
            addAllFromBrowseDatabasePlist(plist);
        }
        else {
            sendAPI("MYMPD_API_PLAYLIST_ADD_TRACK", {"uri": uri, "plist": plist});
        }
        uiElements.modalAddToPlaylist.hide();
    }
    else {
        document.getElementById('addToPlaylistPlaylist').classList.add('is-invalid');
    }
}

//eslint-disable-next-line no-unused-vars
function showRenamePlaylist(from) {
    removeIsInvalid(document.getElementById('modalRenamePlaylist'));
    uiElements.modalRenamePlaylist.show();
    document.getElementById('renamePlaylistFrom').value = from;
    document.getElementById('renamePlaylistTo').value = '';
}

//eslint-disable-next-line no-unused-vars
function renamePlaylist() {
    const from = document.getElementById('renamePlaylistFrom').value;
    const to = document.getElementById('renamePlaylistTo').value;
    if (to !== from && validatePlname(to) === true) {
        sendAPI("MYMPD_API_PLAYLIST_RENAME", {"from": from, "to": to});
        uiElements.modalRenamePlaylist.hide();
    }
    else {
        document.getElementById('renamePlaylistTo').classList.add('is-invalid');
    }
}

//eslint-disable-next-line no-unused-vars
function showSmartPlaylist(playlist) {
    sendAPI("MYMPD_API_SMARTPLS_GET", {"playlist": playlist}, parseSmartPlaylist);
}

//eslint-disable-next-line no-unused-vars
function updateSmartPlaylist(playlist) {
    sendAPI("MPDWORKER_API_SMARTPLS_UPDATE", {"playlist": playlist});
}

//eslint-disable-next-line no-unused-vars
function updateSmartPlaylistClick() {
    sendAPI("MPDWORKER_API_SMARTPLS_UPDATE", {
        "playlist": getAttDec(document.getElementById('BrowsePlaylistsDetailList'), 'data-uri')
    });
    document.getElementById('BrowsePlaylistsDetailList').classList.add('opacity05');    
}

//eslint-disable-next-line no-unused-vars
function showDelPlaylist(uri) {
    showConfirm(t('Do you really want to delete the playlist?', {"playlist": uri}), "Yes, delete it", function() {
        sendAPI("MYMPD_API_PLAYLIST_RM", {"uri": uri});
    });
}

//eslint-disable-next-line no-unused-vars
function showClearPlaylist() {
    const uri = getAttDec(document.getElementById('BrowsePlaylistsDetailList'), 'data-uri');
    showConfirm(t('Do you really want to clear the playlist?', {"playlist": uri}), "Yes, clear it", function() {
        sendAPI("MYMPD_API_PLAYLIST_CLEAR", {
            "uri": uri
        });
        document.getElementById('BrowsePlaylistsDetailList').classList.add('opacity05');
    });
}

function playlistMoveTrack(from, to) {
    sendAPI("MYMPD_API_PLAYLIST_MOVE_TRACK", {
        "plist": app.current.search,
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
        showAddToPlaylist(getAttDec(item, 'data-uri'), '');
    }
}
