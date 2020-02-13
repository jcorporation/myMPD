"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

function parsePlaylists(obj) {
    if (app.current.view === 'All') {
        document.getElementById('BrowsePlaylistsAllList').classList.remove('hide');
        document.getElementById('BrowsePlaylistsDetailList').classList.add('hide');
        document.getElementById('btnBrowsePlaylistsAll').parentNode.classList.add('hide');
        document.getElementById('playlistContentBtns').classList.add('hide');
        document.getElementById('smartPlaylistContentBtns').classList.add('hide');
        document.getElementById('btnAddSmartpls').parentNode.classList.remove('hide');
    } else {
        if (obj.result.uri.indexOf('.') > -1 || obj.result.smartpls === true) {
            document.getElementById('BrowsePlaylistsDetailList').setAttribute('data-ro', 'true')
            document.getElementById('playlistContentBtns').classList.add('hide');
            document.getElementById('smartPlaylistContentBtns').classList.remove('hide');
        }
        else {
            document.getElementById('BrowsePlaylistsDetailList').setAttribute('data-ro', 'false');
            document.getElementById('playlistContentBtns').classList.remove('hide');
            document.getElementById('smartPlaylistContentBtns').classList.add('hide');
        }
        document.getElementById('BrowsePlaylistsDetailList').setAttribute('data-uri', obj.result.uri);
        document.getElementById('BrowsePlaylistsDetailList').getElementsByTagName('caption')[0].innerHTML = 
            (obj.result.smartpls === true ? t('Smart playlist') : t('Playlist'))  + ': ' + obj.result.uri;
        document.getElementById('BrowsePlaylistsDetailList').classList.remove('hide');
        document.getElementById('BrowsePlaylistsAllList').classList.add('hide');
        document.getElementById('btnBrowsePlaylistsAll').parentNode.classList.remove('hide');
        document.getElementById('btnAddSmartpls').parentNode.classList.add('hide');
    }
            
    let nrItems = obj.result.returnedEntities;
    let table = document.getElementById(app.current.app + app.current.tab + app.current.view + 'List');
    let tbody = table.getElementsByTagName('tbody')[0];
    let tr = tbody.getElementsByTagName('tr');
    let navigate = document.activeElement.parentNode.parentNode === table ? true : false;
    let activeRow = 0;
    if (app.current.view === 'All') {
        for (let i = 0; i < nrItems; i++) {
            let uri = encodeURI(obj.result.data[i].uri);
            let row = document.createElement('tr');
            row.setAttribute('data-uri', uri);
            row.setAttribute('data-type', obj.result.data[i].Type);
            row.setAttribute('data-name', obj.result.data[i].name);
            row.setAttribute('tabindex', 0);
            row.innerHTML = '<td data-col="Type"><span class="material-icons">' + (obj.result.data[i].Type === 'smartpls' ? 'queue_music' : 'list') + '</span></td>' +
                            '<td>' + e(obj.result.data[i].name) + '</td>' +
                            '<td>'+ localeDate(obj.result.data[i].last_modified) + '</td>' +
                            '<td data-col="Action"><a href="#" class="material-icons color-darkgrey">' + ligatureMore + '</a></td>';
            if (i < tr.length) {
                activeRow = replaceTblRow(tr[i], row) === true ? i : activeRow;
            }
            else {
                tbody.append(row);
            }
        }
        document.getElementById('cardFooterBrowse').innerText = gtPage('Num playlists', obj.result.returnedEntities, obj.result.totalEntities);
    }
    else if (app.current.view === 'Detail') {
        for (let i = 0; i < nrItems; i++) {
            let uri = encodeURI(obj.result.data[i].uri);
            let row = document.createElement('tr');
            if (obj.result.smartpls === false) {
                row.setAttribute('draggable','true');
            }
            row.setAttribute('id','playlistTrackId' + obj.result.data[i].Pos);
            row.setAttribute('data-type', obj.result.data[i].Type);
            row.setAttribute('data-uri', uri);
            row.setAttribute('data-name', obj.result.data[i].Title);
            row.setAttribute('data-songpos', obj.result.data[i].Pos);
            row.setAttribute('tabindex', 0);
            obj.result.data[i].Duration = beautifySongDuration(obj.result.data[i].Duration);
            let tds = '';
            for (let c = 0; c < settings.colsBrowsePlaylistsDetail.length; c++) {
                tds += '<td data-col="' + settings.colsBrowsePlaylistsDetail[c] + '">' + e(obj.result.data[i][settings.colsBrowsePlaylistsDetail[c]]) + '</td>';
            }
            tds += '<td data-col="Action"><a href="#" class="material-icons color-darkgrey">' + ligatureMore + '</a></td>';
            row.innerHTML = tds;

            if (i < tr.length) {
                activeRow = replaceTblRow(tr[i], row) === true ? i : activeRow;
            }
            else {
                tbody.append(row);
            }
        }
        document.getElementById('cardFooterBrowse').innerText = gtPage('Num songs', obj.result.returnedEntities, obj.result.totalEntities);
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
            tbody.innerHTML = '<tr><td><span class="material-icons">error_outline</span></td>' +
                              '<td colspan="3">' + t('No playlists found') + '</td></tr>';
        }
        else {
            tbody.innerHTML = '<tr><td><span class="material-icons">error_outline</span></td>' +
                              '<td colspan="' + (settings.colsBrowsePlaylistsDetail.length - 1) + '">' + t('Empty playlist') + '</td></tr>';
        }
    }
            
    document.getElementById(app.current.app + app.current.tab + app.current.view + 'List').classList.remove('opacity05');
}

//eslint-disable-next-line no-unused-vars
function playlistDetails(uri) {
    document.getElementById('BrowsePlaylistsAllList').classList.add('opacity05');
    appGoto('Browse', 'Playlists', 'Detail', '0/-/-/' + uri);
}

//eslint-disable-next-line no-unused-vars
function playlistClear() {
    let uri = document.getElementById('BrowsePlaylistsDetailList').getAttribute('data-uri');
    sendAPI("MPD_API_PLAYLIST_CLEAR", {"uri": uri});
    document.getElementById('BrowsePlaylistsDetailList').classList.add('opacity05');    
}

//eslint-disable-next-line no-unused-vars
function playlistShuffle() {
    let uri = document.getElementById('BrowsePlaylistsDetailList').getAttribute('data-uri');
    sendAPI("MPD_API_PLAYLIST_SHUFFLE", {"uri": uri});
    document.getElementById('BrowsePlaylistsDetailList').classList.add('opacity05');    
}

//eslint-disable-next-line no-unused-vars
function playlistSort(tag) {
    let uri = document.getElementById('BrowsePlaylistsDetailList').getAttribute('data-uri');
    sendAPI("MPD_API_PLAYLIST_SORT", {"uri": uri, "tag": tag});
    document.getElementById('BrowsePlaylistsDetailList').classList.add('opacity05');    
}

function getAllPlaylists(obj) {
    let nrItems = obj.result.returnedEntities;
    let playlists = '';
    if (obj.result.offset === 0) {
        if (playlistEl === 'addToPlaylistPlaylist') {
            playlists = '<option value=""></option><option value="new">' + t('New playlist') + '</option>';
        }
        else if (playlistEl === 'selectJukeboxPlaylist' || 
                 playlistEl === 'selectAddToQueuePlaylist' ||
                 playlistEl === 'selectTimerPlaylist'
        ) {
            playlists = '<option value="Database">' + t('Database') + '</option>';
        }
    }
    for (let i = 0; i < nrItems; i++) {
        if (playlistEl === 'addToPlaylistPlaylist' && obj.result.data[i].Type === 'smartpls') {
            continue;
        }
        playlists += '<option value="' + e(obj.result.data[i].uri) + '"';
        if (playlistEl === 'selectJukeboxPlaylist' && obj.result.data[i].uri === settings.jukeboxPlaylist) {
            playlists += ' selected';
        }
        playlists += '>' + e(obj.result.data[i].uri) + '</option>';
    }
    if (obj.result.offset === 0) {
        document.getElementById(playlistEl).innerHTML = playlists;
    }
    else {
        document.getElementById(playlistEl).innerHTML += playlists;
    }
    if (obj.result.totalEntities > obj.result.returnedEntities) {
        obj.result.offset += settings.maxElementsPerPage;
        sendAPI("MPD_API_PLAYLIST_LIST", {"offset": obj.result.offset, "filter": "-"}, getAllPlaylists);
    }
}

//eslint-disable-next-line no-unused-vars
function updateSmartPlaylists() {
    sendAPI("MPD_API_SMARTPLS_UPDATE_ALL", {});
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
    nameEl.classList.remove('is-invalid');
    document.getElementById('saveSmartPlaylistType').value = t(obj.result.type);
    document.getElementById('saveSmartPlaylistType').setAttribute('data-value', obj.result.type);
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
        if (settings.featAdvsearch && obj.result.tag === 'expression') {
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
    let type = document.getElementById('saveSmartPlaylistType').getAttribute('data-value');
    let sortEl = document.getElementById('saveSmartPlaylistSort');
    let sort = sortEl.options[sortEl.selectedIndex].value;
    if (validatePlname(name) === true) {
        if (type === 'search') {
            let tagEl = document.getElementById('selectSaveSmartPlaylistTag');
            let tag = tagEl.options[tagEl.selectedIndex].value;
            let searchstr = document.getElementById('inputSaveSmartPlaylistSearchstr').value;
            sendAPI("MPD_API_SMARTPLS_SAVE", {"type": type, "playlist": name, "tag": tag, "searchstr": searchstr, "sort": sort});
        }
        else if (type === 'sticker') {
            let stickerEl = document.getElementById('selectSaveSmartPlaylistSticker');
            let sticker = stickerEl.options[stickerEl.selectedIndex].value;
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
    let selectDeletePlaylists = document.getElementById('selectDeletePlaylists');
    let btnDeletePlaylists = document.getElementById('btnDeletePlaylists');
    btnWaiting(btnDeletePlaylists, true);
    sendAPI("MPD_API_PLAYLIST_RM_ALL", {"type": selectDeletePlaylists.options[selectDeletePlaylists.selectedIndex].value}, function() {
        let btnDeletePlaylists = document.getElementById('btnDeletePlaylists');
        btnWaiting(btnDeletePlaylists, false);
    });
}

//eslint-disable-next-line no-unused-vars
function showAddToPlaylistCurrentSong() {
    let uri = document.getElementById('currentTitle').getAttribute('data-uri');
    if (uri !== '') {
        showAddToPlaylist(uri, '');
    }
}

function showAddToPlaylist(uri, searchstr) {
    document.getElementById('addToPlaylistUri').value = uri;
    document.getElementById('addToPlaylistSearch').value = searchstr;
    document.getElementById('addToPlaylistPlaylist').innerHTML = '';
    document.getElementById('addToPlaylistNewPlaylist').value = '';
    document.getElementById('addToPlaylistNewPlaylistDiv').classList.add('hide');
    document.getElementById('addToPlaylistNewPlaylist').classList.remove('is-invalid');
    toggleBtn('toggleAddToPlaylistBtn',0);
    let streamUrl = document.getElementById('streamUrl')
    streamUrl.focus();
    streamUrl.value = '';
    streamUrl.classList.remove('is-invalid');
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
        playlistEl = 'addToPlaylistPlaylist';
        sendAPI("MPD_API_PLAYLIST_LIST", {"offset": 0, "filter": "-"}, getAllPlaylists);
    }
}

//eslint-disable-next-line no-unused-vars
function addToPlaylist() {
    let uri = document.getElementById('addToPlaylistUri').value;
    if (uri === 'stream') {
        uri = document.getElementById('streamUrl').value;
        if (uri === '' || uri.indexOf('http') === -1) {
            document.getElementById('streamUrl').classList.add('is-invalid');
            return;
        }
    }
    let plistEl = document.getElementById('addToPlaylistPlaylist');
    let plist = plistEl.options[plistEl.selectedIndex].value;
    if (plist === 'new') {
        let newPl = document.getElementById('addToPlaylistNewPlaylist').value;
        if (validatePlname(newPl) === true) {
            plist = newPl;
        } else {
            document.getElementById('addToPlaylistNewPlaylist').classList.add('is-invalid');
            return;
        }
    }
    if (plist !== '') {
        if (uri === 'SEARCH') {
            addAllFromSearchPlist(plist, null, false);
        }
        if (uri === 'ALBUM') {
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
    document.getElementById('renamePlaylistTo').classList.remove('is-invalid');
    modalRenamePlaylist.show();
    document.getElementById('renamePlaylistFrom').value = from;
    document.getElementById('renamePlaylistTo').value = '';
}

//eslint-disable-next-line no-unused-vars
function renamePlaylist() {
    let from = document.getElementById('renamePlaylistFrom').value;
    let to = document.getElementById('renamePlaylistTo').value;
    if (to !== from && validatePlname(to) === true && validatePlname(from) === true) {
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
    sendAPI("MPD_API_SMARTPLS_UPDATE", {"playlist": playlist});
}

//eslint-disable-next-line no-unused-vars
function updateSmartPlaylistClick() {
    let uri = document.getElementById('BrowsePlaylistsDetailList').getAttribute('data-uri');
    sendAPI("MPD_API_SMARTPLS_UPDATE", {"playlist": uri});
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
        showAddToPlaylist(item.getAttribute('data-uri'), '');
    }
}
