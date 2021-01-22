"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

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
    
   document.getElementById('BrowsePlaylistsAllList').addEventListener('click', function(event) {
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

function parsePlaylists(obj) {
    if (app.current.view === 'All') {
        document.getElementById('BrowsePlaylistsAllList').classList.remove('hide');
        document.getElementById('BrowsePlaylistsDetailList').classList.add('hide');
        document.getElementById('btnBrowsePlaylistsAll').parentNode.classList.add('hide');
        document.getElementById('playlistContentBtns').classList.add('hide');
        document.getElementById('smartPlaylistContentBtns').classList.add('hide');
        document.getElementById('btnAddSmartpls').parentNode.classList.remove('hide');
        document.getElementById('BrowseNavPlaylists').parentNode.classList.remove('hide');
    }
    else {
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
        document.getElementById('BrowsePlaylistsDetailList').classList.remove('hide');
        document.getElementById('BrowsePlaylistsAllList').classList.add('hide');
        document.getElementById('btnBrowsePlaylistsAll').parentNode.classList.remove('hide');
        document.getElementById('btnAddSmartpls').parentNode.classList.add('hide');
        document.getElementById('BrowseNavPlaylists').parentNode.classList.add('hide');
    }
            
    let nrItems = obj.result.returnedEntities;
    let table = document.getElementById(app.current.app + app.current.tab + app.current.view + 'List');
    let tbody = table.getElementsByTagName('tbody')[0];
    let tr = tbody.getElementsByTagName('tr');
    let navigate = document.activeElement.parentNode.parentNode === table ? true : false;
    let activeRow = 0;
    if (app.current.view === 'All') {
        for (let i = 0; i < nrItems; i++) {
            let row = document.createElement('tr');
            setAttEnc(row, 'data-uri', obj.result.data[i].uri);
            setAttEnc(row, 'data-type', obj.result.data[i].Type);
            setAttEnc(row, 'data-name', obj.result.data[i].name);
            row.setAttribute('tabindex', 0);
            row.innerHTML = '<td data-col="Type"><span class="mi">' + (obj.result.data[i].Type === 'smartpls' ? 'queue_music' : 'list') + '</span></td>' +
                            '<td>' + e(obj.result.data[i].name) + '</td>' +
                            '<td>'+ localeDate(obj.result.data[i].last_modified) + '</td>' +
                            '<td data-col="Action"><a href="#" class="mi color-darkgrey">' + ligatureMore + '</a></td>';
            if (i < tr.length) {
                activeRow = replaceTblRow(tr[i], row) === true ? i : activeRow;
            }
            else {
                tbody.append(row);
            }
        }
        //document.getElementById('cardFooterBrowse').innerText = gtPage('Num playlists', obj.result.returnedEntities, obj.result.totalEntities);
    }
    else if (app.current.view === 'Detail') {
        for (let i = 0; i < nrItems; i++) {
            let row = document.createElement('tr');
            if (obj.result.smartpls === false) {
                row.setAttribute('draggable','true');
            }
            row.setAttribute('id','playlistTrackId' + obj.result.data[i].Pos);
            setAttEnc(row, 'data-type', obj.result.data[i].Type);
            setAttEnc(row, 'data-uri', obj.result.data[i].uri);
            setAttEnc(row, 'data-name', obj.result.data[i].Title);
            setAttEnc(row, 'data-songpos', obj.result.data[i].Pos);
            row.setAttribute('tabindex', 0);
            obj.result.data[i].Duration = beautifySongDuration(obj.result.data[i].Duration);
            let tds = '';
            for (let c = 0; c < settings.colsBrowsePlaylistsDetail.length; c++) {
                tds += '<td data-col="' + settings.colsBrowsePlaylistsDetail[c] + '">' + e(obj.result.data[i][settings.colsBrowsePlaylistsDetail[c]]) + '</td>';
            }
            tds += '<td data-col="Action"><a href="#" class="mi color-darkgrey">' + ligatureMore + '</a></td>';
            row.innerHTML = tds;

            if (i < tr.length) {
                activeRow = replaceTblRow(tr[i], row) === true ? i : activeRow;
            }
            else {
                tbody.append(row);
            }
        }
        let tfoot = table.getElementsByTagName('tfoot')[0];
        let colspan = settings.colsBrowsePlaylistsDetail.length;
        colspan++;
        tfoot.innerHTML = '<tr><td colspan="' + (colspan + 1) + '"><small>' + t('Num songs', obj.result.totalEntities) + '&nbsp;&ndash;&nbsp;' + beautifyDuration(obj.result.totalTime) + '</small></td></tr>';
    }
    let trLen = tr.length - 1;
    for (let i = trLen; i >= nrItems; i --) {
        tr[i].remove();
    }

    if (navigate === true) {
        focusTable(0);
    }

    setPagination(obj.result.totalEntities, obj.result.returnedEntities);
    
    if (nrItems === 0) {
        if (app.current.view === 'All') {
            tbody.innerHTML = '<tr class="not-clickable"><td><span class="mi">error_outline</span></td>' +
                              '<td colspan="3">' + t('No playlists found') + '</td></tr>';
        }
        else {
            tbody.innerHTML = '<tr class="not-clickable"><td><span class="mi">error_outline</span></td>' +
                              '<td colspan="' + settings.colsBrowsePlaylistsDetail.length + '">' + t('Empty playlist') + '</td></tr>';
        }
    }
            
    document.getElementById(app.current.app + app.current.tab + app.current.view + 'List').classList.remove('opacity05');
}

//eslint-disable-next-line no-unused-vars
function playlistDetails(uri) {
    document.getElementById('BrowsePlaylistsAllList').classList.add('opacity05');
    appGoto('Browse', 'Playlists', 'Detail', '0', undefined, uri, '-', '-', '');
}

//eslint-disable-next-line no-unused-vars
function playlistClear() {
    let uri = getAttDec(document.getElementById('BrowsePlaylistsDetailList'), 'data-uri');
    sendAPI("MPD_API_PLAYLIST_CLEAR", {"uri": uri});
    document.getElementById('BrowsePlaylistsDetailList').classList.add('opacity05');    
}

//eslint-disable-next-line no-unused-vars
function playlistShuffle() {
    let uri = getAttDec(document.getElementById('BrowsePlaylistsDetailList'), 'data-uri');
    sendAPI("MPD_API_PLAYLIST_SHUFFLE", {"uri": uri});
    document.getElementById('BrowsePlaylistsDetailList').classList.add('opacity05');    
}

//eslint-disable-next-line no-unused-vars
function playlistSort(tag) {
    let uri = getAttDec(document.getElementById('BrowsePlaylistsDetailList'), 'data-uri');
    sendAPI("MPD_API_PLAYLIST_SORT", {"uri": uri, "tag": tag});
    document.getElementById('BrowsePlaylistsDetailList').classList.add('opacity05');    
}

function getAllPlaylists(obj, playlistSelect, playlistValue) {
    let nrItems = obj.result.returnedEntities;
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

    for (let i = 0; i < nrItems; i++) {
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
    sendAPI("MPD_API_PLAYLIST_RM_TRACK", {"uri": uri, "track": pos});
    document.getElementById('BrowsePlaylistsDetailList').classList.add('opacity05');    
}

//eslint-disable-next-line no-unused-vars
function toggleAddToPlaylistFrm() {
    let btn = document.getElementById('toggleAddToPlaylistBtn');
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
    let nameEl = document.getElementById('saveSmartPlaylistName');
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
    let elSelectSaveSmartPlaylistTag = document.getElementById('selectSaveSmartPlaylistTag');
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
        let timerange = obj.result.timerange / 24 / 60 / 60;
        document.getElementById('inputSaveSmartPlaylistNewestTimerange').value = timerange;
    }
    modalSaveSmartPlaylist.show();
    nameEl.focus();
}

//eslint-disable-next-line no-unused-vars
function saveSmartPlaylist() {
    let name = document.getElementById('saveSmartPlaylistName').value;
    let type = getAttDec(document.getElementById('saveSmartPlaylistType'), 'data-value');
    let sort = getSelectValue('saveSmartPlaylistSort');
    if (validatePlname(name) === true) {
        if (type === 'search') {
            let tag = getSelectValue('selectSaveSmartPlaylistTag');
            let searchstr = document.getElementById('inputSaveSmartPlaylistSearchstr').value;
            sendAPI("MPD_API_SMARTPLS_SAVE", {"type": type, "playlist": name, "tag": tag, "searchstr": searchstr, "sort": sort});
        }
        else if (type === 'sticker') {
            let sticker = getSelectValue('selectSaveSmartPlaylistSticker'); 
            let maxentriesEl = document.getElementById('inputSaveSmartPlaylistStickerMaxentries');
            if (!validateInt(maxentriesEl)) {
                return;
            }
            let minvalueEl = document.getElementById('inputSaveSmartPlaylistStickerMinvalue');
            if (!validateInt(minvalueEl)) {
                return;
            }
            sendAPI("MPD_API_SMARTPLS_SAVE", {"type": type, "playlist": name, "sticker": sticker, "maxentries": parseInt(maxentriesEl.value), 
                "minvalue": parseInt(minvalueEl.value), "sort": sort});
        }
        else if (type === 'newest') {
            let timerangeEl = document.getElementById('inputSaveSmartPlaylistNewestTimerange');
            if (!validateInt(timerangeEl)) {
                return;
            }
            let timerange = parseInt(timerangeEl.value) * 60 * 60 * 24;
            sendAPI("MPD_API_SMARTPLS_SAVE", {"type": type, "playlist": name, "timerange": timerange, "sort": sort});
        }
        else {
            document.getElementById('saveSmartPlaylistType').classList.add('is-invalid');
            return;
        }
        modalSaveSmartPlaylist.hide();
        showNotification(t('Saved smart playlist %{name}', {"name": name}), '', '', 'success');
    }
    else {
        document.getElementById('saveSmartPlaylistName').classList.add('is-invalid');
    }
}

//eslint-disable-next-line no-unused-vars
function addSmartpls(type) {
    let obj = {"jsonrpc":"2.0", "id":0, "result": {"method":"MPD_API_SMARTPLS_GET"}};
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
    sendAPI("MPD_API_PLAYLIST_RM_ALL", {"type": getSelectValue('selectDeletePlaylists')}, function() {
        btnWaiting(document.getElementById('btnDeletePlaylists'), false);
    });
}

//eslint-disable-next-line no-unused-vars
function showAddToPlaylistCurrentSong() {
    let uri = getAttDec(document.getElementById('currentTitle'), 'data-uri');
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
    let streamUrl = document.getElementById('streamUrl')
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
    modalAddToPlaylist.show();
    if (settings.featPlaylists) {
        sendAPI("MPD_API_PLAYLIST_LIST", {"searchstr": "", "offset": 0, "limit": 0}, function(obj) {
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
        let newPl = document.getElementById('addToPlaylistNewPlaylist').value;
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
            let expression = document.getElementById('addToPlaylistSearch').value;
            addAllFromSearchPlist(plist, expression, false);
        }
        else if (uri === 'DATABASE') {
            addAllFromBrowseDatabasePlist(plist);
        }
        else {
            sendAPI("MPD_API_PLAYLIST_ADD_TRACK", {"uri": uri, "plist": plist});
        }
        modalAddToPlaylist.hide();
    }
    else {
        document.getElementById('addToPlaylistPlaylist').classList.add('is-invalid');
    }
}

//eslint-disable-next-line no-unused-vars
function showRenamePlaylist(from) {
    removeIsInvalid(document.getElementById('modalRenamePlaylist'));
    modalRenamePlaylist.show();
    document.getElementById('renamePlaylistFrom').value = from;
    document.getElementById('renamePlaylistTo').value = '';
}

//eslint-disable-next-line no-unused-vars
function renamePlaylist() {
    let from = document.getElementById('renamePlaylistFrom').value;
    let to = document.getElementById('renamePlaylistTo').value;
    if (to !== from && validatePlname(to) === true) {
        sendAPI("MPD_API_PLAYLIST_RENAME", {"from": from, "to": to});
        modalRenamePlaylist.hide();
    }
    else {
        document.getElementById('renamePlaylistTo').classList.add('is-invalid');
    }
}

//eslint-disable-next-line no-unused-vars
function showSmartPlaylist(playlist) {
    sendAPI("MPD_API_SMARTPLS_GET", {"playlist": playlist}, parseSmartPlaylist);
}

//eslint-disable-next-line no-unused-vars
function updateSmartPlaylist(playlist) {
    sendAPI("MPDWORKER_API_SMARTPLS_UPDATE", {"playlist": playlist});
}

//eslint-disable-next-line no-unused-vars
function updateSmartPlaylistClick() {
    let uri = getAttDec(document.getElementById('BrowsePlaylistsDetailList'), 'data-uri');
    sendAPI("MPDWORKER_API_SMARTPLS_UPDATE", {"playlist": uri});
    document.getElementById('BrowsePlaylistsDetailList').classList.add('opacity05');    
}

//eslint-disable-next-line no-unused-vars
function showDelPlaylist(uri) {
    document.getElementById('deletePlaylist').value = uri;
    modalDeletePlaylist.show();
}

//eslint-disable-next-line no-unused-vars
function delPlaylist() {
    let uri = document.getElementById('deletePlaylist').value;
    sendAPI("MPD_API_PLAYLIST_RM", {"uri": uri});
    modalDeletePlaylist.hide();
}

function playlistMoveTrack(from, to) {
    sendAPI("MPD_API_PLAYLIST_MOVE_TRACK", {"plist": app.current.search, "from": from, "to": to});
}

//eslint-disable-next-line no-unused-vars
function addSelectedItemToPlaylist() {
    let item = document.activeElement;
    if (item) {
        if (item.parentNode.parentNode.id === 'BrowsePlaylistsAllList') {
            return;
        }
        showAddToPlaylist(getAttDec(item, 'data-uri'), '');
    }
}
