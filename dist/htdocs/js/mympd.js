/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

function sendAPI(method, params, callback, onerror) {
    let request = {"jsonrpc": "2.0", "id": 0, "method": method, "params": params};
    let ajaxRequest=new XMLHttpRequest();
    ajaxRequest.open('POST', subdir + '/api', true);
    ajaxRequest.setRequestHeader('Content-type', 'application/json');
    ajaxRequest.onreadystatechange = function() {
        if (ajaxRequest.readyState === 4) {
            if (ajaxRequest.responseText !== '') {
                let obj = JSON.parse(ajaxRequest.responseText);
                if (obj.error) {
                    showNotification(t(obj.error.message, obj.error.data), '', '', 'danger');
                    logError(JSON.stringify(obj.error));
                }
                else if (obj.result && obj.result.message && obj.result.message !== 'ok') {
                    logDebug('Got API response: ' + JSON.stringify(obj.result));
                    showNotification(t(obj.result.message, obj.result.data), '', '', 'success');
                }
                else if (obj.result && obj.result.message && obj.result.message === 'ok') {
                    logDebug('Got API response: ' + JSON.stringify(obj.result));
                }
                else if (obj.result && obj.result.method) {
                    logDebug('Got API response of type: ' + obj.result.method);
                }
                else {
                    logError('Got invalid API response: ' + JSON.stringify(obj));
                    if (onerror !== true) {
                        return;
                    }
                }
                if (callback !== undefined && typeof(callback) === 'function') {
                    if (obj.result !== undefined || onerror === true) {
                        logDebug('Calling ' + callback.name);
                        callback(obj);
                    }
                    else {
                        logDebug('Undefined resultset, skip calling ' + callback.name);
                    }
                }
            }
            else {
                logError('Empty response for request: ' + JSON.stringify(request));
                if (onerror === true) {
                    if (callback !== undefined && typeof(callback) === 'function') {
                        logDebug('Got empty API response calling ' + callback.name);
                        callback('');
                    }
                }
            }
        }
    };
    ajaxRequest.send(JSON.stringify(request));
    logDebug('Send API request: ' + method);
}

function webSocketConnect() {
    if (socket !== null && socket.readyState === WebSocket.OPEN) {
        logInfo('Socket already connected');
        websocketConnected = true;
        socketRetry = 0;
        return;
    }
    else if (socket !== null && socket.readyState === WebSocket.CONNECTING) {
        logInfo('Socket connection in progress');
        websocketConnected = false;
        socketRetry++;
        if (socketRetry > 20) {
            logError('Socket connection timed out');
            webSocketClose();
            setTimeout(function() {
                webSocketConnect();
            }, 1000);
            socketRetry = 0;
        }
        return;
    }
    else {
        websocketConnected = false;
    }
    
    let wsUrl = getWsUrl();
    socket = new WebSocket(wsUrl);
    socketRetry = 0;
    logInfo('Connecting to ' + wsUrl);

    try {
        socket.onopen = function() {
            logInfo('Websocket is connected');
            websocketConnected = true;
            if (websocketTimer !== null) {
                clearTimeout(websocketTimer);
                websocketTimer = null;
            }
        }

        socket.onmessage = function got_packet(msg) {
            var obj;
            try {
                obj = JSON.parse(msg.data);
                logDebug('Websocket notification: ' + JSON.stringify(obj));
            }
            catch(error) {
                logError('Invalid JSON data received: ' + msg.data);
                return;
            }
            
            if (obj.error) {
                showNotification(t(obj.error.message, obj.error.data), '', '', 'danger');
                return;
            }
            else if (obj.result) {
                showNotification(t(obj.result.message, obj.result.data), '', '', 'success');
                return;
            }

            switch (obj.method) {
                case 'welcome':
                    websocketConnected = true;
                    showNotification(t('Connected to myMPD'), wsUrl, '', 'success');
                    appRoute();
                    sendAPI("MPD_API_PLAYER_STATE", {}, parseState, true);
                    break;
                case 'update_state':
                    obj.result = obj.params;
                    parseState(obj);
                    break;
                case 'mpd_disconnected':
                    if (progressTimer) {
                        clearTimeout(progressTimer);
                    }
                    getSettings(true);
                    break;
                case 'mpd_connected':
                    showNotification(t('Connected to MPD'), '', '', 'success');
                    sendAPI("MPD_API_PLAYER_STATE", {}, parseState);
                    getSettings(true);
                    break;
                case 'update_queue':
                    if (app.current.app === 'Queue') {
                        getQueue();
                    }
                    obj.result = obj.params;
                    parseUpdateQueue(obj);
                    break;
                case 'update_options':
                    getSettings();
                    break;
                case 'update_outputs':
                    sendAPI("MPD_API_PLAYER_OUTPUT_LIST", {}, parseOutputs);
                    break;
                case 'update_started':
                    updateDBstarted(false);
                    break;
                case 'update_database':
                    //fall through
                case 'update_finished':
                    updateDBfinished(obj.method);
                    break;
                case 'update_volume':
                    obj.result = obj.params;
                    parseVolume(obj);
                    break;
                case 'update_stored_playlist':
                    if (app.current.app === 'Browse' && app.current.tab === 'Playlists' && app.current.view === 'All') {
                        sendAPI("MPD_API_PLAYLIST_LIST", {"offset": app.current.page, "searchstr": app.current.search}, parsePlaylists);
                    }
                    else if (app.current.app === 'Browse' && app.current.tab === 'Playlists' && app.current.view === 'Detail') {
                        sendAPI("MPD_API_PLAYLIST_CONTENT_LIST", {"offset": app.current.page, "searchstr": app.current.search, "uri": app.current.search, "cols": settings.colsBrowsePlaylistsDetail}, parsePlaylists);
                    }
                    break;
                case 'update_lastplayed':
                    if (app.current.app === 'Queue' && app.current.tab === 'LastPlayed') {
                        sendAPI("MPD_API_QUEUE_LAST_PLAYED", {"offset": app.current.page, "cols": settings.colsQueueLastPlayed}, parseLastPlayed);
                    }
                    break;
                case 'update_jukebox':
                    if (app.current.app === 'Queue' && app.current.tab === 'Jukebox') {
                        sendAPI("MPD_API_JUKEBOX_LIST", {"offset": app.current.page, "cols": settings.colsQueueJukebox}, parseJukeboxList);
                    }
                    break;
                case 'error':
                    if (document.getElementById('alertMpdState').classList.contains('hide')) {
                        showNotification(t(obj.params.message), '', '', 'danger');
                    }
                    break;
                case 'warn':
                    if (document.getElementById('alertMpdState').classList.contains('hide')) {
                        showNotification(t(obj.params.message), '', '', 'warning');
                    }
                    break;
                case 'info':
                    if (document.getElementById('alertMpdState').classList.contains('hide')) {
                        showNotification(t(obj.params.message), '', '', 'success');
                    }
                    break;
                default:
                    break;
            }
        }

        socket.onclose = function(){
            logError('Websocket is disconnected');
            websocketConnected = false;
            if (appInited === true) {
                toggleUI();
                if (progressTimer) {
                    clearTimeout(progressTimer);
                }
            }
            else {
                showAppInitAlert(t('Websocket connection failed'));
                logError('Websocket connection failed.');
            }
            if (websocketTimer !== null) {
                clearTimeout(websocketTimer);
                websocketTimer = null;
            }
            websocketTimer = setTimeout(function() {
                logInfo('Reconnecting websocket');
                toggleAlert('alertMympdState', true, t('Websocket connection failed, trying to reconnect') + '&nbsp;&nbsp;<div class="spinner-border spinner-border-sm"></div>');
                webSocketConnect();
            }, 3000);
            socket = null;
        }

    } catch(error) {
        logError(error);
    }
}

function webSocketClose() {
    if (websocketTimer !== null) {
        clearTimeout(websocketTimer);
        websocketTimer = null;
    }
    if (socket !== null) {
        socket.onclose = function () {}; // disable onclose handler first
        socket.close();
        socket = null;
    }
    websocketConnected = false;
}

function getWsUrl() {
    let hostname = window.location.hostname;
    let protocol = window.location.protocol;
    let port = window.location.port;
    
    if (protocol === 'https:') {
        protocol = 'wss://';
    }
    else {
        protocol = 'ws://';
    }

    let wsUrl = protocol + hostname + (port !== '' ? ':' + port : '') + subdir + '/ws/';
    return wsUrl;
}
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

function navBrowseHandler(event) {
    if (event.target.nodeName === 'BUTTON') {
        const tag = event.target.getAttribute('data-tag');
        if (tag === 'Playlists' || tag === 'Filesystem') {
            appGoto('Browse', tag, undefined);
            return;
        }
        
        if (app.current.app === 'Browse' && app.current.tab !== 'Database') {
            let view = app.apps.Browse.tabs.Database.active;
            appGoto('Browse', 'Database', view);
            return;
        }
        if (tag !== 'Album') {
            app.current.filter = tag;
            app.current.sort = tag;
        }
        app.current.search = '';
        document.getElementById('searchDatabaseMatch').value = 'contains';
        appGoto(app.current.app, app.current.tab, app.current.view, 
            '0', app.current.filter, app.current.sort, tag, app.current.search);
    }
}

function gotoBrowse() {
    let x = event.target;
    let tag = x.getAttribute('data-tag');
    let name = decodeURI(x.getAttribute('data-name'));
    if (tag === null) {
        tag = x.parentNode.getAttribute('data-tag');
        name = decodeURI(x.parentNode.getAttribute('data-name'));
    }
    if (tag !== '' && name !== '' && name !== '-' && settings.browsetags.includes(tag)) {
        appGoto('Browse', 'Database', 'List', '0', tag, 'AlbumArtist', 'Album', name);
    }
}

function parseFilesystem(obj) {
    let list = app.current.app + (app.current.tab === 'Filesystem' ? app.current.tab : '');
    let table = document.getElementById(app.current.app + (app.current.tab === undefined ? '' : app.current.tab) + 'List');
    let tbody = table.getElementsByTagName('tbody')[0];
    let colspan = settings['cols' + list].length;
    colspan--;

    if (obj.error) {
        tbody.innerHTML = '<tr><td><span class="material-icons">error_outline</span></td>' +
                          '<td colspan="' + colspan + '">' + t(obj.error.message) + '</td></tr>';
        document.getElementById(app.current.app + (app.current.tab === undefined ? '' : app.current.tab) + 'List').classList.remove('opacity05');
        //document.getElementById('cardFooterBrowse').innerText = '';
        return;
    }
    
    if (app.current.app === 'Browse' && app.current.tab === 'Filesystem') {
        const imageList = document.getElementById('BrowseFilesystemImages');
        imageList.innerHTML = '';
        if ((obj.result.images.length === 0 && obj.result.bookletPath === '') || settings.publish === false) {
            imageList.classList.add('hide');
        }
        else {
            imageList.classList.remove('hide');
        }
        if (obj.result.bookletPath !== '' && settings.publish === true) {
            let img = document.createElement('div');
            img.style.backgroundImage = 'url("' + subdir + '/assets/coverimage-booklet.svg")';
            img.classList.add('booklet');
            img.setAttribute('data-href', subdir + '/browse/music/' + obj.result.bookletPath);
            img.title = t('Booklet');
            imageList.appendChild(img);
        }
        for (let i = 0; i < obj.result.images.length; i++) {
            let img = document.createElement('div');
            img.style.backgroundImage = 'url("' + subdir + '/browse/music/' + obj.result.images[i] + '"),url("assets/coverimage-loading.svg")';
            imageList.appendChild(img);
        }
    }
    let nrItems = obj.result.returnedEntities;
    let tr = tbody.getElementsByTagName('tr');
    let navigate = document.activeElement.parentNode.parentNode === table ? true : false;
    let activeRow = 0;
    for (let i = 0; i < nrItems; i++) {
        let uri = encodeURI(obj.result.data[i].uri);
        let row = document.createElement('tr');
        let tds = '';
        row.setAttribute('data-type', obj.result.data[i].Type);
        row.setAttribute('data-uri', uri);
        row.setAttribute('tabindex', 0);
        if (obj.result.data[i].Type === 'song') {
            row.setAttribute('data-name', obj.result.data[i].Title);
        }
        else {
            row.setAttribute('data-name', obj.result.data[i].name);
        }
        
        switch(obj.result.data[i].Type) {
            case 'dir':
            case 'smartpls':
            case 'plist':
                for (let c = 0; c < settings['cols' + list].length; c++) {
                    tds += '<td data-col="' + settings['cols' + list][c] + '">';
                    if (settings['cols' + list][c] === 'Type') {
                        if (obj.result.data[i].Type === 'dir') {
                            tds += '<span class="material-icons">folder_open</span>';
                        }
                        else {
                            tds += '<span class="material-icons">' + (obj.result.data[i].Type === 'smartpls' ? 'queue_music' : 'list') + '</span>';
                        }
                    }
                    else if (settings['cols' + list][c] === 'Title') {
                        tds += e(obj.result.data[i].name);
                    }
                    tds += '</td>';
                }
                tds += '<td data-col="Action"><a href="#" class="material-icons color-darkgrey">' + ligatureMore + '</a></td>';
                row.innerHTML = tds;
                break;
            case 'song':
                obj.result.data[i].Duration = beautifySongDuration(obj.result.data[i].Duration);
                for (let c = 0; c < settings['cols' + list].length; c++) {
                    tds += '<td data-col="' + settings['cols' + list][c] + '">';
                    if (settings['cols' + list][c] === 'Type') {
                        tds += '<span class="material-icons">music_note</span>';
                    }
                    else {
                        tds += e(obj.result.data[i][settings['cols' + list][c]]);
                    }
                    tds += '</td>';
                }
                tds += '<td data-col="Action"><a href="#" class="material-icons color-darkgrey">' + ligatureMore + '</a></td>';
                row.innerHTML = tds;
                break;
        }
        if (i < tr.length) {
            activeRow = replaceTblRow(tr[i], row) === true ? i : activeRow;
        }
        else {
            tbody.append(row);
        }
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
        tbody.innerHTML = '<tr><td><span class="material-icons">error_outline</span></td>' +
                          '<td colspan="' + colspan + '">' + t('Empty list') + '</td></tr>';
    }
    document.getElementById(app.current.app + (app.current.tab === undefined ? '' : app.current.tab) + 'List').classList.remove('opacity05');
    //document.getElementById('cardFooterBrowse').innerText = t('Num entries', obj.result.totalEntities);
}

function addAllFromBrowseFilesystem() {
    sendAPI("MPD_API_QUEUE_ADD_TRACK", {"uri": app.current.search});
    showNotification(t('Added all songs'), '', '', 'success');
}

function addAllFromBrowseDatabasePlist(plist) {
    if (app.current.search.length >= 2) {
        sendAPI("MPD_API_DATABASE_SEARCH", {"plist": plist, "filter": app.current.view, "searchstr": app.current.search, "offset": 0, "cols": settings.colsSearch, "replace": false});
    }
}

function parseBookmarks(obj) {
    let list = '<table class="table table-sm table-dark table-borderless mb-0">';
    for (let i = 0; i < obj.result.returnedEntities; i++) {
        list += '<tr data-id="' + obj.result.data[i].id + '" data-type="' + obj.result.data[i].type + '" ' +
                'data-uri="' + encodeURI(obj.result.data[i].uri) + '">' +
                '<td class="nowrap"><a class="text-light" href="#" data-href="goto">' + e(obj.result.data[i].name) + '</a></td>' +
                '<td><a class="text-light material-icons material-icons-small" href="#" data-href="edit">edit</a></td><td>' +
                '<a class="text-light material-icons material-icons-small" href="#" data-href="delete">delete</a></td></tr>';
    }
    if (obj.result.returnedEntities === 0) {
        list += '<tr><td class="text-light nowrap">' + t('No bookmarks found') + '</td></tr>';
    }
    list += '</table>';
    document.getElementById('BrowseFilesystemBookmarks').innerHTML = list;
}

function showBookmarkSave(id, name, uri, type) {
    document.getElementById('saveBookmarkName').classList.remove('is-invalid');
    document.getElementById('saveBookmarkId').value = id;
    document.getElementById('saveBookmarkName').value = name;
    document.getElementById('saveBookmarkUri').value = uri;
    document.getElementById('saveBookmarkType').value = type;
    modalSaveBookmark.show();
}

//eslint-disable-next-line no-unused-vars
function saveBookmark() {
    let id = parseInt(document.getElementById('saveBookmarkId').value);
    let name = document.getElementById('saveBookmarkName').value;
    let uri = document.getElementById('saveBookmarkUri').value;
    let type = document.getElementById('saveBookmarkType').value;
    if (name !== '') {
        sendAPI("MYMPD_API_BOOKMARK_SAVE", {"id": id, "name": name, "uri": uri, "type": type});
        modalSaveBookmark.hide();
    }
    else {
        document.getElementById('saveBookmarkName').classList.add('is-invalid');
    }
}

function parseDatabase(obj) {
    let nrItems = obj.result.returnedEntities;
    let cardContainer = document.getElementById('BrowseDatabaseCards');
    let cols = cardContainer.getElementsByClassName('col');
    const has_io = 'IntersectionObserver' in window ? true : false;

    if (cols.length === 0) {
        cardContainer.innerHTML = '';
    }
    for (let i = 0; i < nrItems; i++) {
        let col = document.createElement('div');
        col.classList.add('col', 'px-0', 'flex-grow-0');
        if (obj.result.data[i].AlbumArtist === '') {
            obj.result.data[i].AlbumArtist = t('Unknown artist');
        }
        if (obj.result.data[i].Album === '') {
            obj.result.data[i].Album = t('Unknown album');
        }
        let id;
        let html;
        let picture = '';
        if (obj.result.tag === 'Album') {
            id = genId('database' + obj.result.data[i].Album + obj.result.data[i].AlbumArtist);
            picture = subdir + '/albumart/' + encodeURI(obj.result.data[i].FirstSongUri);
            html = '<div class="card card-grid clickable" data-picture="' + picture  + '" ' + 
                       'data-uri="' + encodeURI(obj.result.data[i].FirstSongUri.replace(/\/[^/]+$/, '')) + '" ' +
                       'data-type="dir" data-name="' + encodeURI(obj.result.data[i].Album) + '" ' +
                       'data-album="' + encodeURI(obj.result.data[i].Album) + '" ' +
                       'data-albumartist="' + encodeURI(obj.result.data[i].AlbumArtist) + '" tabindex="0">' +
                   '<div class="card-body album-cover-loading album-cover-grid bg-white" id="' + id + '"></div>' +
                   '<div class="card-footer card-footer-grid p-2" title="' + e(obj.result.data[i].AlbumArtist) + ': ' + e(obj.result.data[i].Album) + '">' +
                   e(obj.result.data[i].Album) + '<br/><small>' + e(obj.result.data[i].AlbumArtist) + '</small>' +
                   '</div></div>';
        }
        else {
            id = genId('database' + obj.result.data[i].value);
            picture = subdir + '/tagpics/' + obj.result.tag + '/' + encodeURI(obj.result.data[i].value);
            html = '<div class="card card-grid clickable" data-picture="' + picture + '" data-tag="' + encodeURI(obj.result.data[i].value) + '" tabindex="0">' +
                   (obj.result.pics === true ? '<div class="card-body album-cover-loading album-cover-grid bg-white" id="' + id + '"></div>' : '') +
                   '<div class="card-footer card-footer-grid p-2" title="' + e(obj.result.data[i].value) + '">' +
                   e(obj.result.data[i].value) + '<br/>' +
                   '</div></div>';
        }
        col.innerHTML = html;
        let replaced = false;
        if (i < cols.length) {
            if (cols[i].firstChild.getAttribute('data-picture') !== col.firstChild.getAttribute('data-picture')) {
                cols[i].replaceWith(col);
                replaced = true;
            }
        }
        else {
            cardContainer.append(col);
            replaced = true;
        }
        if (replaced === true) {
            if (has_io === true) {
                let options = {
                    root: null,
                    rootMargin: '0px',
                };
                let observer = new IntersectionObserver(setGridImage, options);
                observer.observe(col);
            }
            else {
                col.firstChild.firstChild.style.backgroundImage = picture;
            }
        }
    }
    let colsLen = cols.length - 1;
    for (let i = colsLen; i >= nrItems; i --) {
        cols[i].remove();
    }
    
    setPagination(obj.result.totalEntities, obj.result.returnedEntities);
                    
    if (nrItems === 0) {
        cardContainer.innerHTML = '<div><span class="material-icons">error_outline</span>&nbsp;' + t('Empty list') + '</div>';
    }
    //document.getElementById('cardFooterBrowse').innerText = gtPage('Num entries', obj.result.returnedEntities, obj.result.totalEntities);
}

function setGridImage(changes, observer) {
    changes.forEach(change => {
        if (change.intersectionRatio > 0) {
            observer.unobserve(change.target);
            const uri = decodeURI(change.target.firstChild.getAttribute('data-picture'));
            const body = change.target.firstChild.getElementsByClassName('card-body')[0];
            if (body) {
                body.style.backgroundImage = 'url("' + uri + '"), url("' + subdir + '/assets/coverimage-loading.svg")';
            }
        }
    });
}

function parseAlbumDetails(obj) {
    const coverEl = document.getElementById('viewDetailDatabaseCover');
    coverEl.style.backgroundImage = 'url("' + subdir + '/albumart/' + obj.result.data[0].uri + '"), url("' + subdir + '/assets/coverimage-loading.svg")';
    coverEl.setAttribute('data-images', obj.result.images.join(';;'));
    const infoEl = document.getElementById('viewDetailDatabaseInfo');
    infoEl.innerHTML = '<h1>' + e(obj.result.Album) + '</h1>' +
        '<small> ' + t('AlbumArtist') + '</small><p>' + e(obj.result.AlbumArtist) + '</p>' +
        (obj.result.bookletPath === '' || settings.featBrowse === false ? '' : 
            '<span class="text-light material-icons">description</span>&nbsp;<a class="text-light" target="_blank" href="' + subdir + '/browse/music/' + 
            e(obj.result.bookletPath) + '">' + t('Download booklet') + '</a>') +
        '</p>';
    const table = document.getElementById('BrowseDatabaseDetailList');
    const tbody = table.getElementsByTagName('tbody')[0];
    const nrCols = settings.colsBrowseDatabaseDetail.length;
    let titleList = '';
    if (obj.result.Discs > 1) {
        titleList = '<tr class="not-clickable"><td><span class="material-icons">album</span></td><td colspan="' + nrCols +'">' + t('Disc 1') + '</td></tr>';
    }
    let nrItems = obj.result.returnedEntities;
    let lastDisc = parseInt(obj.result.data[0].Disc);
    for (let i = 0; i < nrItems; i++) {
        if (lastDisc < parseInt(obj.result.data[i].Disc)) {
            titleList += '<tr class="not-clickable"><td><span class="material-icons">album</span></td><td colspan="' + nrCols +'">' + 
                t('Disc') + ' ' + e(obj.result.data[i].Disc) + '</td></tr>';
        }
        if (obj.result.data[i].Duration) {
            obj.result.data[i].Duration = beautifySongDuration(obj.result.data[i].Duration);
        }
        titleList += '<tr tabindex="0" data-type="song" data-name="' + obj.result.data[i].Title + '" data-uri="' + encodeURI(obj.result.data[i].uri) + '">';
        for (let c = 0; c < settings.colsBrowseDatabaseDetail.length; c++) {
            titleList += '<td data-col="' + settings.colsBrowseDatabaseDetail[c] + '">' + e(obj.result.data[i][settings.colsBrowseDatabaseDetail[c]]) + '</td>';
        }
        titleList += '<td data-col="Action"><a href="#" class="material-icons color-darkgrey">' + ligatureMore + '</a></td></tr>';
        lastDisc = obj.result.data[i].Disc;
    }
    tbody.innerHTML = titleList;
    //document.getElementById('cardFooterBrowse').innerHTML = t('Num songs', obj.result.totalEntities) + ' &ndash; ' + beautifyDuration(obj.result.totalTime);
    document.getElementById('BrowseDatabaseDetailList').classList.remove('opacity05');
}

//eslint-disable-next-line no-unused-vars
function backToAlbumGrid() {
    appGoto('Browse', 'Database', 'List');
}  

//eslint-disable-next-line no-unused-vars
function addAlbum(action) {
    const album = decodeURI(app.current.tag);
    const albumArtist = decodeURI(app.current.search);
    const expression = '((Album == \'' + album + '\') AND (AlbumArtist == \'' + albumArtist + '\'))';
    if (action === 'appendQueue') {
        addAllFromSearchPlist('queue', expression, false);
    }
    else if (action === 'replaceQueue') {
        addAllFromSearchPlist('queue', expression, true);
    }
    else if (action === 'addPlaylist') {
        showAddToPlaylist('ALBUM', expression);
    }
}

function searchAlbumgrid(x) {
    let expression = '';
    let crumbs = document.getElementById('searchDatabaseCrumb').children;
    for (let i = 0; i < crumbs.length; i++) {
        if (i > 0) {
            expression += ' AND ';
        }
        expression += '(' + decodeURI(crumbs[i].getAttribute('data-filter')) + ')';
    }
    if (x !== '') {
        if (expression !== '') {
            expression += ' AND ';
        }
        let match = document.getElementById('searchDatabaseMatch');
        expression += '(' + app.current.filter + ' ' + match.options[match.selectedIndex].value + ' \'' + x +'\')';
    }
    
    if (expression.length <= 2) {
        expression = '';
    }
    appGoto(app.current.app, app.current.tab, app.current.view, 
        '0', app.current.filter, app.current.sort, app.current.tag, expression);
}
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

function navigateGrid(grid, keyCode) {
    let handled = false;
    if (keyCode === 'Enter') {
        if (app.current.app === 'Browse' && app.current.tab === 'Database' && app.current.view === 'List') {
            if (app.current.tag === 'Album') {
                appGoto('Browse', 'Database', 'Detail', '0','Album','AlbumArtist', 
                    decodeURI(grid.getAttribute('data-album')),
                    decodeURI(grid.getAttribute('data-albumartist')));
            }
            else {
                app.current.search = '';
                document.getElementById('searchDatabaseStr').value = '';
                appGoto(app.current.app, app.current.card, undefined, '0', 'Album', 'AlbumArtist', 'Album',
                    '(' + app.current.tag + ' == \'' + decodeURI(grid.getAttribute('data-tag')) + '\')');
            }
            handled = true;
        }
        else if (app.current.app === 'Home') {
            const href = event.target.getAttribute('data-href');
            if (href !== null) {
               parseCmd(event, href);
            }
        }
    }
    else if (keyCode === ' ') {
        if (app.current.app === 'Browse' && app.current.tab === 'Database' && app.current.view === 'List') {
            if (app.current.tag === 'Album') {
                showMenu(grid.getElementsByClassName('card-footer')[0], event);
            }
            handled = true;
        }
        else if (app.current.app === 'Home') {
            showMenu(grid.getElementsByClassName('card-footer')[0], event);
        }
    }
    else if (keyCode === 'ArrowDown' || keyCode === 'ArrowUp') {
        const cur = grid;
        const next = keyCode === 'ArrowDown' ? (grid.parentNode.nextElementSibling !== null ? grid.parentNode.nextElementSibling.firstChild : null)
                                             : (grid.parentNode.previousElementSibling !== null ? grid.parentNode.previousElementSibling.firstChild : null);
        if (next !== null) {
            next.focus();
            cur.classList.remove('selected');
            next.classList.add('selected');
            handled = true;
            scrollFocusIntoView();
        }
    }
    else if (keyCode === 'Escape') {
        const cur = grid;
        cur.blur();
        cur.classList.remove('selected');
        handled = true;
    }
    if (handled === true) {
        event.stopPropagation();
        event.preventDefault();
    }
}
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

function parseHome(obj) {
    let nrItems = obj.result.returnedEntities;
    let cardContainer = document.getElementById('HomeCards');
    let cols = cardContainer.getElementsByClassName('col');
    if (cols.length === 0) {
        cardContainer.innerHTML = '';
    }
    for (let i = 0; i < nrItems; i++) {
        let col = document.createElement('div');
        col.classList.add('col', 'px-0', 'flex-grow-0');
        if (obj.result.data[i].AlbumArtist === '') {
            obj.result.data[i].AlbumArtist = t('Unknown artist');
        }
        if (obj.result.data[i].Album === '') {
            obj.result.data[i].Album = t('Unknown album');
        }
        let href=JSON.stringify({"cmd": obj.result.data[i].cmd, "options": obj.result.data[i].options});
        let html = '<div class="card home-icons clickable" draggable="true" tabindex="0" data-pos="' + i + '" data-href=\'' + 
                   href + '\'  title="' + e(obj.result.data[i].name) + '">' +
                   '<div class="card-body material-icons">' + e(obj.result.data[i].ligature) + '</div>' +
                   '<div class="card-footer card-footer-grid p-2">' +
                   e(obj.result.data[i].name) + 
                   '</div></div>';
        col.innerHTML = html;
        if (i < cols.length) {
            cols[i].replaceWith(col);
        }
        else {
            cardContainer.append(col);
        }
        if (obj.result.data[i].image !== '') {
            col.getElementsByClassName('card-body')[0].style.backgroundImage = 'url("' + subdir + '/browse/pics/' + obj.result.data[i].image + '")';
        }
        if (obj.result.data[i].bgcolor !== '') {
            col.getElementsByClassName('card-body')[0].style.backgroundColor = obj.result.data[i].bgcolor;
        }
    }
    let colsLen = cols.length - 1;
    for (let i = colsLen; i >= nrItems; i --) {
        cols[i].remove();
    }
                    
    if (nrItems === 0) {
        cardContainer.innerHTML = '<div>' + t('You have not added any Home Icons yet. You can add views, playlists and scripts.') + '</div>';
    }
}

function dragAndDropHome() {
    const homeCards = document.getElementById('HomeCards');
    homeCards.addEventListener('dragstart', function(event) {
        if (event.target.classList.contains('home-icons')) {
            event.target.classList.add('opacity05');
            event.dataTransfer.setDragImage(event.target, 0, 0);
            event.dataTransfer.effectAllowed = 'move';
            dragSrc = event.target;
            dragEl = event.target.cloneNode(true);
        }
    }, false);
    homeCards.addEventListener('dragleave', function(event) {
        event.preventDefault();
        if (dragEl.classList.contains('home-icons') === false) {
            return;
        }
        if (event.target.nodeName === 'DIV' && event.target.classList.contains('home-icons')) {
            event.target.classList.remove('dragover-icon');
        }
    }, false);
    homeCards.addEventListener('dragover', function(event) {
        event.preventDefault();
        if (dragEl.classList.contains('home-icons') === false) {
            return;
        }
        let th = homeCards.getElementsByClassName('dragover-icon');
        let thLen = th.length;
        for (let i = 0; i < thLen; i++) {
            th[i].classList.remove('dragover-icon');
        }
        if (event.target.nodeName === 'DIV' && event.target.classList.contains('home-icons')) {
            event.target.classList.add('dragover-icon');
        }
        else if (event.target.nodeName === 'DIV' && event.target.parentNode.classList.contains('home-icons')) {
            event.target.parentNode.classList.add('dragover-icon');
        }
        event.dataTransfer.dropEffect = 'move';
    }, false);
    homeCards.addEventListener('dragend', function(event) {
        event.preventDefault();
        if (dragEl.classList.contains('home-icons') === false) {
            return;
        }
        let th = homeCards.getElementsByClassName('dragover-icon');
        let thLen = th.length;
        for (let i = 0; i < thLen; i++) {
            th[i].classList.remove('dragover-icon');
        }
        dragSrc.classList.remove('opacity05');
    }, false);
    homeCards.addEventListener('drop', function(event) {
        event.preventDefault();
        event.stopPropagation();
        if (dragEl.classList.contains('home-icons') === false) {
            return;
        }
        let dst = event.target;
        if (dst.nodeName === 'DIV') {
            if (dst.classList.contains('card-body')) {
                dst = dst.parentNode;
            }
            if (dst.classList.contains('home-icons')) {
                dragEl.classList.remove('opacity05');
                const to = parseInt(dst.getAttribute('data-pos'));
                const from = parseInt(dragSrc.getAttribute('data-pos'));
                if (isNaN(to) === false && isNaN(from) === false && from !== to) {
                    sendAPI("MYMPD_API_HOME_ICON_MOVE", {"from": from, "to": to}, function(obj) {
                        parseHome(obj);
                    });
                }
            }
        }
        let th = homeCards.getElementsByClassName('dragover-icon');
        let thLen = th.length;
        for (let i = 0; i < thLen; i++) {
            th[i].classList.remove('dragover-icon');
        }

    }, false);
}

//eslint-disable-next-line no-unused-vars
function executeHomeIcon(pos) {
    const el = document.getElementById('HomeCards').children[pos].firstChild;
    parseCmd(null, el.getAttribute('data-href'));
}

//eslint-disable-next-line no-unused-vars
function addViewToHome() {
    _addHomeIcon('appGoto', '', 'preview', [app.current.app, app.current.tab, app.current.view, 
        app.current.page, app.current.filter, app.current.sort, app.current.tag, app.current.search]); 
}

//eslint-disable-next-line no-unused-vars
function addScriptToHome(name, script_def) {
    let script = JSON.parse(script_def);
    let options = [script.script, script.arguments.join(',')];
    _addHomeIcon('execScriptFromOptions', name, 'description', options);
}

//eslint-disable-next-line no-unused-vars
function addPlistToHome(uri, name) {
    _addHomeIcon('replaceQueue', name, 'list', ['plist', uri, name]);
}

function _addHomeIcon(cmd, name, ligature, options) {
    document.getElementById('modalEditHomeIconTitle').innerHTML = t('Add to homescreen');
    document.getElementById('inputHomeIconReplace').value = 'false';
    document.getElementById('inputHomeIconOldpos').value = '0';
    document.getElementById('inputHomeIconName').value = name;
    document.getElementById('inputHomeIconLigature').value = ligature;
    document.getElementById('inputHomeIconBgcolor').value = '#28a745';
    document.getElementById('selectHomeIconCmd').value = cmd;
    
    showHomeIconCmdOptions(options);
    getHomeIconPictureList('');
    
    document.getElementById('homeIconPreview').innerText = ligature;
    document.getElementById('homeIconPreview').style.backgroundColor = '#28a745';
    document.getElementById('homeIconPreview').style.backgroundImage = '';
    document.getElementById('divHomeIconLigature').classList.remove('hide');
    modalEditHomeIcon.show();
}

//eslint-disable-next-line no-unused-vars
function duplicateHomeIcon(pos) {
    _editHomeIcon(pos, false, "Duplicate home icon");
}

//eslint-disable-next-line no-unused-vars
function editHomeIcon(pos) {
    _editHomeIcon(pos, true, "Edit home icon");
}

function _editHomeIcon(pos, replace, title) {
    document.getElementById('modalEditHomeIconTitle').innerHTML = t(title);
    sendAPI("MYMPD_API_HOME_ICON_GET", {"pos": pos}, function(obj) {
        document.getElementById('inputHomeIconReplace').value = replace;
        document.getElementById('inputHomeIconOldpos').value = pos;
        document.getElementById('inputHomeIconName').value = obj.result.data.name;
        document.getElementById('inputHomeIconLigature').value = obj.result.data.ligature;
        document.getElementById('inputHomeIconBgcolor').value = obj.result.data.bgcolor;
        document.getElementById('selectHomeIconCmd').value = obj.result.data.cmd;

        showHomeIconCmdOptions(obj.result.data.options);
        getHomeIconPictureList(obj.result.data.image);

        document.getElementById('homeIconPreview').innerText = obj.result.data.ligature;
        document.getElementById('homeIconPreview').style.backgroundColor = obj.result.data.bgcolor;
        
        if (obj.result.data.image === '') {
            document.getElementById('divHomeIconLigature').classList.remove('hide');
            document.getElementById('homeIconPreview').style.backgroundImage = '';
        }
        else {
            document.getElementById('divHomeIconLigature').classList.add('hide');
            document.getElementById('homeIconPreview').style.backgroundImage = 'url(' + subdir + '"/browse/pics/' + obj.result.data.image + '")';
        }
        
        modalEditHomeIcon.show();
    });
}

//eslint-disable-next-line no-unused-vars
function saveHomeIcon() {
    let formOK = true;
    let nameEl = document.getElementById('inputHomeIconName');
    if (!validateNotBlank(nameEl)) {
        formOK = false;
    }
    if (formOK === true) {
        let options = [];
        let optionEls = document.getElementById('divHomeIconOptions').getElementsByTagName('input');
        for (let i = 0; i < optionEls.length; i++) {
            //workarround for parsing arrays with empty values in frozen
            let value = optionEls[i].value !== '' ? optionEls[i].value : '!undefined!';
            options.push(value);
        }
        const image = getSelectValue('selectHomeIconImage');
        sendAPI("MYMPD_API_HOME_ICON_SAVE", {
            "replace": (document.getElementById('inputHomeIconReplace').value === 'true' ? true : false),
            "oldpos": parseInt(document.getElementById('inputHomeIconOldpos').value),
            "name": nameEl.value,
            "ligature": (image === '' ? document.getElementById('inputHomeIconLigature').value : ''),
            "bgcolor": document.getElementById('inputHomeIconBgcolor').value,
            "image": image,
            "cmd": document.getElementById('selectHomeIconCmd').value,
            "options": options
            }, function() {
                modalEditHomeIcon.hide();
                sendAPI("MYMPD_API_HOME_LIST", {}, function(obj) {
                    parseHome(obj);
                });
            });
    }
}

//eslint-disable-next-line no-unused-vars
function deleteHomeIcon(pos) {
    sendAPI("MYMPD_API_HOME_ICON_DELETE", {"pos": pos}, function(obj) {
        parseHome(obj);
    });
}

function showHomeIconCmdOptions(values) {
    let list = '';
    const optionsText =getSelectedOptionAttribute('selectHomeIconCmd', 'data-options')
    if (optionsText !== undefined) {    
        const options = JSON.parse(optionsText);
        for (let i = 0; i < options.options.length; i++) {
            let value = values !== undefined ? values[i] !== undefined ? values[i] : '' : '';
            list += '<div class="form-group row">' +
                '<label class="col-sm-4 col-form-label">' + t(options.options[i]) + '</label>' +
                '<div class="col-sm-8"><input class="form-control border-secondary" value="' + e(value) + '"></div>' +
                '</div>';
        }
    }
    document.getElementById('divHomeIconOptions').innerHTML = list;
}

function getHomeIconPictureList(picture) {
    sendAPI("MYMPD_API_HOME_ICON_PICTURE_LIST", {}, function(obj) {
        let options = '<option value="">' + t('Use ligature') + '</option>';
        for (let i = 0; i < obj.result.returnedEntities; i++) {
            options += '<option value="' + e(obj.result.data[i]) + '">' + e(obj.result.data[i])  + '</option>';
        }
        let sel = document.getElementById('selectHomeIconImage');
        sel.innerHTML = options;
        sel.value = picture;
    });
}
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

//eslint-disable-next-line no-unused-vars
function delQueueJukeboxSong(pos) {
    sendAPI("MPD_API_JUKEBOX_RM", {"pos": pos}, function() {
        sendAPI("MPD_API_JUKEBOX_LIST", {"offset": app.current.page, "cols": settings.colsQueueJukebox}, parseJukeboxList);
    });
}

function parseJukeboxList(obj) {
    let nrItems = obj.result.returnedEntities;
    let table = document.getElementById('QueueJukeboxList');
    let navigate = document.activeElement.parentNode.parentNode === table ? true : false;
    let activeRow = 0;
    let tbody = table.getElementsByTagName('tbody')[0];
    let tr = tbody.getElementsByTagName('tr');
    for (let i = 0; i < nrItems; i++) {
        obj.result.data[i].Duration = beautifySongDuration(obj.result.data[i].Duration);
        obj.result.data[i].LastPlayed = localeDate(obj.result.data[i].LastPlayed);
        let row = document.createElement('tr');
        row.setAttribute('data-uri', obj.result.data[i].uri);
        row.setAttribute('data-name', obj.result.data[i].Title);
        row.setAttribute('data-type', 'song');
        row.setAttribute('data-pos', i);
        row.setAttribute('tabindex', 0);
        let tds = '';
        for (let c = 0; c < settings.colsQueueJukebox.length; c++) {
            tds += '<td data-col="' + settings.colsQueueJukebox[c] + '">' + e(obj.result.data[i][settings.colsQueueJukebox[c]]) + '</td>';
        }
        tds += '<td data-col="Action">';
        if (obj.result.data[i].uri !== '') {
            tds += '<a href="#" class="material-icons color-darkgrey">' + ligatureMore + '</a>';
        }
        tds += '</td>';
        row.innerHTML = tds;
        if (i < tr.length) {
            activeRow = replaceTblRow(tr[i], row) === true ? i : activeRow;
        }
        else {
            tbody.append(row);
        }
    }
    let trLen = tr.length - 1;
    for (let i = trLen; i >= nrItems; i --) {
        tr[i].remove();
    }                    

    let colspan = settings['colsQueueJukebox'].length;
    colspan--;
    
    if (nrItems === 0) {
        tbody.innerHTML = '<tr><td><span class="material-icons">error_outline</span></td>' +
            '<td colspan="' + colspan + '">' + t('Empty list') + '</td></tr>';
    }

    if (navigate === true) {
        focusTable(activeRow);
    }

    setPagination(obj.result.totalEntities, obj.result.returnedEntities);
    document.getElementById('QueueJukeboxList').classList.remove('opacity05');
}
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

//eslint-disable-next-line no-unused-vars
var keymap = {
    "ArrowLeft": {"cmd": "clickPrev", "options": [], "desc": "Previous song", "key": "keyboard_arrow_left"},
    "ArrowRight": {"cmd": "clickNext", "options": [], "desc": "Next song", "key": "keyboard_arrow_right"},
    " ": {"cmd": "clickPlay", "options": [], "desc": "Toggle play / pause", "key": "space_bar"},
    "s": {"cmd": "clickStop", "options": [], "desc": "Stop playing"},
    "-": {"cmd": "volumeStep", "options": ["down"], "desc": "Volume down"},
    "+": {"cmd": "volumeStep", "options": ["up"], "desc": "Volume up"},
    "c": {"cmd": "sendAPI", "options": [{"cmd": "MPD_API_QUEUE_CLEAR"}], "desc": "Clear queue"},
    "u": {"cmd": "updateDB", "options": ["", true], "desc": "Update database"},
    "r": {"cmd": "rescanDB", "options": ["", true], "desc": "Rescan database"},
    "p": {"cmd": "updateSmartPlaylists", "options": [false], "desc": "Update smart playlists", "req": "featSmartpls"},
    "a": {"cmd": "showAddToPlaylist", "options": ["stream", ""], "desc": "Add stream"},
    "t": {"cmd": "openModal", "options": ["modalSettings"], "desc": "Open settings"},
    "i": {"cmd": "clickTitle", "options": [], "desc": "Open song details"},
    "l": {"cmd": "openDropdown", "options": ["dropdownLocalPlayer"], "desc": "Open local player"},
    "0": {"cmd": "appGoto", "options": ["Playback"], "desc": "Goto playback"},
    "1": {"cmd": "appGoto", "options": ["Queue", "Current"], "desc": "Goto queue"},
    "2": {"cmd": "appGoto", "options": ["Queue", "LastPlayed"], "desc": "Goto last played"},
    "3": {"cmd": "appGoto", "options": ["Browse", "Database"], "desc": "Goto browse database", "req": "featTags"},
    "4": {"cmd": "appGoto", "options": ["Browse", "Playlists"], "desc": "Goto browse playlists", "req": "featPlaylists"},
    "5": {"cmd": "appGoto", "options": ["Browse", "Filesystem"], "desc": "Goto browse filesystem"},
    "6": {"cmd": "appGoto", "options": ["Search"], "desc": "Goto search"},
    "m": {"cmd": "openDropdown", "options": ["dropdownMainMenu"], "desc": "Open main menu"},
    "v": {"cmd": "openDropdown", "options": ["dropdownVolumeMenu"], "desc": "Open volume menu"},
    "S": {"cmd": "sendAPI", "options": [{"cmd": "MPD_API_QUEUE_SHUFFLE"}], "desc": "Shuffle queue"},
    "C": {"cmd": "sendAPI", "options": [{"cmd": "MPD_API_QUEUE_CROP"}], "desc": "Crop queue"},
    "?": {"cmd": "openModal", "options": ["modalAbout"], "desc": "Open about"},
    "/": {"cmd": "focusSearch", "options": [], "desc": "Focus search"},
    "n": {"cmd": "focusTable", "options": [], "desc": "Focus table"},
    "q": {"cmd": "queueSelectedItem", "options": [true], "desc": "Append item to queue"},
    "Q": {"cmd": "queueSelectedItem", "options": [false], "desc": "Replace queue with item"},
    "d": {"cmd": "dequeueSelectedItem", "options": [], "desc": "Remove item from queue"},
    "x": {"cmd": "addSelectedItemToPlaylist", "options": [], "desc": "Append item to playlist"},
    "F": {"cmd": "openFullscreen", "options": [], "desc": "Open fullscreen"}
};
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

function e(x) {
    if (isNaN(x)) {
        return x.replace(/([<>"'])/g, function(m0, m1) {
            if (m1 === '<') return '&lt;';
            else if (m1 === '>') return '&gt;';
            else if (m1 === '"') return '&quot;';
            else if (m1 === '\'') return '&apos;';
        }).replace(/\\u(003C|003E|0022|0027)/gi, function(m0, m1) {
            if (m1 === '003C') return '&lt;';
            else if (m1 === '003E') return '&gt;';
            else if (m1 === '0022') return '&quot;';
            else if (m1 === '0027') return '&apos;';
        });
    }
    return x;
}

function t(phrase, number, data) {
    let result = undefined;
    if (isNaN(number)) {
        data = number;
    }

    if (phrases[phrase]) {
        result = phrases[phrase][locale];
        if (result === undefined) {
            if (locale !== 'en-US') {
                logWarn('Phrase "' + phrase + '" for locale ' + locale + ' not found');
            }
            result = phrases[phrase]['en-US'];
        }
    }
    if (result === undefined) {
        result = phrase;
    }

    if (isNaN(number) === false) {
        let p = result.split(' |||| ');
        if (p.length > 1) {
            result = p[smartCount(number)];
        }
        result = result.replace('%{smart_count}', number);
    }
    
    if (data !== null) {
        result = result.replace(/%\{(\w+)\}/g, function(m0, m1) {
            return data[m1];
        });
    }
    
    return e(result);
}

function smartCount(number) {
    if (number === 0) { return 1; }
    else if (number === 1) { return 0; }
    else { return 1; }
}

function localeDate(secs) {
    let d;
    if (secs === undefined) {
       d  = new Date();
    }
    else {
        d = new Date(secs * 1000);
    }
    return d.toLocaleString(locale);
}

function beautifyDuration(x) {
    let days = Math.floor(x / 86400);
    let hours = Math.floor(x / 3600) - days * 24;
    let minutes = Math.floor(x / 60) - hours * 60 - days * 1440;
    let seconds = x - days * 86400 - hours * 3600 - minutes * 60;

    return (days > 0 ? days + '\u2009'+ t('Days') + ' ' : '') +
        (hours > 0 ? hours + '\u2009' + t('Hours') + ' ' + 
        (minutes < 10 ? '0' : '') : '') + minutes + '\u2009' + t('Minutes') + ' ' + 
        (seconds < 10 ? '0' : '') + seconds + '\u2009' + t('Seconds');
}

function beautifySongDuration(x) {
    let hours = Math.floor(x / 3600);
    let minutes = Math.floor(x / 60) - hours * 60;
    let seconds = x - hours * 3600 - minutes * 60;

    return (hours > 0 ? hours + ':' + (minutes < 10 ? '0' : '') : '') + 
        minutes + ':' + (seconds < 10 ? '0' : '') + seconds;
}

//eslint-disable-next-line no-unused-vars
function gtPage(phrase, returnedEntities, totalEntities) {
    if (totalEntities > -1) {
        return t(phrase, totalEntities);
    }
    else if (returnedEntities + app.current.page < settings.maxElementsPerPage) {
        return t(phrase, returnedEntities);
    }
    else {
        return '> ' + t(phrase, settings.maxElementsPerPage);
    }
}

function i18nHtml(root) {
    let attributes = [['data-phrase', 'innerText'], 
        ['data-title-phrase', 'title'], 
        ['data-placeholder-phrase', 'placeholder']
    ];
    for (let i = 0; i < attributes.length; i++) {
        let els = root.querySelectorAll('[' + attributes[i][0] + ']');
        let elsLen = els.length;
        for (let j = 0; j < elsLen; j++) {
            els[j][attributes[i][1]] = t(els[j].getAttribute(attributes[i][0]));
        }
    }
}
function setLocalPlayerUrl() {
    if (window.location.protocol === 'https:') {
        document.getElementById('infoLocalplayer').classList.remove('hide');
        document.getElementById('selectStreamMode').options[0].setAttribute('data-phrase','HTTPS Port');
    }
    else {
        document.getElementById('infoLocalplayer').classList.add('hide');
        document.getElementById('selectStreamMode').options[0].setAttribute('data-phrase','HTTP Port');
    }
    if (settings.streamUrl === '') {
        settings.mpdstream = window.location.protocol + '//';
        if (settings.mpdHost.match(/^127\./) !== null || settings.mpdHost === 'localhost' || settings.mpdHost.match(/^\//) !== null) {
            settings.mpdstream += window.location.hostname;
        }
        else {
            settings.mpdstream += settings.mpdHost;
        }
        settings.mpdstream += ':' + settings.streamPort + '/';
    } 
    else {
        settings.mpdstream = settings.streamUrl;
    }
    const localPlayer = document.getElementById('localPlayer');
    if (localPlayer.src !== settings.mpdstream) {
        localPlayer.pause();
        localPlayer.src = settings.mpdstream;
        localPlayer.load();
        setTimeout(function() {
            checkLocalPlayerState();
        }, 500);
    }

}

function clickCheckLocalPlayerState(event) {
    const el = event.target;
    el.classList.add('disabled');
    const parent = document.getElementById('localPlayer').parentNode;
    document.getElementById('localPlayer').remove();
    let localPlayer = document.createElement('audio');
    localPlayer.setAttribute('preload', 'none');
    localPlayer.setAttribute('controls', '');
    localPlayer.setAttribute('id', 'localPlayer');
    localPlayer.classList.add('mx-4');
    parent.appendChild(localPlayer);
    setLocalPlayerUrl();
    setTimeout(function() {
        el.classList.remove('disabled');
        localPlayer.play();
    }, 500);
}

function checkLocalPlayerState() {
    const localPlayer = document.getElementById('localPlayer');
    document.getElementById('errorLocalPlayback').classList.add('hide');
    document.getElementById('alertLocalPlayback').classList.add('hide');
    if (localPlayer.networkState === 0) {
        logDebug('localPlayer networkState: ' + localPlayer.networkState);
        document.getElementById('alertLocalPlayback').classList.remove('hide');
    }
    else if (localPlayer.networkState >=1) {
        logDebug('localPlayer networkState: ' + localPlayer.networkState);
    }
    if (localPlayer.networkState === 3) {
        logError('localPlayer networkState: ' + localPlayer.networkState);
        document.getElementById('errorLocalPlayback').classList.remove('hide');
    }
}
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

//eslint-disable-next-line no-unused-vars
function logError(line) {
    logLog(0, 'ERROR: ' + line);
}

//eslint-disable-next-line no-unused-vars
function logWarn(line) {
    logLog(1, 'WARN: ' + line);
}

//eslint-disable-next-line no-unused-vars
function logInfo(line) {
    logLog(2, 'INFO: ' + line);
}

//eslint-disable-next-line no-unused-vars
function logVerbose(line) {
    logLog(3, 'VERBOSE: ' + line);
}

//eslint-disable-next-line no-unused-vars
function logDebug(line) {
    logLog(4, 'DEBUG: ' + line);
}

function logLog(loglevel, line) {
    if (settings.loglevel >= loglevel) {
        if (loglevel === 0) {
            console.error(line);
        }
        else if (loglevel === 1) {
            console.warn(line);
        }
        else if (loglevel === 4) {
            console.debug(line);
        }
        else {
            console.log(line);
        }
    }
}
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

//eslint-disable-next-line no-unused-vars
function openFullscreen() {
    let elem = document.documentElement;
    if (elem.requestFullscreen) {
        elem.requestFullscreen();
    }
    else if (elem.mozRequestFullScreen) { /* Firefox */
        elem.mozRequestFullScreen();
    }
    else if (elem.webkitRequestFullscreen) { /* Chrome, Safari and Opera */
        elem.webkitRequestFullscreen();
    }
    else if (elem.msRequestFullscreen) { /* IE/Edge */
        elem.msRequestFullscreen();
    }
}

function setViewport(store) {
    let viewport = document.querySelector("meta[name=viewport]");
    viewport.setAttribute('content', 'width=device-width, initial-scale=' + scale + ', maximum-scale=' + scale);
    if (store === true) {
        try {
            localStorage.setItem('scale-ratio', scale);
        }
        catch(err) {
            logError('Can not save scale-ratio in localStorage: ' + err.message);
        }
    }
}

//eslint-disable-next-line no-unused-vars
function addStream() {
    let streamUriEl = document.getElementById('streamUrl');
    if (validateStream(streamUriEl) === true) {
        sendAPI("MPD_API_QUEUE_ADD_TRACK", {"uri": streamUriEl.value});
        modalAddToPlaylist.hide();
        showNotification(t('Added stream %{streamUri} to queue', {"streamUri": streamUriEl.value}), '', '', 'success');
    }
}

function seekRelativeForward() {
    seekRelative(5);
}

function seekRelativeBackward() {
    seekRelative(-5);
}

function seekRelative(offset) {
    sendAPI("MPD_API_SEEK_CURRENT", {"seek": offset, "relative": true});
}

//eslint-disable-next-line no-unused-vars
function clickPlay() {
    if (playstate !== 'play') {
        sendAPI("MPD_API_PLAYER_PLAY", {});
    }
    else if (settings.footerStop === true) {
        sendAPI("MPD_API_PLAYER_STOP", {});
    }
    else {
        sendAPI("MPD_API_PLAYER_PAUSE", {});
    }
}

//eslint-disable-next-line no-unused-vars
function clickStop() {
    sendAPI("MPD_API_PLAYER_STOP", {});
}

//eslint-disable-next-line no-unused-vars
function clickPrev() {
    sendAPI("MPD_API_PLAYER_PREV", {});
}

//eslint-disable-next-line no-unused-vars
function clickNext() {
    sendAPI("MPD_API_PLAYER_NEXT", {});
}

//eslint-disable-next-line no-unused-vars
function execSyscmd(cmd) {
    sendAPI("MYMPD_API_SYSCMD", {"cmd": cmd});
}

//eslint-disable-next-line no-unused-vars
function clearCovercache() {
    sendAPI("MYMPD_API_COVERCACHE_CLEAR", {});
}

//eslint-disable-next-line no-unused-vars
function cropCovercache() {
    sendAPI("MYMPD_API_COVERCACHE_CROP", {});
}

//eslint-disable-next-line no-unused-vars
function updateDB(uri, showModal) {
    sendAPI("MPD_API_DATABASE_UPDATE", {"uri": uri});
    updateDBstarted(showModal);
}

//eslint-disable-next-line no-unused-vars
function rescanDB(uri, showModal) {
    sendAPI("MPD_API_DATABASE_RESCAN", {"uri": uri});
    updateDBstarted(showModal);
}

function updateDBstarted(showModal) {
    if (showModal === true) {
        document.getElementById('updateDBfinished').innerText = '';
        document.getElementById('updateDBfooter').classList.add('hide');
        let updateDBprogress = document.getElementById('updateDBprogress');
        updateDBprogress.style.width = '20px';
        updateDBprogress.style.marginLeft = '-20px';
        modalUpdateDB.show();
        updateDBprogress.classList.add('updateDBprogressAnimate');
    }

    showNotification(t('Database update started'), '', '', 'success');
}

function updateDBfinished(idleEvent) {
    if (document.getElementById('modalUpdateDB').classList.contains('show')) {
        _updateDBfinished(idleEvent);
    }
    else {
        //on small databases the modal opens after the finish event
        setTimeout(function() {
            _updateDBfinished(idleEvent);
        }, 100);
    }
}

function _updateDBfinished(idleEvent) {
    //spinner in mounts modal
    let el = document.getElementById('spinnerUpdateProgress');
    if (el) {
        let parent = el.parentNode;
        el.remove();
        for (let i = 0; i < parent.children.length; i++) {
            parent.children[i].classList.remove('hide');
        }
    }

    //update database modal
    if (document.getElementById('modalUpdateDB').classList.contains('show')) {
        if (idleEvent === 'update_database') {
            document.getElementById('updateDBfinished').innerText = t('Database successfully updated');
        }
        else if (idleEvent === 'update_finished') {
            document.getElementById('updateDBfinished').innerText = t('Database update finished');
        }
        let updateDBprogress = document.getElementById('updateDBprogress');
        updateDBprogress.classList.remove('updateDBprogressAnimate');
        updateDBprogress.style.width = '100%';
        updateDBprogress.style.marginLeft = '0px';
        document.getElementById('updateDBfooter').classList.remove('hide');
    }

    //general notification
    if (idleEvent === 'update_database') {
        showNotification(t('Database successfully updated'), '', '', 'success');
    }
    else if (idleEvent === 'update_finished') {
        showNotification(t('Database update finished'), '', '', 'success');
    }
}

//eslint-disable-next-line no-unused-vars
function zoomPicture(el) {
    if (el.classList.contains('booklet')) {
        window.open(el.getAttribute('data-href'));
        return;
    }
    
    if (el.classList.contains('carousel')) {
        let imgSrc = el.getAttribute('data-images');
        let images;
        if (imgSrc !== null) {
            images = el.getAttribute('data-images').split(';;');
        }
        else {
            images = lastSongObj.images.slice();
        }
        if (images.length > 0) {
            for (let i = 0; i < images.length; i++) {
                images[i] = subdir + '/browse/music/' + images[i];
            }
            const imgEl = document.getElementById('modalPictureImg');
            imgEl.style.paddingTop = 0;
            createImgCarousel(imgEl, 'picsCarousel', images);
            document.getElementById('modalPictureZoom').classList.add('hide');
            modalPicture.show();
            return;
        }
    }
    
    if (el.style.backgroundImage !== '') {
        const imgEl = document.getElementById('modalPictureImg');
        imgEl.innerHTML = '';
        imgEl.style.paddingTop = '100%';
        imgEl.style.backgroundImage = el.style.backgroundImage;
        document.getElementById('modalPictureZoom').classList.remove('hide');
        modalPicture.show();
    }
}

//eslint-disable-next-line no-unused-vars
function zoomZoomPicture() {
    window.open(document.getElementById('modalPictureImg').style.backgroundImage.match(/^url\(["']?([^"']*)["']?\)/)[1]);
}


function createImgCarousel(imgEl, name, images) {
    let carousel = '<div id="' + name + '" class="carousel slide" data-ride="carousel">' +
        '<ol class="carousel-indicators">';
    for (let i = 0; i < images.length; i++) {
        carousel += '<li data-target="#' + name + '" data-slide-to="' + i + '"' +
            (i === 0 ? ' class="active"' : '') + '></li>';
    }
    carousel += '</ol>' +
        '<div class="carousel-inner" role="listbox">';
    for (let i = 0; i < images.length; i++) {
        carousel += '<div class="carousel-item' + (i === 0 ? ' active' : '') + '"><div></div></div>';
    }
    carousel += '</div>' +
        '<a class="carousel-control-prev" href="#' + name + '" data-slide="prev">' +
            '<span class="carousel-control-prev-icon"></span>' +
        '</a>' +
        '<a class="carousel-control-next" href="#' + name + '" data-slide="next">' +
            '<span class="carousel-control-next-icon"></span>' +
        '</a>' +
        '</div>';
    imgEl.innerHTML = carousel;
    let carouselItems = imgEl.getElementsByClassName('carousel-item');
    for (let i = 0; i < carouselItems.length; i++) {
        carouselItems[i].children[0].style.backgroundImage = 'url("' + encodeURI(images[i]) + '")';
    }
    let myCarousel = document.getElementById(name);
    //eslint-disable-next-line no-undef, no-unused-vars
    let myCarouselInit = new BSN.Carousel(myCarousel, {
        interval: false,
        pause: false
    });
}
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

//eslint-disable-next-line no-unused-vars
function unmountMount(mountPoint) {
    sendAPI("MPD_API_MOUNT_UNMOUNT", {"mountPoint": mountPoint}, showListMounts);
}

//eslint-disable-next-line no-unused-vars
function mountMount() {
    let formOK = true;
    document.getElementById('errorMount').classList.add('hide');
    
    if (formOK === true) {
        sendAPI("MPD_API_MOUNT_MOUNT", {
            "mountUrl": getSelectValue('selectMountUrlhandler') + document.getElementById('inputMountUrl').value,
            "mountPoint": document.getElementById('inputMountPoint').value,
            }, showListMounts, true);
    }
}

//eslint-disable-next-line no-unused-vars
function updateMount(el, uri) {
    let parent = el.parentNode;
    for (let i = 0; i < parent.children.length; i++) {
        parent.children[i].classList.add('hide');
    }
    let spinner = document.createElement('div');
    spinner.setAttribute('id', 'spinnerUpdateProgress');
    spinner.classList.add('spinner-border', 'spinner-border-sm');
    el.parentNode.insertBefore(spinner, el);
    updateDB(uri, false);    
}

//eslint-disable-next-line no-unused-vars
function showEditMount(uri, storage) {
    document.getElementById('listMounts').classList.remove('active');
    document.getElementById('editMount').classList.add('active');
    document.getElementById('listMountsFooter').classList.add('hide');
    document.getElementById('editMountFooter').classList.remove('hide');
    document.getElementById('errorMount').classList.add('hide');

    let c = uri.match(/^(\w+:\/\/)(.+)$/);
    if (c !== null && c.length > 2) {
        document.getElementById('selectMountUrlhandler').value = c[1];
        document.getElementById('inputMountUrl').value = c[2];
        document.getElementById('inputMountPoint').value = storage;
    }
    else {
        document.getElementById('inputMountUrl').value = '';
        document.getElementById('inputMountPoint').value = '';
    }
    document.getElementById('inputMountUrl').focus();
    document.getElementById('inputMountUrl').classList.remove('is-invalid');
    document.getElementById('inputMountPoint').classList.remove('is-invalid');
}

function showListMounts(obj) {
    if (obj && obj.error && obj.error.message) {
        let emEl = document.getElementById('errorMount');
        emEl.innerText = obj.error.message;
        emEl.classList.remove('hide');
        return;
    }
    document.getElementById('listMounts').classList.add('active');
    document.getElementById('editMount').classList.remove('active');
    document.getElementById('listMountsFooter').classList.remove('hide');
    document.getElementById('editMountFooter').classList.add('hide');
    sendAPI("MPD_API_MOUNT_LIST", {}, parseListMounts);
}

function parseListMounts(obj) {
    let tbody = document.getElementById('listMounts').getElementsByTagName('tbody')[0];
    let tr = tbody.getElementsByTagName('tr');
    
    let activeRow = 0;
    for (let i = 0; i < obj.result.returnedEntities; i++) {
        let row = document.createElement('tr');
        row.setAttribute('data-url', encodeURI(obj.result.data[i].mountUrl));
        row.setAttribute('data-point', encodeURI(obj.result.data[i].mountPoint));
        if (obj.result.data[i].mountPoint === '') {
            row.classList.add('not-clickable');
        }
        let tds = '<td>' + (obj.result.data[i].mountPoint === '' ? '<span class="material-icons">home</span>' : e(obj.result.data[i].mountPoint)) + '</td>' +
                  '<td>' + e(obj.result.data[i].mountUrl) + '</td>';
        if (obj.result.data[i].mountPoint !== '') {
            tds += '<td data-col="Action">' + 
                   '<a href="#" title="' + t('Unmount') + '" data-action="unmount" class="material-icons color-darkgrey">delete</a>' +
                   '<a href="#" title="' + t('Update') + '" data-action="update"class="material-icons color-darkgrey">refresh</a>' +
                   '</td>';
        }
        else {
            tds += '<td>&nbsp;</td>';
        }
        row.innerHTML = tds;
        if (i < tr.length) {
            activeRow = replaceTblRow(tr[i], row) === true ? i : activeRow;
        }
        else {
            tbody.append(row);
        }
    }
    let trLen = tr.length - 1;
    for (let i = trLen; i >= obj.result.returnedEntities; i --) {
        tr[i].remove();
    }

    if (obj.result.returnedEntities === 0) {
        tbody.innerHTML = '<tr><td><span class="material-icons">error_outline</span></td>' +
                          '<td colspan="4">' + t('Empty list') + '</td></tr>';
    }     
}

function parseNeighbors(obj) {
    let list = '';
    if (obj.error) {
        list = '<div class="list-group-item"><span class="material-icons">error_outline</span> ' + t(obj.error.message) + '</div>';
    }
    else {
        for (let i = 0; i < obj.result.returnedEntities; i++) {
            list += '<a href="#" class="list-group-item list-group-item-action" data-value="' + obj.result.data[i].uri + '">' + 
                    obj.result.data[i].uri + '<br/><small>' + obj.result.data[i].displayName + '</small></a>';
        }    
        if (obj.result.returnedEntities === 0) {
            list = '<div class="list-group-item"><span class="material-icons">error_outline</span>&nbsp;' + t('Empty list') + '</div>';
        }
    }
    document.getElementById('dropdownNeighbors').children[0].innerHTML = list;
}
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/* Disable eslint warnings */
/* global BSN, phrases, locales */

var socket = null;
var websocketConnected = false;
var websocketTimer = null;
var socketRetry = 0;

var lastSong = '';
var lastSongObj = {};
var lastState;
var currentSong = {};
var playstate = '';
var settingsLock = false;
var settingsParsed = false;
var settingsNew = {};
var settings = {};
settings.loglevel = 2;
var alertTimeout = null;
var progressTimer = null;
var deferredA2HSprompt;
var dragSrc;
var dragEl;

var appInited = false;
var subdir = '';
var uiEnabled = true;
var locale = navigator.language || navigator.userLanguage;
var scale = '1.0';
var isMobile = /iPhone|iPad|iPod|Android/i.test(navigator.userAgent);
var ligatureMore = 'menu';
var progressBarTransition = 'width 1s linear';

var app = {};
app.apps = { 
    "Home": { 
        "page": 0,
        "filter": "-",
        "sort": "-",
        "tag": "-",
        "search": "",
        "scrollPos": 0
    },
    "Playback": { 
        "page": 0,
        "filter": "-",
        "sort": "-",
        "tag": "-",
        "search": "",
        "scrollPos": 0
    },
    "Queue": {
        "active": "Current",
        "tabs": { 
            "Current": { 
                "page": 0,
                "filter": "any",
                "sort": "-",
                "tag": "-",
                "search": "",
                "scrollPos": 0 
            },
            "LastPlayed": {
                "page": 0,
                "filter": "any",
                "sort": "-",
                "tag": "-",
                "search": "",
                "scrollPos": 0 
            },
            "Jukebox": {
                "page": 0,
                "filter": "any",
                "sort": "-",
                "tag": "-",
                "search": "",
                "scrollPos": 0 
            }
        }
    },
    "Browse": { 
        "active": "Database", 
        "tabs":  { 
            "Filesystem": { 
                "page": 0,
                "filter": "-",
                "sort": "-",
                "tag": "-",
                "search": "",
                "scrollPos": 0 
            },
            "Playlists": { 
                "active": "All",
                "views": { 
                    "All": {
                        "page": 0,
                        "filter": "-",
                        "sort": "-",
                        "tag": "-",
                        "search": "", 
                        "scrollPos": 0 
                    },
                    "Detail": {
                        "page": 0,
                        "filter": "-",
                        "sort": "-",
                        "tag": "-",
                        "search": "",
                        "scrollPos": 0
                    }
                }
            },
            "Database": { 
                "active": "List",
                "views": { 
                    "List": { 
                        "page": 0,
                        "filter": "AlbumArtist",
                        "sort": "AlbumArtist",
                        "tag": "Album",
                        "search": "",
                        "scrollPos": 0
                    },
                    "Detail": { 
                        "page": 0,
                        "filter": "-",
                        "sort": "-",
                        "tag": "-",
                        "search": "",
                        "scrollPos": 0
                    }
                }
            }
        }
    },
    "Search": { 
        "page": 0,
        "filter": "any",
        "sort": "-",
        "tag": "-",
        "search": "",
        "scrollPos": 0
    }
};

app.current = { "app": "Home", "tab": undefined, "view": undefined, "page": 0, "filter": "", "search": "", "sort": "", "tag": "", "scrollPos": 0 };
app.last = { "app": undefined, "tab": undefined, "view": undefined, "filter": "", "search": "", "sort": "", "tag": "", "scrollPos": 0 };

var domCache = {};
domCache.navbarBtns = document.getElementById('navbar-main').getElementsByTagName('div');
domCache.navbarBtnsLen = domCache.navbarBtns.length;
domCache.counter = document.getElementById('counter');
domCache.volumePrct = document.getElementById('volumePrct');
domCache.volumeControl = document.getElementById('volumeControl');
domCache.volumeMenu = document.getElementById('volumeMenu');
domCache.btnsPlay = document.getElementsByClassName('btnPlay');
domCache.btnsPlayLen = domCache.btnsPlay.length;
domCache.btnPrev = document.getElementById('btnPrev');
domCache.btnNext = document.getElementById('btnNext');
domCache.progress = document.getElementById('footerProgress');
domCache.progressBar = document.getElementById('footerProgressBar');
domCache.progressPos = document.getElementById('footerProgressPos')
domCache.volumeBar = document.getElementById('volumeBar');
domCache.outputs = document.getElementById('outputs');
domCache.btnA2HS = document.getElementById('nav-add2homescreen');
domCache.currentCover = document.getElementById('currentCover');
domCache.currentTitle = document.getElementById('currentTitle');
domCache.footerTitle = document.getElementById('footerTitle');
domCache.footerArtist = document.getElementById('footerArtist');
domCache.footerAlbum = document.getElementById('footerAlbum');
domCache.footerCover = document.getElementById('footerCover');
domCache.btnVoteUp = document.getElementById('btnVoteUp');
domCache.btnVoteDown = document.getElementById('btnVoteDown');
domCache.badgeQueueItems = document.getElementById('badgeQueueItems');
domCache.searchstr = document.getElementById('searchstr');
domCache.searchCrumb = document.getElementById('searchCrumb');
domCache.body = document.getElementsByTagName('body')[0];
domCache.footer = document.getElementsByTagName('footer')[0];
domCache.header = document.getElementById('header');
domCache.mainMenu = document.getElementById('mainMenu');

/* eslint-disable no-unused-vars */
var modalConnection = new BSN.Modal(document.getElementById('modalConnection'));
var modalSettings = new BSN.Modal(document.getElementById('modalSettings'));
var modalAbout = new BSN.Modal(document.getElementById('modalAbout')); 
var modalSaveQueue = new BSN.Modal(document.getElementById('modalSaveQueue'));
var modalAddToQueue = new BSN.Modal(document.getElementById('modalAddToQueue'));
var modalSongDetails = new BSN.Modal(document.getElementById('modalSongDetails'));
var modalAddToPlaylist = new BSN.Modal(document.getElementById('modalAddToPlaylist'));
var modalRenamePlaylist = new BSN.Modal(document.getElementById('modalRenamePlaylist'));
var modalUpdateDB = new BSN.Modal(document.getElementById('modalUpdateDB'));
var modalSaveSmartPlaylist = new BSN.Modal(document.getElementById('modalSaveSmartPlaylist'));
var modalDeletePlaylist = new BSN.Modal(document.getElementById('modalDeletePlaylist'));
var modalSaveBookmark = new BSN.Modal(document.getElementById('modalSaveBookmark'));
var modalTimer = new BSN.Modal(document.getElementById('modalTimer'));
var modalMounts = new BSN.Modal(document.getElementById('modalMounts'));
var modalExecScript = new BSN.Modal(document.getElementById('modalExecScript'));
var modalScripts = new BSN.Modal(document.getElementById('modalScripts'));
var modalPartitions = new BSN.Modal(document.getElementById('modalPartitions'));
var modalPartitionOutputs = new BSN.Modal(document.getElementById('modalPartitionOutputs'));
var modalTrigger = new BSN.Modal(document.getElementById('modalTrigger'));
var modalOutputAttributes = new BSN.Modal(document.getElementById('modalOutputAttributes'));
var modalPicture = new BSN.Modal(document.getElementById('modalPicture'));
var modalEditHomeIcon = new BSN.Modal(document.getElementById('modalEditHomeIcon'));

var dropdownMainMenu = new BSN.Dropdown(document.getElementById('mainMenu'));
var dropdownVolumeMenu = new BSN.Dropdown(document.getElementById('volumeMenu'));
var dropdownBookmarks = new BSN.Dropdown(document.getElementById('BrowseFilesystemBookmark'));
var dropdownLocalPlayer = new BSN.Dropdown(document.getElementById('localPlaybackMenu'));
var dropdownPlay = new BSN.Dropdown(document.getElementById('btnPlayDropdown'));
var dropdownDatabaseSort = new BSN.Dropdown(document.getElementById('btnDatabaseSortDropdown'));
var dropdownNeighbors = new BSN.Dropdown(document.getElementById('btnDropdownNeighbors'));

var collapseDBupdate = new BSN.Collapse(document.getElementById('navDBupdate'));
var collapseSettings = new BSN.Collapse(document.getElementById('navSettings'));
var collapseSyscmds = new BSN.Collapse(document.getElementById('navSyscmds'));
var collapseScripting = new BSN.Collapse(document.getElementById('navScripting'));
var collapseJukeboxMode = new BSN.Collapse(document.getElementById('labelJukeboxMode'));
/* eslint-enable no-unused-vars */

function appPrepare(scrollPos) {
    if (app.current.app !== app.last.app || app.current.tab !== app.last.tab || app.current.view !== app.last.view) {
        //Hide all cards + nav
        for (let i = 0; i < domCache.navbarBtnsLen; i++) {
            domCache.navbarBtns[i].classList.remove('active');
        }
        document.getElementById('cardHome').classList.add('hide');
        document.getElementById('cardPlayback').classList.add('hide');
        document.getElementById('cardQueue').classList.add('hide');
        document.getElementById('cardBrowse').classList.add('hide');
        document.getElementById('cardSearch').classList.add('hide');
        document.getElementById('cardQueueCurrent').classList.add('hide');
        document.getElementById('cardQueueLastPlayed').classList.add('hide');
        document.getElementById('cardQueueJukebox').classList.add('hide');
        document.getElementById('cardBrowsePlaylists').classList.add('hide');
        document.getElementById('cardBrowseFilesystem').classList.add('hide');
        document.getElementById('cardBrowseDatabase').classList.add('hide');
        //show active card + nav
        document.getElementById('card' + app.current.app).classList.remove('hide');
        if (document.getElementById('nav' + app.current.app)) {
            document.getElementById('nav' + app.current.app).classList.add('active');
        }
        if (app.current.tab !== undefined) {
            document.getElementById('card' + app.current.app + app.current.tab).classList.remove('hide');
        }
        scrollToPosY(scrollPos);
    }
    let list = document.getElementById(app.current.app + 
        (app.current.tab === undefined ? '' : app.current.tab) + 
        (app.current.view === undefined ? '' : app.current.view) + 'List');
    if (list) {
        list.classList.add('opacity05');
    }
}

function appGoto(card, tab, view, page, filter, sort, tag, search) {
    let scrollPos = 0;
    if (document.body.scrollTop) {
        scrollPos = document.body.scrollTop
    }
    else {
        scrollPos = document.documentElement.scrollTop;
    }
        
    if (app.apps[app.current.app].scrollPos !== undefined) {
        app.apps[app.current.app].scrollPos = scrollPos
    }
    else if (app.apps[app.current.app].tabs[app.current.tab].scrollPos !== undefined) {
        app.apps[app.current.app].tabs[app.current.tab].scrollPos = scrollPos
    }
    else if (app.apps[app.current.app].tabs[app.current.tab].views[app.current.view].scrollPos !== undefined) {
        app.apps[app.current.app].tabs[app.current.tab].views[app.current.view].scrollPos = scrollPos;
    }

    let hash = '';
    if (app.apps[card].tabs) {
        if (tab === undefined) {
            tab = app.apps[card].active;
        }
        if (app.apps[card].tabs[tab].views) {
            if (view === undefined) {
                view = app.apps[card].tabs[tab].active;
            }
            hash = '/' + encodeURIComponent(card) + '/' + encodeURIComponent(tab) + '/' + encodeURIComponent(view) + '!' + 
                encodeURIComponent(page === undefined ? app.apps[card].tabs[tab].views[view].page : page) + '/' +
                encodeURIComponent(filter === undefined ? app.apps[card].tabs[tab].views[view].filter : filter) + '/' +
                encodeURIComponent(sort === undefined ? app.apps[card].tabs[tab].views[view].sort : sort) + '/' +
                encodeURIComponent(tag === undefined ? app.apps[card].tabs[tab].views[view].tag : tag) + '/' +
                encodeURIComponent(search === undefined ? app.apps[card].tabs[tab].views[view].search : search);
        }
        else {
            hash = '/' + encodeURIComponent(card) + '/' + encodeURIComponent(tab) + '!' + 
                encodeURIComponent(page === undefined ? app.apps[card].tabs[tab].page : page) + '/' +
                encodeURIComponent(filter === undefined ? app.apps[card].tabs[tab].filter : filter) + '/' +
                encodeURIComponent(sort === undefined ? app.apps[card].tabs[tab].sort : sort) + '/' +
                encodeURIComponent(tag === undefined ? app.apps[card].tabs[tab].tag : tag) + '/' +
                encodeURIComponent(search === undefined ? app.apps[card].tabs[tab].search : search);
        }
    }
    else {
        hash = '/' + encodeURIComponent(card) + '!' + 
            encodeURIComponent(page === undefined ? app.apps[card].page : page) + '/' +
            encodeURIComponent(filter === undefined ? app.apps[card].filter : filter) + '/' +
            encodeURIComponent(sort === undefined ? app.apps[card].sort : sort) + '/' +
            encodeURIComponent(tag === undefined ? app.apps[card].tag : tag) + '/' +
            encodeURIComponent(search === undefined ? app.apps[card].search : search);
    }
    location.hash = hash;
}

function appRoute() {
    if (settingsParsed === false) {
        appInitStart();
        return;
    }
    let hash = location.hash;
    let params = hash.match(/^#\/(\w+)\/?(\w+)?\/?(\w+)?!(\d+)\/([^/]+)\/([^/]+)\/([^/]+)\/(.*)$/);
    if (params) {
        app.current.app = decodeURIComponent(params[1]);
        app.current.tab = params[2] !== undefined ? decodeURIComponent(params[2]) : undefined;
        app.current.view = params[3] !== undefined ? decodeURIComponent(params[3]) : undefined;
        app.current.page = decodeURIComponent(parseInt(params[4]));
        app.current.filter = decodeURIComponent(params[5]);
        app.current.sort = decodeURIComponent(params[6]);
        app.current.tag = decodeURIComponent(params[7]);
        app.current.search = decodeURIComponent(params[8]);
        
        if (app.apps[app.current.app].page !== undefined) {
            app.apps[app.current.app].page = app.current.page;
            app.apps[app.current.app].filter = app.current.filter;
            app.apps[app.current.app].sort = app.current.sort;
            app.apps[app.current.app].tag = app.current.tag;
            app.apps[app.current.app].search = app.current.search;
            app.current.scrollPos = app.apps[app.current.app].scrollPos;
        }
        else if (app.apps[app.current.app].tabs[app.current.tab].page !== undefined) {
            app.apps[app.current.app].tabs[app.current.tab].page = app.current.page;
            app.apps[app.current.app].tabs[app.current.tab].filter = app.current.filter;
            app.apps[app.current.app].tabs[app.current.tab].sort = app.current.sort;
            app.apps[app.current.app].tabs[app.current.tab].tag = app.current.tag;
            app.apps[app.current.app].tabs[app.current.tab].search = app.current.search;
            app.apps[app.current.app].active = app.current.tab;
            app.current.scrollPos = app.apps[app.current.app].tabs[app.current.tab].scrollPos;
        }
        else if (app.apps[app.current.app].tabs[app.current.tab].views[app.current.view].page !== undefined) {
            app.apps[app.current.app].tabs[app.current.tab].views[app.current.view].page = app.current.page;
            app.apps[app.current.app].tabs[app.current.tab].views[app.current.view].filter = app.current.filter;
            app.apps[app.current.app].tabs[app.current.tab].views[app.current.view].sort = app.current.sort;
            app.apps[app.current.app].tabs[app.current.tab].views[app.current.view].tag = app.current.tag;
            app.apps[app.current.app].tabs[app.current.tab].views[app.current.view].search = app.current.search;
            app.apps[app.current.app].active = app.current.tab;
            app.apps[app.current.app].tabs[app.current.tab].active = app.current.view;
            app.current.scrollPos = app.apps[app.current.app].tabs[app.current.tab].views[app.current.view].scrollPos;
        }
    }
    else {
        appPrepare(0);
        if (settings.featHome === true) {
            appGoto('Home');
        }
        else {
            appGoto('Playback');
        }
        return;
    }

    appPrepare(app.current.scrollPos);

    if (app.current.app === 'Home') {
        sendAPI("MYMPD_API_HOME_LIST", {}, parseHome);
    }
    else if (app.current.app === 'Playback') {
        sendAPI("MPD_API_PLAYER_CURRENT_SONG", {}, songChange);
    }    
    else if (app.current.app === 'Queue' && app.current.tab === 'Current' ) {
        selectTag('searchqueuetags', 'searchqueuetagsdesc', app.current.filter);
        getQueue();
    }
    else if (app.current.app === 'Queue' && app.current.tab === 'LastPlayed') {
        sendAPI("MPD_API_QUEUE_LAST_PLAYED", {"offset": app.current.page, "cols": settings.colsQueueLastPlayed}, parseLastPlayed);
    }
    else if (app.current.app === 'Queue' && app.current.tab === 'Jukebox') {
        sendAPI("MPD_API_JUKEBOX_LIST", {"offset": app.current.page, "cols": settings.colsQueueJukebox}, parseJukeboxList);
    }
    else if (app.current.app === 'Browse' && app.current.tab === 'Playlists' && app.current.view === 'All') {
        sendAPI("MPD_API_PLAYLIST_LIST", {"offset": app.current.page, "searchstr": app.current.search}, parsePlaylists);
        const searchPlaylistsStrEl = document.getElementById('searchPlaylistsStr');
        if (searchPlaylistsStrEl.value === '' && app.current.search !== '') {
            searchPlaylistsStrEl.value = app.current.search;
        }
    }
    else if (app.current.app === 'Browse' && app.current.tab === 'Playlists' && app.current.view === 'Detail') {
        sendAPI("MPD_API_PLAYLIST_CONTENT_LIST", {"offset": app.current.page, "searchstr": app.current.search, "uri": app.current.filter, "cols": settings.colsBrowsePlaylistsDetail}, parsePlaylists);
        const searchPlaylistsStrEl = document.getElementById('searchPlaylistsStr');
        if (searchPlaylistsStrEl.value === '' && app.current.search !== '') {
            searchPlaylistsStrEl.value = app.current.search;
        }
    }    
    else if (app.current.app === 'Browse' && app.current.tab === 'Filesystem') {
        sendAPI("MPD_API_DATABASE_FILESYSTEM_LIST", {"offset": app.current.page, "path": (app.current.search ? app.current.search : "/"), 
            "searchstr": (app.current.filter !== '-' ? app.current.filter : ''), "cols": settings.colsBrowseFilesystem}, parseFilesystem, true);
        // Don't add all songs from root
        if (app.current.search) {
            document.getElementById('BrowseFilesystemAddAllSongs').removeAttribute('disabled');
            document.getElementById('BrowseFilesystemAddAllSongsBtn').removeAttribute('disabled');
        }
        else {
            document.getElementById('BrowseFilesystemAddAllSongs').setAttribute('disabled', 'disabled');
            document.getElementById('BrowseFilesystemAddAllSongsBtn').setAttribute('disabled', 'disabled');
        }
        // Create breadcrumb
        let breadcrumbs='<li class="breadcrumb-item"><a data-uri="" class="text-body material-icons">home</a></li>';
        let pathArray = app.current.search.split('/');
        let pathArrayLen = pathArray.length;
        let fullPath = '';
        for (let i = 0; i < pathArrayLen; i++) {
            if (pathArrayLen - 1 === i) {
                breadcrumbs += '<li class="breadcrumb-item active">' + e(pathArray[i]) + '</li>';
                break;
            }
            fullPath += pathArray[i];
            breadcrumbs += '<li class="breadcrumb-item"><a class="text-body" href="#" data-uri="' + encodeURI(fullPath) + '">' + e(pathArray[i]) + '</a></li>';
            fullPath += '/';
        }
        document.getElementById('BrowseBreadcrumb').innerHTML = breadcrumbs;
        const searchFilesystemStrEl = document.getElementById('searchFilesystemStr');
        if (searchFilesystemStrEl.value === '' && app.current.filter !== '-') {
            searchFilesystemStrEl.value = app.current.filter;
        }
    }
    else if (app.current.app === 'Browse' && app.current.tab === 'Database' && app.current.view === 'List') {
        document.getElementById('viewListDatabase').classList.remove('hide');
        document.getElementById('viewDetailDatabase').classList.add('hide');
        selectTag('searchDatabaseTags', 'searchDatabaseTagsDesc', app.current.filter);
        selectTag('BrowseDatabaseByTagDropdown', 'btnBrowseDatabaseByTagDesc', app.current.tag);
        let sort = app.current.sort;
        let sortdesc = false;
        if (app.current.sort.charAt(0) === '-') {
            sortdesc = true;
            sort = app.current.sort.substr(1);
            toggleBtnChk('databaseSortDesc', true);
        }
        else {
            toggleBtnChk('databaseSortDesc', false);
        }
        selectTag('databaseSortTags', undefined, sort);
        if (app.current.tag === 'Album') {
            const crumbEl = document.getElementById('searchDatabaseCrumb');
            const searchEl = document.getElementById('searchDatabaseStr');
            
            let crumbs = '';
            let elements = app.current.search.split(' AND ');
            for (let i = 0; i < elements.length - 1 ; i++) {
                let value = elements[i].substring(1, elements[i].length - 1);
                crumbs += '<button data-filter="' + encodeURI(value) + '" class="btn btn-light mr-2">' + e(value) + '<span class="badge badge-secondary">&times</span></button>';
            }
            crumbEl.innerHTML = crumbs;
            if (searchEl.value === '' && elements.length >= 1) {
                let lastEl = elements[elements.length - 1].substring(1,  elements[elements.length - 1].length - 1);
                let lastElValue = lastEl.substring(lastEl.indexOf('\'') + 1, lastEl.length - 1);
                if (searchEl.value !== lastElValue) {
                    crumbEl.innerHTML += '<button data-filter="' + encodeURI(lastEl) +'" class="btn btn-light mr-2">' + e(lastEl) + '<span class="badge badge-secondary">&times;</span></button>';
                }
                document.getElementById('searchDatabaseMatch').value = 'contains';
                //selectTag('searchDatabaseTags', 'searchDatabaseTagsDesc', 'Album');
            }
            crumbEl.classList.remove('hide');
            document.getElementById('searchDatabaseMatch').classList.remove('hide');
            document.getElementById('btnDatabaseSortDropdown').removeAttribute('disabled');
            document.getElementById('btnDatabaseSearchDropdown').removeAttribute('disabled');
            sendAPI("MPD_API_DATABASE_GET_ALBUMS", {"offset": app.current.page, "searchstr": app.current.search, 
                "filter": app.current.filter, "sort": sort, "sortdesc": sortdesc}, parseDatabase);
        }
        else {
            document.getElementById('searchDatabaseCrumb').classList.add('hide');
            document.getElementById('searchDatabaseMatch').classList.add('hide');
            document.getElementById('btnDatabaseSortDropdown').setAttribute('disabled', 'disabled');
            document.getElementById('btnDatabaseSearchDropdown').setAttribute('disabled', 'disabled');
            document.getElementById('searchDatabaseStr').value = app.current.search;
            sendAPI("MPD_API_DATABASE_TAG_LIST", {"offset": app.current.page, "searchstr": app.current.search, 
                "filter": app.current.filter, "sort": sort, "sortdesc": sortdesc, "tag": app.current.tag}, parseDatabase);
        }
    }
    else if (app.current.app === 'Browse' && app.current.tab === 'Database' && app.current.view === 'Detail') {
        document.getElementById('viewListDatabase').classList.add('hide');
        document.getElementById('viewDetailDatabase').classList.remove('hide');
        if (app.current.filter === 'Album') {
            let cols = settings.colsBrowseDatabaseDetail.slice();
            if (cols.includes('Disc') === false) {
                cols.push('Disc');
            }
            sendAPI("MPD_API_DATABASE_TAG_ALBUM_TITLE_LIST", {"album": app.current.tag,
                "searchstr": app.current.search,
                "tag": app.current.sort, "cols": cols}, parseAlbumDetails);
        }    
    }
    else if (app.current.app === 'Search') {
        domCache.searchstr.focus();
        if (settings.featAdvsearch) {
            let crumbs = '';
            let elements = app.current.search.substring(1, app.current.search.length - 1).split(' AND ');
            for (let i = 0; i < elements.length - 1 ; i++) {
                let value = elements[i].substring(1, elements[i].length - 1);
                crumbs += '<button data-filter="' + encodeURI(value) + '" class="btn btn-light mr-2">' + e(value) + '<span class="badge badge-secondary">&times</span></button>';
            }
            domCache.searchCrumb.innerHTML = crumbs;
            if (domCache.searchstr.value === '' && elements.length >= 1) {
                let lastEl = elements[elements.length - 1].substring(1,  elements[elements.length - 1].length - 1);
                let lastElValue = lastEl.substring(lastEl.indexOf('\'') + 1, lastEl.length - 1);
                if (domCache.searchstr.value !== lastElValue) {
                    domCache.searchCrumb.innerHTML += '<button data-filter="' + encodeURI(lastEl) +'" class="btn btn-light mr-2">' + e(lastEl) + '<span class="ml-2 badge badge-secondary">&times;</span></button>';
                }
                let match = lastEl.substring(lastEl.indexOf(' ') + 1);
                match = match.substring(0, match.indexOf(' '));
                if (match === '') {
                    match = 'contains';
                }
                document.getElementById('searchMatch').value = match;
            }
        }
        else {
            if (domCache.searchstr.value === '' && app.current.search !== '') {
                domCache.searchstr.value = app.current.search;
            }
        }
        if (app.last.app !== app.current.app) {
            if (app.current.search !== '') {
                let colspan = settings['cols' + app.current.app].length;
                colspan--;
                document.getElementById('SearchList').getElementsByTagName('tbody')[0].innerHTML=
                    '<tr><td><span class="material-icons">search</span></td>' +
                    '<td colspan="' + colspan + '">' + t('Searching...') + '</td></tr>';
            }
        }

        if (domCache.searchstr.value.length >= 2 || domCache.searchCrumb.children.length > 0) {
            if (settings.featAdvsearch) {
                let sort = app.current.sort;
                let sortdesc = false;
                if (sort === '-') {
                    if (settings.tags.includes('Title')) {
                        sort = 'Title';
                    }
                    else {
                        sort = '-';
                    }
                    document.getElementById('SearchList').setAttribute('data-sort', sort);
                }
                else {
                    if (sort.indexOf('-') === 0) {
                        sortdesc = true;
                        sort = sort.substring(1);
                    }
                }
                sendAPI("MPD_API_DATABASE_SEARCH_ADV", {"plist": "", "offset": app.current.page, "sort": sort, "sortdesc": sortdesc, "expression": app.current.search, "cols": settings.colsSearch, "replace": false}, parseSearch);
            }
            else {
                sendAPI("MPD_API_DATABASE_SEARCH", {"plist": "", "offset": app.current.page, "filter": app.current.filter, "searchstr": app.current.search, "cols": settings.colsSearch, "replace": false}, parseSearch);
            }
        } else {
            document.getElementById('SearchList').getElementsByTagName('tbody')[0].innerHTML = '';
            document.getElementById('searchAddAllSongs').setAttribute('disabled', 'disabled');
            document.getElementById('searchAddAllSongsBtn').setAttribute('disabled', 'disabled');
            document.getElementById('SearchList').classList.remove('opacity05');
            setPagination(0, 0);
        }
        selectTag('searchtags', 'searchtagsdesc', app.current.filter);
    }
    else {
        appGoto("Home");
    }

    app.last.app = app.current.app;
    app.last.tab = app.current.tab;
    app.last.view = app.current.view;
}

function showAppInitAlert(text) {
    document.getElementById('splashScreenAlert').innerHTML = '<p class="text-danger">' + t(text) + '</p>' +
        '<p><a id="appReloadBtn" class="btn btn-danger text-light clickable">' + t('Reload') + '</a></p>';
    document.getElementById('appReloadBtn').addEventListener('click', function() {
        clearAndReload();
    }, false);
}


function clearAndReload() {
    if ('serviceWorker' in navigator) {
        caches.keys().then(function(cacheNames) {
            cacheNames.forEach(function(cacheName) {
                caches.delete(cacheName);
            });
        });
    }
    location.reload();
}

function a2hsInit() {
    window.addEventListener('beforeinstallprompt', function(event) {
        logDebug('Event: beforeinstallprompt');
        // Prevent Chrome 67 and earlier from automatically showing the prompt
        event.preventDefault();
        // Stash the event so it can be triggered later
        deferredA2HSprompt = event;
        // Update UI notify the user they can add to home screen
        domCache.btnA2HS.classList.remove('hide');
    });

    domCache.btnA2HS.addEventListener('click', function() {
        // Hide our user interface that shows our A2HS button
        domCache.btnA2HS.classList.add('hide');
        // Show the prompt
        deferredA2HSprompt.prompt();
        // Wait for the user to respond to the prompt
        deferredA2HSprompt.userChoice.then((choiceResult) => {
            choiceResult.outcome === 'accepted' ? logDebug('User accepted the A2HS prompt') : logDebug('User dismissed the A2HS prompt');
            deferredA2HSprompt = null;
        });
    });
    
    window.addEventListener('appinstalled', function() {
        logInfo('myMPD installed as app');
        showNotification(t('myMPD installed as app'), '', '', 'success');
    });
}

function appInitStart() {
    //add app routing event handler
    window.addEventListener('hashchange', appRoute, false);

    //set initial scale
    if (isMobile === true) {
        scale = localStorage.getItem('scale-ratio');
        if (scale === null) {
            scale = '1.0';
        }
        setViewport(false);
    }
    else {
        let m = document.getElementsByClassName('featMobile');
        for (let i = 0; i < m.length; i++) {
            m[i].classList.add('hide');
        }        
    }

    subdir = window.location.pathname.replace('/index.html', '').replace(/\/$/, '');
    let localeList = '<option value="default" data-phrase="Browser default"></option>';
    for (let i = 0; i < locales.length; i++) {
        localeList += '<option value="' + e(locales[i].code) + '">' + e(locales[i].desc) + ' (' + e(locales[i].code) + ')</option>';
    }
    document.getElementById('selectLocale').innerHTML = localeList;
    
    i18nHtml(document.getElementById('splashScreenAlert'));
    
    //set loglevel
    let script = document.getElementsByTagName("script")[0].src.replace(/^.*[/]/, '');
    if (script !== 'combined.js') {
        settings.loglevel = 4;
    }
    //register serviceworker
    if ('serviceWorker' in navigator && window.location.protocol === 'https:' 
        && window.location.hostname !== 'localhost' && script === 'combined.js')
    {
        window.addEventListener('load', function() {
            navigator.serviceWorker.register('/sw.js', {scope: '/'}).then(function(registration) {
                // Registration was successful
                logInfo('ServiceWorker registration successful.');
                registration.update();
            }, function(err) {
                // Registration failed
                logError('ServiceWorker registration failed: ' + err);
            });
        });
    }

    appInited = false;
    document.getElementById('splashScreen').classList.remove('hide');
    document.getElementsByTagName('body')[0].classList.add('overflow-hidden');
    document.getElementById('splashScreenAlert').innerText = t('Fetch myMPD settings');

    a2hsInit();

    getSettings(true);
    appInitWait();
}

function appInitWait() {
    setTimeout(function() {
        if (settingsParsed === 'true' && websocketConnected === true) {
            //app initialized
            document.getElementById('splashScreenAlert').innerText = t('Applying settings');
            document.getElementById('splashScreen').classList.add('hide-fade');
            setTimeout(function() {
                document.getElementById('splashScreen').classList.add('hide');
                document.getElementById('splashScreen').classList.remove('hide-fade');
                document.getElementsByTagName('body')[0].classList.remove('overflow-hidden');
            }, 500);
            appInit();
            appInited = true;
            return;
        }
        
        if (settingsParsed === 'true') {
            //parsed settings, now its save to connect to websocket
            document.getElementById('splashScreenAlert').innerText = t('Connect to websocket');
            webSocketConnect();
        }
        else if (settingsParsed === 'error') {
            return;
        }
        appInitWait();
    }, 500);
}

function appInit() {
    document.getElementById('btnChVolumeDown').addEventListener('click', function(event) {
        event.stopPropagation();
    }, false);
    document.getElementById('btnChVolumeUp').addEventListener('click', function(event) {
        event.stopPropagation();
    }, false);

    domCache.volumeBar.addEventListener('click', function(event) {
        event.stopPropagation();
    }, false);
    domCache.volumeBar.addEventListener('change', function() {
        sendAPI("MPD_API_PLAYER_VOLUME_SET", {"volume": domCache.volumeBar.value});
    }, false);

    domCache.progress.addEventListener('click', function(event) {
        if (currentSong && currentSong.currentSongId >= 0 && currentSong.totalTime > 0) {
            domCache.progressBar.style.transition = 'none';
            domCache.progressBar.style.width = event.clientX + 'px';
            setTimeout(function() {
                domCache.progressBar.style.transition = progressBarTransition;
            }, 10)
            const seekVal = Math.ceil((currentSong.totalTime * event.clientX) / event.target.offsetWidth);
            sendAPI("MPD_API_PLAYER_SEEK", {"songid": currentSong.currentSongId, "seek": seekVal});
        }
    }, false);
    domCache.progress.addEventListener('mousemove', function(event) {
        if ((playstate === 'pause' || playstate === 'play') && currentSong.totalTime > 0) {
            domCache.progressPos.innerText = beautifySongDuration(Math.ceil((currentSong.totalTime / event.target.offsetWidth) * event.clientX));
            domCache.progressPos.style.display = 'block';
            const w = domCache.progressPos.offsetWidth / 2;
            const posX = event.clientX < w ? event.clientX : (event.clientX < window.innerWidth - w ? event.clientX - w : event.clientX - (w * 2));
            domCache.progressPos.style.left = posX + 'px';
        }
    }, false);
    domCache.progress.addEventListener('mouseout', function() {
        domCache.progressPos.style.display = 'none';
    }, false);

    let collapseArrows = document.querySelectorAll('.subMenu');
    let collapseArrowsLen = collapseArrows.length;
    for (let i = 0; i < collapseArrowsLen; i++) {
        collapseArrows[i].addEventListener('click', function(event) {
            event.stopPropagation();
            event.preventDefault();
            let icon = this.getElementsByTagName('span')[0];
            icon.innerText = icon.innerText === 'keyboard_arrow_right' ? 'keyboard_arrow_down' : 'keyboard_arrow_right';
        }, false);
    }    
    
    document.getElementById('volumeMenu').parentNode.addEventListener('show.bs.dropdown', function () {
        sendAPI("MPD_API_PLAYER_OUTPUT_LIST", {}, parseOutputs);
    });
    
    document.getElementById('btnDropdownNeighbors').parentNode.addEventListener('show.bs.dropdown', function () {
        if (settings.featNeighbors === true) {
            sendAPI("MPD_API_MOUNT_NEIGHBOR_LIST", {}, parseNeighbors, true);
        }
        else {
            document.getElementById('dropdownNeighbors').children[0].innerHTML = 
                '<div class="list-group-item"><span class="material-icons">warning</span> ' + t('Neighbors are disabled') + '</div>';
        }
    });
    
    document.getElementById('dropdownNeighbors').children[0].addEventListener('click', function (event) {
        event.preventDefault();
        if (event.target.nodeName === 'A') {
            let c = event.target.getAttribute('data-value').match(/^(\w+:\/\/)(.+)$/);
            document.getElementById('selectMountUrlhandler').value = c[1];
            document.getElementById('inputMountUrl').value = c[2];
        }
    });
    
    document.getElementById('BrowseFilesystemBookmark').parentNode.addEventListener('show.bs.dropdown', function () {
        sendAPI("MYMPD_API_BOOKMARK_LIST", {"offset": 0}, parseBookmarks);
    });
    
    document.getElementById('playDropdown').parentNode.addEventListener('show.bs.dropdown', function () {
        showPlayDropdown();
    });

    document.getElementById('playDropdown').addEventListener('click', function (event) {
        event.preventDefault();
        event.stopPropagation();
    });
    
    let dropdowns = document.querySelectorAll('.dropdown-toggle');
    for (let i = 0; i < dropdowns.length; i++) {
        dropdowns[i].parentNode.addEventListener('show.bs.dropdown', function () {
            alignDropdown(this);
        });
    }
    
    document.getElementById('modalTimer').addEventListener('shown.bs.modal', function () {
        showListTimer();
    });

    document.getElementById('modalMounts').addEventListener('shown.bs.modal', function () {
        showListMounts();
    });
    
    document.getElementById('modalScripts').addEventListener('shown.bs.modal', function () {
        showListScripts();
    });
    
    document.getElementById('modalTrigger').addEventListener('shown.bs.modal', function () {
        showListTrigger();
    });
    
    document.getElementById('modalPartitions').addEventListener('shown.bs.modal', function () {
        showListPartitions();
    });
    
    document.getElementById('modalPartitionOutputs').addEventListener('shown.bs.modal', function () {
        sendAPI("MPD_API_PLAYER_OUTPUT_LIST", {"partition": "default"}, parsePartitionOutputsList, false);
    });
    
    document.getElementById('modalAbout').addEventListener('shown.bs.modal', function () {
        sendAPI("MPD_API_DATABASE_STATS", {}, parseStats);
        getServerinfo();
        let trs = '';
        for (let key in keymap) {
            if (keymap[key].req === undefined || settings[keymap[key].req] === true) {
                trs += '<tr><td><div class="key' + (keymap[key].key && keymap[key].key.length > 1 ? ' material-icons material-icons-small' : '') + 
                       '">' + (keymap[key].key !== undefined ? keymap[key].key : key ) + '</div></td><td>' + t(keymap[key].desc) + '</td></tr>';
            }
        }
        document.getElementById('tbodyShortcuts').innerHTML = trs;
    });
    
    document.getElementById('modalAddToPlaylist').addEventListener('shown.bs.modal', function () {
        if (!document.getElementById('addStreamFrm').classList.contains('hide')) {
            document.getElementById('streamUrl').focus();
            document.getElementById('streamUrl').value = '';
        }
        else {
            document.getElementById('addToPlaylistPlaylist').focus();
        }
    });
    
    document.getElementById('inputTimerVolume').addEventListener('change', function() {
        document.getElementById('textTimerVolume').innerHTML = this.value + '&nbsp;%';
    }, false);
    
    document.getElementById('selectTimerAction').addEventListener('change', function() {
        selectTimerActionChange();
    }, false);
    
    document.getElementById('selectTriggerScript').addEventListener('change', function() {
        selectTriggerActionChange();
    }, false);
    
    let selectTimerHour = ''; 
    for (let i = 0; i < 24; i++) {
        selectTimerHour += '<option value="' + i + '">' + zeroPad(i, 2) + '</option>';
    }
    document.getElementById('selectTimerHour').innerHTML = selectTimerHour;
    
    let selectTimerMinute = ''; 
    for (let i = 0; i < 60; i = i + 5) {
        selectTimerMinute += '<option value="' + i + '">' + zeroPad(i, 2) + '</option>';
    }
    document.getElementById('selectTimerMinute').innerHTML = selectTimerMinute;
    

    document.getElementById('inputHighlightColor').addEventListener('change', function() {
        document.getElementById('highlightColorPreview').style.backgroundColor = this.value;
    }, false);
    
    document.getElementById('inputBgColor').addEventListener('change', function() {
        document.getElementById('bgColorPreview').style.backgroundColor = this.value;
    }, false);
    
    document.getElementById('modalAddToQueue').addEventListener('shown.bs.modal', function () {
        document.getElementById('inputAddToQueueQuantity').classList.remove('is-invalid');
        document.getElementById('warnJukeboxPlaylist2').classList.add('hide');
        if (settings.featPlaylists === true) {
            sendAPI("MPD_API_PLAYLIST_LIST_ALL", {"searchstr": ""}, function(obj) { 
                getAllPlaylists(obj, 'selectAddToQueuePlaylist');
            });
        }
    });

    document.getElementById('modalUpdateDB').addEventListener('hidden.bs.modal', function () {
        document.getElementById('updateDBprogress').classList.remove('updateDBprogressAnimate');
    });
    
    document.getElementById('modalSaveQueue').addEventListener('shown.bs.modal', function () {
        let plName = document.getElementById('saveQueueName');
        plName.focus();
        plName.value = '';
        plName.classList.remove('is-invalid');
    });
        
    document.getElementById('modalSettings').addEventListener('shown.bs.modal', function () {
        getSettings();
        document.getElementById('inputCrossfade').classList.remove('is-invalid');
        document.getElementById('inputMixrampdb').classList.remove('is-invalid');
        document.getElementById('inputMixrampdelay').classList.remove('is-invalid');
        document.getElementById('inputScaleRatio').classList.remove('is-invalid');
    });

    document.getElementById('modalConnection').addEventListener('shown.bs.modal', function () {
        getSettings();
        document.getElementById('inputMpdHost').classList.remove('is-invalid');
        document.getElementById('inputMpdPort').classList.remove('is-invalid');
        document.getElementById('inputMpdPass').classList.remove('is-invalid');
    });

    document.getElementById('btnJukeboxModeGroup').addEventListener('mouseup', function () {
        setTimeout(function() {
            let value = document.getElementById('btnJukeboxModeGroup').getElementsByClassName('active')[0].getAttribute('data-value');
            if (value === '0') {
                document.getElementById('inputJukeboxQueueLength').setAttribute('disabled', 'disabled');
                document.getElementById('selectJukeboxPlaylist').setAttribute('disabled', 'disabled');
            }
            else if (value === '2') {
                document.getElementById('inputJukeboxQueueLength').setAttribute('disabled', 'disabled');
                document.getElementById('selectJukeboxPlaylist').setAttribute('disabled', 'disabled');
                document.getElementById('selectJukeboxPlaylist').value = 'Database';
            }
            else if (value === '1') {
                document.getElementById('inputJukeboxQueueLength').removeAttribute('disabled');
                document.getElementById('selectJukeboxPlaylist').removeAttribute('disabled');
            }
            if (value !== '0') {
                toggleBtnChk('btnConsume', true);            
            }
            checkConsume();
        }, 100);
    });
    
    document.getElementById('btnConsume').addEventListener('mouseup', function() {
        setTimeout(function() { 
            checkConsume(); 
        }, 100);
    });
    
    document.getElementById('btnStickers').addEventListener('mouseup', function() {
        setTimeout(function() {
            if (document.getElementById('btnStickers').classList.contains('active')) {
                document.getElementById('warnPlaybackStatistics').classList.add('hide');
                document.getElementById('inputJukeboxLastPlayed').removeAttribute('disabled');
            }
            else {
                document.getElementById('warnPlaybackStatistics').classList.remove('hide');
                document.getElementById('inputJukeboxLastPlayed').setAttribute('disabled', 'disabled');
            }
        }, 100);
    });
    
    document.getElementById('selectAddToQueueMode').addEventListener('change', function () {
        let value = this.options[this.selectedIndex].value;
        if (value === '2') {
            document.getElementById('inputAddToQueueQuantity').setAttribute('disabled', 'disabled');
            document.getElementById('inputAddToQueueQuantity').value = '1';
            document.getElementById('selectAddToQueuePlaylist').setAttribute('disabled', 'disabled');
            document.getElementById('selectAddToQueuePlaylist').value = 'Database';
        }
        else if (value === '1') {
            document.getElementById('inputAddToQueueQuantity').removeAttribute('disabled');
            document.getElementById('selectAddToQueuePlaylist').removeAttribute('disabled');
        }
    });

    document.getElementById('addToPlaylistPlaylist').addEventListener('change', function () {
        if (this.options[this.selectedIndex].value === 'new') {
            document.getElementById('addToPlaylistNewPlaylistDiv').classList.remove('hide');
            document.getElementById('addToPlaylistNewPlaylist').focus();
        }
        else {
            document.getElementById('addToPlaylistNewPlaylistDiv').classList.add('hide');
        }
    }, false);
    
    document.getElementById('selectMusicDirectory').addEventListener('change', function () {
        if (this.options[this.selectedIndex].value === 'auto') {
            document.getElementById('inputMusicDirectory').value = settings.musicDirectoryValue;
            document.getElementById('inputMusicDirectory').setAttribute('readonly', 'readonly');
        }
        else if (this.options[this.selectedIndex].value === 'none') {
            document.getElementById('inputMusicDirectory').value = '';
            document.getElementById('inputMusicDirectory').setAttribute('readonly', 'readonly');
        }
        else {
            document.getElementById('inputMusicDirectory').value = '';
            document.getElementById('inputMusicDirectory').removeAttribute('readonly');
        }
    }, false);
    
    document.getElementById('syscmds').addEventListener('click', function(event) {
        if (event.target.nodeName === 'A') {
            parseCmd(event, event.target.getAttribute('data-href'));
        }
    }, false);
    
    document.getElementById('scripts').addEventListener('click', function(event) {
        if (event.target.nodeName === 'A') {
            execScript(event.target.getAttribute('data-href'));
        }
    }, false);

    let hrefs = document.querySelectorAll('[data-href]');
    let hrefsLen = hrefs.length;
    for (let i = 0; i < hrefsLen; i++) {
        if (hrefs[i].classList.contains('notclickable') === false) {
            hrefs[i].classList.add('clickable');
        }
        let parentInit = hrefs[i].parentNode.classList.contains('noInitChilds') ? true : false;
        if (parentInit === false) {
            parentInit = hrefs[i].parentNode.parentNode.classList.contains('noInitChilds') ? true : false;
        }
        if (parentInit === true) {
            //handler on parentnode
            continue;
        }
        hrefs[i].addEventListener('click', function(event) {
            parseCmd(event, this.getAttribute('data-href'));
        }, false);
    }
    
    document.getElementById('HomeCards').addEventListener('click', function(event) {
        if (event.target.classList.contains('card-body')) {
            const href = event.target.parentNode.getAttribute('data-href');
            if (href !== null) {
               parseCmd(event, href);
            }
        }
        else if (event.target.classList.contains('card-footer')){
            let sels = document.getElementById('HomeCards').getElementsByClassName('selected');
            for (let i = 0; i < sels.length; i++) {
                sels[i].classList.remove('selected');
            }
            event.target.parentNode.classList.add('selected');
            showMenu(event.target, event);
            event.stopPropagation();
        }
    }, false);
    
    document.getElementById('HomeCards').addEventListener('keydown', function(event) {
        navigateGrid(event.target, event.key);
    }, false);
    
    dragAndDropHome();
    
    document.getElementById('selectHomeIconCmd').addEventListener('change', function() {
        showHomeIconCmdOptions();
    }, false);

    document.getElementById('inputHomeIconLigature').addEventListener('change', function(event) {
        document.getElementById('homeIconPreview').innerText = event.target.value;
        if (event.target.value !== '') {
            document.getElementById('selectHomeIconImage').value = '';
            document.getElementById('homeIconPreview').style.backgroundImage = '';
        }
    }, false);
    
    document.getElementById('inputHomeIconBgcolor').addEventListener('change', function(event) {
        document.getElementById('homeIconPreview').style.backgroundColor = event.target.value;
    }, false);
    
    document.getElementById('selectHomeIconImage').addEventListener('change', function(event) {
        const value = getSelectValue(event.target);
        document.getElementById('homeIconPreview').style.backgroundImage = 'url("' + subdir + '/browse/pics/' + value  + '")';
        if (value !== '') {
            document.getElementById('divHomeIconLigature').classList.add('hide');
            document.getElementById('homeIconPreview').innerHTML = '';
        }
        else {
            document.getElementById('divHomeIconLigature').classList.remove('hide');
            document.getElementById('homeIconPreview').innerText = document.getElementById('inputHomeIconLigature').value;
        }
    }, false);

    let pd = document.getElementsByClassName('pages');
    let pdLen = pd.length;
    for (let i = 0; i < pdLen; i++) {
        pd[i].addEventListener('click', function(event) {
            if (event.target.nodeName === 'BUTTON') {
                gotoPage(event.target.getAttribute('data-page'));
            }
        }, false);
    }

    document.getElementById('cardPlaybackTags').addEventListener('click', function(event) {
        if (event.target.nodeName === 'P') 
            gotoBrowse();
    }, false);

    document.getElementById('BrowseBreadcrumb').addEventListener('click', function(event) {
        if (event.target.nodeName === 'A') {
            event.preventDefault();
            appGoto('Browse', 'Filesystem', undefined, '0', app.current.filter, app.current.sort, '-', decodeURI(event.target.getAttribute('data-uri')));
        }
    }, false);
    
    document.getElementById('tbodySongDetails').addEventListener('click', function(event) {
        if (event.target.nodeName === 'A') {
            if (event.target.id === 'calcFingerprint') {
                sendAPI("MPD_API_DATABASE_FINGERPRINT", {"uri": decodeURI(event.target.getAttribute('data-uri'))}, parseFingerprint);
                event.preventDefault();
                let parent = event.target.parentNode;
                let spinner = document.createElement('div');
                spinner.classList.add('spinner-border', 'spinner-border-sm');
                event.target.classList.add('hide');
                parent.appendChild(spinner);
            }
            else if (event.target.parentNode.getAttribute('data-tag') !== null) {
                modalSongDetails.hide();
                event.preventDefault();
                gotoBrowse();
            } 
        }
        else if (event.target.nodeName === 'BUTTON') { 
            if (event.target.getAttribute('data-href')) {
                parseCmd(event, event.target.getAttribute('data-href'));
            }
        }
    }, false);

    document.getElementById('outputs').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            event.stopPropagation();
            event.preventDefault();
            sendAPI("MPD_API_PLAYER_TOGGLE_OUTPUT", {"output": event.target.getAttribute('data-output-id'), "state": (event.target.classList.contains('active') ? 0 : 1)});
            toggleBtn(event.target.id);
        }
        else if (event.target.nodeName === 'A') {
            event.preventDefault();
            showListOutputAttributes(decodeURI(event.target.parentNode.getAttribute('data-output-name')));
        }
    }, false);
    
    document.getElementById('listTimerList').addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (event.target.nodeName === 'TD') {
            if (!event.target.parentNode.classList.contains('not-clickable')) {
                showEditTimer(event.target.parentNode.getAttribute('data-id'));
            }
        }
        else if (event.target.nodeName === 'A') {
            deleteTimer(event.target.parentNode.parentNode.getAttribute('data-id'));
        }
        else if (event.target.nodeName === 'BUTTON') {
            toggleTimer(event.target, event.target.parentNode.parentNode.getAttribute('data-id'));
        }
    }, false);

    document.getElementById('listMountsList').addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (event.target.nodeName === 'TD') {
            if (event.target.parentNode.getAttribute('data-point') === '') {
                return false;
            }
            showEditMount(decodeURI(event.target.parentNode.getAttribute('data-url')),decodeURI(event.target.parentNode.getAttribute('data-point')));
        }
        else if (event.target.nodeName === 'A') {
            let action = event.target.getAttribute('data-action');
            let mountPoint = decodeURI(event.target.parentNode.parentNode.getAttribute('data-point'));
            if (action === 'unmount') {
                unmountMount(mountPoint);
            }
            else if (action === 'update') {
                updateMount(event.target, mountPoint);
            }
        }
    }, false);
    
    document.getElementById('listScriptsList').addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (event.target.nodeName === 'TD') {
            if (settings.featScripteditor === false || event.target.parentNode.getAttribute('data-script') === '') {
                return false;
            }
            showEditScript(decodeURI(event.target.parentNode.getAttribute('data-script')));
        }
        else if (event.target.nodeName === 'A') {
            let action = event.target.getAttribute('data-action');
            let script = decodeURI(event.target.parentNode.parentNode.getAttribute('data-script'));
            if (action === 'delete') {
                deleteScript(script);
            }
            else if (action === 'execute') {
                execScript(event.target.getAttribute('data-href'));
            }
            else if (action === 'add2home') {
                addScriptToHome(script, event.target.getAttribute('data-href'))
            }
        }
    }, false);
    
    document.getElementById('listTriggerList').addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (event.target.nodeName === 'TD') {
            let id = decodeURI(event.target.parentNode.getAttribute('data-trigger-id'));
            showEditTrigger(id);
        }
        else if (event.target.nodeName === 'A') {
            let action = event.target.getAttribute('data-action');
            let id = decodeURI(event.target.parentNode.parentNode.getAttribute('data-trigger-id'));
            if (action === 'delete') {
                deleteTrigger(id);
            }
        }
    }, false);
    
    document.getElementById('listPartitionsList').addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (event.target.nodeName === 'A') {
            let action = event.target.getAttribute('data-action');
            let partition = decodeURI(event.target.parentNode.parentNode.getAttribute('data-partition'));
            if (action === 'delete') {
                deletePartition(partition);
            }
            else if (action === 'switch') {
                switchPartition(partition);
            }
        }
    }, false);
    
    document.getElementById('partitionOutputsList').addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (event.target.nodeName === 'TD') {
            let outputName = decodeURI(event.target.parentNode.getAttribute('data-output'));
            moveOutput(outputName);
            modalPartitionOutputs.hide();
        }
    }, false);
    
    document.getElementById('QueueCurrentList').addEventListener('click', function(event) {
        if (event.target.nodeName === 'TD') {
            sendAPI("MPD_API_PLAYER_PLAY_TRACK", {"track": event.target.parentNode.getAttribute('data-trackid')});
        }
        else if (event.target.nodeName === 'A') {
            showMenu(event.target, event);
        }
    }, false);
    
    document.getElementById('QueueLastPlayedList').addEventListener('click', function(event) {
        if (event.target.nodeName === 'A') {
            showMenu(event.target, event);
        }
    }, false);

    document.getElementById('QueueJukeboxList').addEventListener('click', function(event) {
        if (event.target.nodeName === 'A') {
            showMenu(event.target, event);
        }
    }, false);

    document.getElementById('BrowseFilesystemList').addEventListener('click', function(event) {
        if (event.target.nodeName === 'TD') {
            switch(event.target.parentNode.getAttribute('data-type')) {
                case 'parentDir':
                case 'dir':
                    appGoto('Browse', 'Filesystem', undefined, '0', app.current.filter, app.current.sort, '-', decodeURI(event.target.parentNode.getAttribute("data-uri")));
                    break;
                case 'song':
                    appendQueue('song', decodeURI(event.target.parentNode.getAttribute("data-uri")), event.target.parentNode.getAttribute("data-name"));
                    break;
                case 'plist':
                    appendQueue('plist', decodeURI(event.target.parentNode.getAttribute("data-uri")), event.target.parentNode.getAttribute("data-name"));
                    break;
            }
        }
        else if (event.target.nodeName === 'A') {
            showMenu(event.target, event);
        }
    }, false);

    document.getElementById('BrowseFilesystemBookmarks').addEventListener('click', function(event) {
        if (event.target.nodeName === 'A') {
            let id = event.target.parentNode.parentNode.getAttribute('data-id');
            let type = event.target.parentNode.parentNode.getAttribute('data-type');
            let uri = decodeURI(event.target.parentNode.parentNode.getAttribute('data-uri'));
            let name = event.target.parentNode.parentNode.firstChild.innerText;
            let href = event.target.getAttribute('data-href');
            
            if (href === 'delete') {
                sendAPI("MYMPD_API_BOOKMARK_RM", {"id": id}, function() {
                    sendAPI("MYMPD_API_BOOKMARK_LIST", {"offset": 0}, parseBookmarks);
                });
                event.preventDefault();
                event.stopPropagation();
            }
            else if (href === 'edit') {
                showBookmarkSave(id, name, uri, type);
            }
            else if (href === 'goto') {
                appGoto('Browse', 'Filesystem', undefined, '0','-','-','-', uri);
            }
        }
    }, false);

    document.getElementById('BrowsePlaylistsAllList').addEventListener('click', function(event) {
        if (event.target.nodeName === 'TD') {
            appendQueue('plist', decodeURI(event.target.parentNode.getAttribute("data-uri")), event.target.parentNode.getAttribute("data-name"));
        }
        else if (event.target.nodeName === 'A') {
            showMenu(event.target, event);
        }
    }, false);

    document.getElementById('BrowsePlaylistsDetailList').addEventListener('click', function(event) {
        if (event.target.nodeName === 'TD') {
            appendQueue('plist', decodeURI(event.target.parentNode.getAttribute("data-uri")), event.target.parentNode.getAttribute("data-name"));
        }
        else if (event.target.nodeName === 'A') {
            showMenu(event.target, event);
        }
    }, false);    
    
    document.getElementById('SearchList').addEventListener('click', function(event) {
        if (event.target.nodeName === 'TD') {
            appendQueue('song', decodeURI(event.target.parentNode.getAttribute("data-uri")), event.target.parentNode.getAttribute("data-name"));
        }
        else if (event.target.nodeName === 'A') {
            showMenu(event.target, event);
        }
    }, false);

    document.getElementById('BrowseDatabaseCards').addEventListener('click', function(event) {
        if (app.current.tag === 'Album') {
            if (event.target.classList.contains('card-body')) {
                appGoto('Browse', 'Database', 'Detail', '0', 'Album', 'AlbumArtist', 
                    decodeURI(event.target.parentNode.getAttribute('data-album')), 
                    decodeURI(event.target.parentNode.getAttribute('data-albumartist')));
            }
            else if (event.target.classList.contains('card-footer')){
                showMenu(event.target, event);
            }
        }
        else {
            app.current.search = '';
            document.getElementById('searchDatabaseStr').value = '';
            appGoto(app.current.app, app.current.card, undefined, '0', 'Album', 'AlbumArtist', 'Album', 
                '(' + app.current.tag + ' == \'' + decodeURI(event.target.parentNode.getAttribute('data-tag')) + '\')');
        }
    }, false);
    
    document.getElementById('BrowseDatabaseCards').addEventListener('keydown', function(event) {
        navigateGrid(event.target, event.key);
    }, false);
    
    document.getElementById('BrowseDatabaseDetailList').addEventListener('click', function(event) {
        if (event.target.nodeName === 'TD') {
            appendQueue('song', decodeURI(event.target.parentNode.getAttribute('data-uri')), event.target.parentNode.getAttribute('data-name'));
        }
        else if (event.target.nodeName === 'A') {
            showMenu(event.target, event);
        }
    }, false);

    document.getElementById('BrowseFilesystemAddAllSongsDropdown').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            if (event.target.getAttribute('data-phrase') === 'Add all to queue') {
                addAllFromBrowseFilesystem();
            }
            else if (event.target.getAttribute('data-phrase') === 'Add all to playlist') {
                showAddToPlaylist(app.current.search, '');
            }
        }
    }, false);

    document.getElementById('searchAddAllSongsDropdown').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            if (event.target.getAttribute('data-phrase') === 'Add all to queue') {
                addAllFromSearchPlist('queue', null, false);
            }
            else if (event.target.getAttribute('data-phrase') === 'Add all to playlist') {
                showAddToPlaylist('SEARCH', '');
            }
            else if (event.target.getAttribute('data-phrase') === 'Save as smart playlist') {
                saveSearchAsSmartPlaylist();
            }
        }
    }, false);
    
    document.getElementById('searchtags').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            app.current.filter = event.target.getAttribute('data-tag');
            doSearch(domCache.searchstr.value);
        }
    }, false);
    
    document.getElementById('searchDatabaseTags').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            app.current.filter = event.target.getAttribute('data-tag');
            appGoto(app.current.app, app.current.tab, app.current.view, '0', app.current.filter, app.current.sort, app.current.tag, app.current.search);
        }
    }, false);
    
    document.getElementById('databaseSortDesc').addEventListener('click', function(event) {
        toggleBtnChk(this);
        event.stopPropagation();
        event.preventDefault();
        if (app.current.sort.charAt(0) === '-') {
            app.current.sort = app.current.sort.substr(1);
        }
        else {
            app.current.sort = '-' + app.current.sort;
        }
        appGoto(app.current.app, app.current.tab, app.current.view, '0', app.current.filter, app.current.sort, app.current.tag, app.current.search);
    }, false);

    document.getElementById('databaseSortTags').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            event.preventDefault();
            event.stopPropagation();
            app.current.sort = event.target.getAttribute('data-tag');
            appGoto(app.current.app, app.current.tab, app.current.view, '0', app.current.filter, app.current.sort, app.current.tag, app.current.search);
        }
    }, false);

    document.getElementById('BrowseDatabaseByTagDropdown').addEventListener('click', function(event) {
        navBrowseHandler(event);
    }, false);
    document.getElementById('BrowseNavPlaylistsDropdown').addEventListener('click', function(event) {
        navBrowseHandler(event);
    }, false);
    document.getElementById('BrowseNavFilesystemDropdown').addEventListener('click', function(event) {
        navBrowseHandler(event);
    }, false);

    document.getElementById('dropdownSortPlaylistTags').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            event.preventDefault();
            playlistSort(event.target.getAttribute('data-tag'));
        }
    }, false);

    document.getElementById('searchqueuestr').addEventListener('keyup', function(event) {
        if (event.key === 'Escape') {
            this.blur();
        }
        else {
            appGoto(app.current.app, app.current.tab, app.current.view, '0', app.current.filter , app.current.sort, '-', this.value);
        }
    }, false);

    document.getElementById('searchqueuetags').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            appGoto(app.current.app, app.current.tab, app.current.view, 
                app.current.page, event.target.getAttribute('data-tag'), app.current.sort, '-', app.current.search);
        }
    }, false);
    
    document.getElementById('searchFilesystemStr').addEventListener('keyup', function(event) {
        if (event.key === 'Escape') {
            this.blur();
        }
        else {
            appGoto(app.current.app, app.current.tab, app.current.view, 
                '0', (this.value !== '' ? this.value : '-'), app.current.sort, '-', app.current.search);
        }
    }, false);
    
    document.getElementById('searchPlaylistsStr').addEventListener('keyup', function(event) {
        if (event.key === 'Escape') {
            this.blur();
        }
        else {
            appGoto(app.current.app, app.current.tab, app.current.view, 
                '0', app.current.filter, app.current.sort, '-', this.value);
        }
    }, false);

    let colDropdowns = ['PlaybackColsDropdown'];
    for (let i = 0; i < colDropdowns.length; i++) {
        document.getElementById(colDropdowns[i]).addEventListener('click', function(event) {
            if (event.target.nodeName === 'BUTTON' && event.target.classList.contains('material-icons')) {
                event.stopPropagation();
                event.preventDefault();
                toggleBtnChk(event.target);
            }
        }, false);
    }
    
    document.getElementById('search').addEventListener('submit', function() {
        return false;
    }, false);

    document.getElementById('searchqueue').addEventListener('submit', function() {
        return false;
    }, false);
    
    document.getElementById('searchdatabase').addEventListener('submit', function() {
        return false;
    }, false);

    document.getElementById('searchDatabaseStr').addEventListener('keyup', function(event) {
        if (event.key === 'Escape') {
            this.blur();
        }
        else if (event.key === 'Enter' && app.current.tag === 'Album') {
            if (this.value !== '') {
                let match = document.getElementById('searchDatabaseMatch');
                let li = document.createElement('button');
                li.classList.add('btn', 'btn-light', 'mr-2');
                li.setAttribute('data-filter', encodeURI(app.current.filter + ' ' + match.options[match.selectedIndex].value + ' \'' + this.value + '\''));
                li.innerHTML = e(app.current.filter) + ' ' + e(match.options[match.selectedIndex].value) + ' \'' + e(this.value) + '\'<span class="ml-2 badge badge-secondary">&times;</span>';
                this.value = '';
                document.getElementById('searchDatabaseCrumb').appendChild(li);
            }
            else {
                searchAlbumgrid(this.value);
            }
        }
        else if (app.current.tag === 'Album') {
            searchAlbumgrid(this.value);
        }
        else {
            appGoto(app.current.app, app.current.tab, app.current.view, 
                '0', app.current.filter, app.current.sort, app.current.tag, this.value);        
        }
    }, false);

    document.getElementById('searchDatabaseCrumb').addEventListener('click', function(event) {
        event.preventDefault();
        event.stopPropagation();
        if (event.target.nodeName === 'SPAN') {
            event.target.parentNode.remove();
            searchAlbumgrid('');
        }
        else if (event.target.nodeName === 'BUTTON') {
            let value = decodeURI(event.target.getAttribute('data-filter'));
            document.getElementById('searchDatabaseStr').value = value.substring(value.indexOf('\'') + 1, value.length - 1);
            let match = value.substring(value.indexOf(' ') + 1);
            match = match.substring(0, match.indexOf(' '));
            document.getElementById('searchDatabaseMatch').value = match;
            let filter = value.substring(0, value.indexOf(' '));
            selectTag('searchDatabaseTags', 'searchDatabaseTagsDesc', filter);
            event.target.remove();
            searchAlbumgrid(document.getElementById('searchDatabaseStr').value);
        }
    }, false);

    domCache.searchstr.addEventListener('keyup', function(event) {
        if (event.key === 'Escape') {
            this.blur();
        }
        else if (event.key === 'Enter' && settings.featAdvsearch) {
            if (this.value !== '') {
                let match = document.getElementById('searchMatch');
                let li = document.createElement('button');
                li.classList.add('btn', 'btn-light', 'mr-2');
                li.setAttribute('data-filter', encodeURI(app.current.filter + ' ' + match.options[match.selectedIndex].value +' \'' + this.value + '\''));
                li.innerHTML = e(app.current.filter) + ' ' + e(match.options[match.selectedIndex].value) + ' \'' + e(this.value) + '\'<span class="ml-2 badge badge-secondary">&times;</span>';
                this.value = '';
                domCache.searchCrumb.appendChild(li);
            }
            else {
                doSearch(this.value);
            }
        }
        else {
            doSearch(this.value);
        }
    }, false);

    domCache.searchCrumb.addEventListener('click', function(event) {
        event.preventDefault();
        event.stopPropagation();
        if (event.target.nodeName === 'SPAN') {
            event.target.parentNode.remove();
            doSearch('');
        }
        else if (event.target.nodeName === 'BUTTON') {
            let value = decodeURI(event.target.getAttribute('data-filter'));
            domCache.searchstr.value = value.substring(value.indexOf('\'') + 1, value.length - 1);
            let filter = value.substring(0, value.indexOf(' '));
            selectTag('searchtags', 'searchtagsdesc', filter);
            let match = value.substring(value.indexOf(' ') + 1);
            match = match.substring(0, match.indexOf(' '));
            document.getElementById('searchMatch').value = match;
            event.target.remove();
            doSearch(domCache.searchstr.value);
        }
    }, false);

    document.getElementById('searchMatch').addEventListener('change', function() {
        doSearch(domCache.searchstr.value);
    }, false);
    
    document.getElementById('SearchList').getElementsByTagName('tr')[0].addEventListener('click', function(event) {
        if (settings.featAdvsearch) {
            if (event.target.nodeName === 'TH') {
                if (event.target.innerHTML === '') {
                    return;
                }
                let col = event.target.getAttribute('data-col');
                if (col === 'Duration') {
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
                
                let s = document.getElementById('SearchList').getElementsByClassName('sort-dir');
                for (let i = 0; i < s.length; i++) {
                    s[i].remove();
                }
                app.current.sort = sortcol;
                event.target.innerHTML = t(col) + '<span class="sort-dir material-icons pull-right">' + 
                    (sortdesc === true ? 'arrow_drop_up' : 'arrow_drop_down') + '</span>';
                appGoto(app.current.app, app.current.tab, app.current.view,
                    app.current.page, app.current.filter,  app.current.sort, '-', app.current.search);
            }
        }
    }, false);

    document.getElementById('inputScriptArgument').addEventListener('keyup', function(event) {
        if (event.key === 'Enter') {
            event.preventDefault();
            event.stopPropagation();
            addScriptArgument();
        }
    }, false);
    
    document.getElementById('selectScriptArguments').addEventListener('click', function(event) {
        if (event.target.nodeName === 'OPTION') {
            removeScriptArgument(event);
        }
    }, false);

    document.getElementsByTagName('body')[0].addEventListener('click', function() {
        hideMenu();
    }, false);

    dragAndDropTable('QueueCurrentList');
    dragAndDropTable('BrowsePlaylistsDetailList');
    dragAndDropTableHeader('QueueCurrent');
    dragAndDropTableHeader('QueueLastPlayed');
    dragAndDropTableHeader('QueueJukebox');
    dragAndDropTableHeader('Search');
    dragAndDropTableHeader('BrowseFilesystem');
    dragAndDropTableHeader('BrowsePlaylistsDetail');
    dragAndDropTableHeader('BrowseDatabaseDetail');

    window.addEventListener('focus', function() {
        sendAPI("MPD_API_PLAYER_STATE", {}, parseState);
    }, false);


    document.addEventListener('keydown', function(event) {
        if (event.target.tagName === 'INPUT' || event.target.tagName === 'SELECT' ||
            event.target.tagName === 'TEXTAREA' || event.ctrlKey || event.altKey) {
            return;
        }
        let cmd = keymap[event.key];
        if (cmd && typeof window[cmd.cmd] === 'function') {
            if (keymap[event.key].req === undefined || settings[keymap[event.key].req] === true)
                parseCmd(event, cmd);
        }        
        
    }, false);
    

    let tables = document.getElementsByTagName('table');
    for (let i = 0; i < tables.length; i++) {
        tables[i].setAttribute('tabindex', 0);
        tables[i].addEventListener('keydown', function(event) {
            navigateTable(this, event.key);
        }, false);
    }

    let selectThemeHtml = '';
    Object.keys(themes).forEach(function(key) {
        selectThemeHtml += '<option value="' + e(key) + '">' + t(themes[key]) + '</option>';
    });
    document.getElementById('selectTheme').innerHTML = selectThemeHtml;

    window.addEventListener('beforeunload', function() {
        webSocketClose();
    });
    
    document.getElementById('alertLocalPlayback').getElementsByTagName('a')[0].addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        clickCheckLocalPlayerState(event);
    }, false);
    
    document.getElementById('errorLocalPlayback').getElementsByTagName('a')[0].addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        clickCheckLocalPlayerState(event);
    }, false);

    document.getElementById('localPlayer').addEventListener('click', function(event) {
        event.stopPropagation();
    });
    
    document.getElementById('localPlayer').addEventListener('canplay', function() {
        logDebug('localPlayer event: canplay');
        document.getElementById('alertLocalPlayback').classList.add('hide');
        document.getElementById('errorLocalPlayback').classList.add('hide');
    });
    document.getElementById('localPlayer').addEventListener('error', function() {
        logError('localPlayer event: error');
        document.getElementById('errorLocalPlayback').classList.remove('hide');
    });
}

//Init app
window.onerror = function(msg, url, line) {
    logError('JavaScript error: ' + msg + ' (' + url + ': ' + line + ')');
    if (settings.loglevel >= 4) {
        if (appInited === true) {
            showNotification(t('JavaScript error'), msg + ' (' + url + ': ' + line + ')', '', 'danger');
        }
        else {
            showAppInitAlert(t('JavaScript error') + ': ' + msg + ' (' + url + ': ' + line + ')');
        }
    }
    return true;
};

appInitStart();
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

function setStateIcon() {
    if (websocketConnected === false || settings.mpdConnected === false) {
        domCache.mainMenu.classList.add('text-light');
        domCache.mainMenu.classList.remove('connected');
    }
    else {
        domCache.mainMenu.classList.add('connected');
        domCache.mainMenu.classList.remove('text-light');
    }
}

function toggleAlert(alertBox, state, msg) {
    const alertBoxEl = document.getElementById(alertBox);
    if (state === false) {
        alertBoxEl.innerHTML = '';
        alertBoxEl.classList.add('hide');
    }
    else {
        alertBoxEl.innerHTML = msg;
        alertBoxEl.classList.remove('hide');
    }
}

function showNotification(notificationTitle, notificationText, notificationHtml, notificationType) {
    if (settings.notificationWeb === true) {
        let notification = new Notification(notificationTitle, {icon: 'assets/favicon.ico', body: notificationText});
        setTimeout(notification.close.bind(notification), 3000);
    } 
    if (settings.notificationPage === true || notificationType === 'danger' || notificationType === 'warning') {
        let alertBox;
        if (alertTimeout) {
            clearTimeout(alertTimeout);
        }
        if (!document.getElementById('alertBox')) {
            alertBox = document.createElement('div');
            alertBox.setAttribute('id', 'alertBox');
            alertBox.classList.add('toast');
        }
        else {
            alertBox = document.getElementById('alertBox');
        }
        
        let toast = '<div class="toast-header">';
        if (notificationType === 'success' ) {
            toast += '<span class="material-icons text-success mr-2">info</span>';
        }
        else if (notificationType === 'warning' ) {
            toast += '<span class="material-icons text-warning mr-2">warning</span>';
        }
        else {
            toast += '<span class="material-icons text-danger mr-2">error</span>';
        }
        toast += '<strong class="mr-auto">' + e(notificationTitle) + '</strong>' +
            '<button type="button" class="ml-2 mb-1 close">&times;</button></div>';
        if (notificationHtml !== '' || notificationText !== '') {
            toast += '<div class="toast-body">' + (notificationHtml === '' ? e(notificationText) : notificationHtml) + '</div>';
        }
        toast += '</div>';
        alertBox.innerHTML = toast;
        
        if (!document.getElementById('alertBox')) {
            document.getElementsByTagName('main')[0].append(alertBox);
            requestAnimationFrame(function() {
                let ab = document.getElementById('alertBox');
                if (ab) {
                    ab.classList.add('alertBoxActive');
                }
            });
        }
        alertBox.getElementsByTagName('button')[0].addEventListener('click', function() {
            hideNotification();
        }, false);

        alertTimeout = setTimeout(function() {
            hideNotification();
        }, 3000);
    }
    setStateIcon();
    logMessage(notificationTitle, notificationText, notificationHtml, notificationType);
}

function logMessage(notificationTitle, notificationText, notificationHtml, notificationType) {
    if (notificationType === 'success') { notificationType = 'Info'; }
    else if (notificationType === 'warning') { notificationType = 'Warning'; }
    else if (notificationType === 'danger') { notificationType = 'Error'; }
    
    let overview = document.getElementById('logOverview');

    let append = true;
    let lastEntry = overview.firstElementChild;
    if (lastEntry) {
        if (lastEntry.getAttribute('data-title') === notificationTitle) {
            append = false;        
        }
    }

    let entry = document.createElement('div');
    entry.classList.add('text-light');
    entry.setAttribute('data-title', notificationTitle);
    let occurence = 1;
    if (append === false) {
        occurence += parseInt(lastEntry.getAttribute('data-occurence'));
    }
    entry.setAttribute('data-occurence', occurence);
    entry.innerHTML = '<small>' + localeDate() + '&nbsp;&ndash;&nbsp;' + t(notificationType) +
        (occurence > 1 ? '&nbsp;(' + occurence + ')' : '') + '</small>' +
        '<p>' + e(notificationTitle) +
        (notificationHtml === '' && notificationText === '' ? '' :
        '<br/>' + (notificationHtml === '' ? e(notificationText) : notificationHtml)) +
        '</p>';

    if (append === true) {
        overview.insertBefore(entry, overview.firstElementChild);
    }
    else {
        overview.replaceChild(entry, lastEntry);
    }
   
    let overviewEls = overview.getElementsByTagName('div');
    if (overviewEls.length > 10) {
        overviewEls[10].remove();
    }
}

//eslint-disable-next-line no-unused-vars
function clearLogOverview() {
    let overviewEls = document.getElementById('logOverview').getElementsByTagName('div');
    for (let i = overviewEls.length - 1; i >= 0; i--) {
        overviewEls[i].remove();
    }
    setStateIcon();
}

function hideNotification() {
    if (alertTimeout) {
        clearTimeout(alertTimeout);
    }

    if (document.getElementById('alertBox')) {
        document.getElementById('alertBox').classList.remove('alertBoxActive');
        setTimeout(function() {
            let alertBox = document.getElementById('alertBox');
            if (alertBox) {
                alertBox.remove();
            }
        }, 750);
    }
}

function notificationsSupported() {
    return "Notification" in window;
}

function setElsState(tag, state, type) {
    let els = type === 'tag' ? document.getElementsByTagName(tag) : document.getElementsByClassName(tag);
    let elsLen = els.length;
    for (let i = 0; i < elsLen; i++) {
        if (els[i].classList.contains('close')) {
            continue;
        }
        if (state === 'disabled') {
            if (els[i].classList.contains('alwaysEnabled') === false) {
                if (els[i].getAttribute('disabled') === null) {
                    els[i].setAttribute('disabled', 'disabled');
                    els[i].classList.add('disabled');
                }
            }
        }
        else {
            if (els[i].classList.contains('disabled')) {
                els[i].removeAttribute('disabled');
                els[i].classList.remove('disabled');
            }
        }
    }
}

function toggleUI() {
    let state = 'disabled';
    const topAlert = document.getElementById('top-alerts');
    if (websocketConnected === true && settings.mpdConnected === true) {
        topAlert.classList.add('hide');
        state = 'enabled';
    }
    else {
        let topPadding = 0;
        if (window.innerWidth < window.innerHeight) {
            topPadding = domCache.header.offsetHeight;
        }
        topAlert.style.paddingTop = topPadding + 'px';
        topAlert.classList.remove('hide');
    }
    let enabled = state === 'disabled' ? false : true;
    if (enabled !== uiEnabled) {
        logDebug('Setting ui state to ' + state);
        setElsState('a', state, 'tag');
        setElsState('input', state, 'tag');
        setElsState('button', state, 'tag');
        setElsState('clickable', state, 'class');
        uiEnabled = enabled;
    }

    if (settings.mpdConnected === true) {
        toggleAlert('alertMpdState', false, '');
    }
    else {
        toggleAlert('alertMpdState', true, t('MPD disconnected'));
        logMessage(t('MPD disconnected'), '', '', 'danger');
    }

    if (websocketConnected === true) {
        toggleAlert('alertMympdState', false, '');
    }
    else {
        toggleAlert('alertMympdState', true, t('Websocket is disconnected'));
        logMessage(t('Websocket is disconnected'), '', '', 'danger');
    }
 
    setStateIcon();
}
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

function moveOutput(output) {
    sendAPI("MPD_API_PARTITION_OUTPUT_MOVE", {"name": output});
}

function parsePartitionOutputsList(obj) {
    let outputs = document.getElementById('outputs').getElementsByTagName('button');
    let outputIds = [];
    for (let i = 0; i < outputs.length; i++) {
        outputIds.push(parseInt(outputs[i].getAttribute('data-output-id')));
    }

    let outputList = '';
    let nr = 0;
    for (let i = 0; i < obj.result.data.length; i++) {
        if (outputIds.includes(obj.result.data[i].id) === false) {
            outputList += '<tr data-output="' + encodeURI(obj.result.data[i].name) + '"><td>' +
                e(obj.result.data[i].name) + '</td></tr>';
            nr++;
        }
    }
    if (nr === 0) {
        outputList = '<tr class="not-clickable"><td><span class="material-icons">error_outline</span>&nbsp;' +
            t('Empty list') + '</td></tr>';
    }
    document.getElementById('partitionOutputsList').innerHTML = outputList;
}

//eslint-disable-next-line no-unused-vars
function savePartition() {
    let formOK = true;
    
    let nameEl = document.getElementById('inputPartitionName');
    if (!validatePlnameEl(nameEl)) {
        formOK = false;
    }
    
    if (formOK === true) {
        sendAPI("MPD_API_PARTITION_NEW", {
            "name": nameEl.value
            }, showListPartitions, false);
    }
}

//eslint-disable-next-line no-unused-vars
function showNewPartition() {
    document.getElementById('listPartitions').classList.remove('active');
    document.getElementById('newPartition').classList.add('active');
    document.getElementById('listPartitionsFooter').classList.add('hide');
    document.getElementById('newPartitionFooter').classList.remove('hide');
    
    const nameEl = document.getElementById('inputPartitionName');
    nameEl.classList.remove('is-invalid');
    nameEl.value = '';
    nameEl.focus();
}

function showListPartitions() {
    document.getElementById('listPartitions').classList.add('active');
    document.getElementById('newPartition').classList.remove('active');
    document.getElementById('listPartitionsFooter').classList.remove('hide');
    document.getElementById('newPartitionFooter').classList.add('hide');
    document.getElementById('errorPartition').classList.add('hide');
    sendAPI("MPD_API_PARTITION_LIST", {}, parsePartitionList, false);
}

function deletePartition(partition) {
    sendAPI("MPD_API_PARTITION_RM", {"name": partition}, function(obj) {
        if (obj.error) {
            let el = document.getElementById('errorPartition');
            el.innerText = t(obj.error.message);
            el.classList.remove('hide');
        }
        sendAPI("MPD_API_PARTITION_LIST", {}, parsePartitionList, false);
    }, true);
}

function switchPartition(partition) {
    sendAPI("MPD_API_PARTITION_SWITCH", {"name": partition}, function(obj) {
        if (obj.error) {
            let el = document.getElementById('errorPartition');
            el.innerText = t(obj.error.message);
            el.classList.remove('hide');
        }
        sendAPI("MPD_API_PARTITION_LIST", {}, parsePartitionList, false);
        sendAPI("MPD_API_PLAYER_STATE", {}, parseState);
    }, true);
}

function parsePartitionList(obj) {
    if (obj.result.data.length > 0) {
        let partitionList = '';
        for (let i = 0; i < obj.result.data.length; i++) {
            partitionList += '<tr data-partition="' + encodeURI(obj.result.data[i].name) + '"><td class="' +
                (obj.result.data[i].name === settings.partition ? 'font-weight-bold' : '') +
                '">' + e(obj.result.data[i].name) + 
                (obj.result.data[i].name === settings.partition ? '&nbsp;(' + t('current') + ')' : '') +
                '</td>' +
                '<td data-col="Action">' +
                (obj.result.data[i].name === 'default' || obj.result.data[i].name === settings.partition  ? '' : 
                    '<a href="#" title="' + t('Delete') + '" data-action="delete" class="material-icons color-darkgrey">delete</a>') +
                (obj.result.data[i].name !== settings.partition ? '<a href="#" title="' + t('Switch to') + '" data-action="switch" class="material-icons color-darkgrey">check_circle</a>' : '') +
                '</td></tr>';
        }
        document.getElementById('listPartitionsList').innerHTML = partitionList;
    }
    else {
        document.getElementById('listPartitionsList').innerHTML = '<tr class="not-clickable"><td><span class="material-icons">error_outline</span></td>' +
            '<td colspan="2">' + t('Empty list') + '</td></tr>';
    }
}
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
        document.getElementById('BrowseNavPlaylists').parentNode.classList.remove('hide');
    }
    else {
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
        //document.getElementById('cardFooterBrowse').innerText = gtPage('Num playlists', obj.result.returnedEntities, obj.result.totalEntities);
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
        //document.getElementById('cardFooterBrowse').innerText = gtPage('Num songs', obj.result.returnedEntities, obj.result.totalEntities);
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
            tbody.innerHTML = '<tr class="not-clickable"><td><span class="material-icons">error_outline</span></td>' +
                              '<td colspan="3">' + t('No playlists found') + '</td></tr>';
        }
        else {
            tbody.innerHTML = '<tr class="not-clickable"><td><span class="material-icons">error_outline</span></td>' +
                              '<td colspan="' + settings.colsBrowsePlaylistsDetail.length + '">' + t('Empty playlist') + '</td></tr>';
        }
    }
            
    document.getElementById(app.current.app + app.current.tab + app.current.view + 'List').classList.remove('opacity05');
}

//eslint-disable-next-line no-unused-vars
function playlistDetails(uri) {
    document.getElementById('BrowsePlaylistsAllList').classList.add('opacity05');
    appGoto('Browse', 'Playlists', 'Detail', '0', uri, '-', '-', '');
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
        sendAPI("MPD_API_PLAYLIST_LIST_ALL", {"searchstr": ""}, function(obj) {
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
    let uri = document.getElementById('BrowsePlaylistsDetailList').getAttribute('data-uri');
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
        showAddToPlaylist(item.getAttribute('data-uri'), '');
    }
}
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

function b64EncodeUnicode(str) {
    return btoa(encodeURIComponent(str).replace(/%([0-9A-F]{2})/g,
        function toSolidBytes(match, p1) {
            return String.fromCharCode('0x' + p1);
    }));
}

function b64DecodeUnicode(str) {
    return decodeURIComponent(atob(str).split('').map(function(c) {
        return '%' + ('00' + c.charCodeAt(0).toString(16)).slice(-2);
    }).join(''));
}

function addMenuItem(href, text) {
    return '<a class="dropdown-item" href="#" data-href=\'' + b64EncodeUnicode(JSON.stringify(href)) + '\'>' + text +'</a>';
}

function hideMenu() {
    let menuEl = document.querySelector('[data-popover]');
    if (menuEl) {
        let m = new BSN.Popover(menuEl, {});
        m.hide();
        menuEl.removeAttribute('data-popover');
        if (menuEl.parentNode.parentNode.classList.contains('selected')) {
            focusTable(undefined, menuEl.parentNode.parentNode.parentNode.parentNode);
        }
        else if (app.current.app === 'Browse' && app.current.tab === 'Database') {
            focusTable(undefined, menuEl.parentNode.parentNode.parentNode.parentNode);
        }
        else if (app.current.app === 'Home') {
            focusTable(undefined, menuEl.parentNode.parentNode.parentNode.parentNode);
        }
    }
}

function showMenu(el, event) {
    event.preventDefault();
    event.stopPropagation();
    hideMenu();
    //if (el.getAttribute('data-init')) {
    //    return;
    //}
    if (el.parentNode.nodeName === 'TH') {
        showMenuTh(el);
    }
    else {
        showMenuTd(el);
    }
}

function showMenuTh(el) {
    let table = app.current.app + (app.current.tab !== undefined ? app.current.tab : '') + (app.current.view !== undefined ? app.current.view : '');
    let menu = '<form class="p-2" id="colChecklist' + table + '">';
    menu += setColsChecklist(table);
    menu += '<button class="btn btn-success btn-block btn-sm mt-2">' + t('Apply') + '</button>';
    menu += '</form>';
    new BSN.Popover(el, { trigger: 'click', delay: 0, dismissible: true, template: '<div class="popover" role="tooltip">' +
        '<div class="arrow"></div>' +
        '<div class="popover-content" id="' + table + 'ColsDropdown' + '">' + menu + '</div>' +
        '</div>', content: ' '});
    let popoverInit = el.Popover;
    if (el.getAttribute('data-init') === null) {
        el.setAttribute('data-init', 'true');
        el.addEventListener('shown.bs.popover', function(event) {
            event.target.setAttribute('data-popover', 'true');
            document.getElementById('colChecklist' + table).addEventListener('click', function(eventClick) {
                if (eventClick.target.nodeName === 'BUTTON' && eventClick.target.classList.contains('material-icons')) {
                    toggleBtnChk(eventClick.target);
                    eventClick.preventDefault();
                    eventClick.stopPropagation();
                }
                else if (eventClick.target.nodeName === 'BUTTON') {
                    eventClick.preventDefault();
                    saveCols(table);
                }
            }, false);
        }, false);
    }
    popoverInit.show();
}

function showMenuTd(el) {
    let type = el.getAttribute('data-type');
    let uri = decodeURI(el.getAttribute('data-uri'));
    let name = decodeURI(el.getAttribute('data-name'));
    let nextsongpos = 0;
    if (type === null || uri === '') {
        type = el.parentNode.getAttribute('data-type');
        uri = decodeURI(el.parentNode.getAttribute('data-uri'));
        name = el.parentNode.getAttribute('data-name');
    }
    if (type === null || uri === '') {
        type = el.parentNode.parentNode.getAttribute('data-type');
        uri = decodeURI(el.parentNode.parentNode.getAttribute('data-uri'));
        name = el.parentNode.parentNode.getAttribute('data-name');
    }
    
    if (lastState) {
        nextsongpos = lastState.nextSongPos;
    }

    let menu = '';
    if ((app.current.app === 'Browse' && app.current.tab === 'Filesystem') || app.current.app === 'Search' ||
        (app.current.app === 'Browse' && app.current.tab === 'Database')) {
        menu += addMenuItem({"cmd": "appendQueue", "options": [type, uri, name]}, t('Append to queue')) +
            (type === 'song' ? addMenuItem({"cmd": "appendAfterQueue", "options": [type, uri, nextsongpos, name]}, t('Add after current playing song')) : '') +
            addMenuItem({"cmd": "replaceQueue", "options": [type, uri, name]}, t('Replace queue')) +
            (type !== 'plist' && type !== 'smartpls' && settings.featPlaylists ? addMenuItem({"cmd": "showAddToPlaylist", "options": [uri, ""]}, t('Add to playlist')) : '') +
            (type === 'song' ? addMenuItem({"cmd": "songDetails", "options": [uri]}, t('Song details')) : '') +
            (type === 'plist' || type === 'smartpls' ? addMenuItem({"cmd": "playlistDetails", "options": [uri]}, t('View playlist')) : '') +
            ((type === 'plist' || type === 'smartpls') && settings.featHome === true ? addMenuItem({"cmd": "addPlistToHome", "options": [uri, name]}, t('Add to homescreen')) : '') +
            (type === 'dir' && settings.featBookmarks ? addMenuItem({"cmd": "showBookmarkSave", "options": [-1, name, uri, type]}, t('Add bookmark')) : '');
        if (app.current.tab === 'Filesystem') {
            menu += (type === 'dir' ? addMenuItem({"cmd": "updateDB", "options": [dirname(uri), true]}, t('Update directory')) : '') +
                (type === 'dir' ? addMenuItem({"cmd": "rescanDB", "options": [dirname(uri), true]}, t('Rescan directory')) : '');
        }
        if (app.current.app === 'Search') {
            //songs must be arragend in one album per folder
            let baseuri = dirname(uri);
            menu += '<div class="dropdown-divider"></div>' +
                '<a class="dropdown-item" id="advancedMenuLink" data-toggle="collapse" href="#advancedMenu"><span class="material-icons material-icons-small-left">keyboard_arrow_right</span>Album actions</a>' +
                '<div class="collapse" id="advancedMenu">' +
                    addMenuItem({"cmd": "appendQueue", "options": [type, baseuri, name]}, t('Append to queue')) +
                    addMenuItem({"cmd": "appendAfterQueue", "options": [type, baseuri, nextsongpos, name]}, t('Add after current playing song')) +
                    addMenuItem({"cmd": "replaceQueue", "options": [type, baseuri, name]}, t('Replace queue')) +
                    (settings.featPlaylists ? addMenuItem({"cmd": "showAddToPlaylist", "options": [baseuri, ""]}, t('Add to playlist')) : '') +
                '</div>';
        }
    }
    else if (app.current.app === 'Browse' && app.current.tab === 'Playlists' && app.current.view === 'All') {
        menu += addMenuItem({"cmd": "appendQueue", "options": [type, uri, name]}, t('Append to queue')) +
            addMenuItem({"cmd": "replaceQueue", "options": [type, uri, name]}, t('Replace queue')) +
            (settings.smartpls === true && type === 'smartpls' ? addMenuItem({"cmd": "playlistDetails", "options": [uri]}, t('View playlist')) : addMenuItem({"cmd": "playlistDetails", "options": [uri]}, t('Edit playlist')))+
            (settings.smartpls === true && type === 'smartpls' ? addMenuItem({"cmd": "showSmartPlaylist", "options": [uri]}, t('Edit smart playlist')) : '') +
            (settings.smartpls === true && type === 'smartpls' ? addMenuItem({"cmd": "updateSmartPlaylist", "options": [uri]}, t('Update smart playlist')) : '') +
            addMenuItem({"cmd": "showRenamePlaylist", "options": [uri]}, t('Rename playlist')) + 
            addMenuItem({"cmd": "showDelPlaylist", "options": [uri]}, t('Delete playlist')) +
            (settings.featHome === true ?addMenuItem({"cmd": "addPlistToHome", "options": [uri, name]}, t('Add to homescreen')) : '');
    }
    else if (app.current.app === 'Browse' && app.current.tab === 'Playlists' && app.current.view === 'Detail') {
        let x = document.getElementById('BrowsePlaylistsDetailList');
        menu += addMenuItem({"cmd": "appendQueue", "options": [type, uri, name]}, t('Append to queue')) +
            addMenuItem({"cmd": "replaceQueue", "options": [type, uri, name]}, t('Replace queue')) +
            (x.getAttribute('data-ro') === 'false' ? addMenuItem({"cmd": "removeFromPlaylist", "options": [x.getAttribute('data-uri'), 
                    el.parentNode.parentNode.getAttribute('data-songpos')]}, t('Remove')) : '') +
            (settings.featPlaylists ? addMenuItem({"cmd": "showAddToPlaylist", "options": [uri, ""]}, t('Add to playlist')) : '') +
            (uri.indexOf('http') === -1 ? addMenuItem({"cmd": "songDetails", "options": [uri]}, t('Song details')) : '');
    }
    else if (app.current.app === 'Queue' && app.current.tab === 'Current') {
        menu += addMenuItem({"cmd": "delQueueSong", "options": ["single", el.parentNode.parentNode.getAttribute('data-trackid')]}, t('Remove')) +
            addMenuItem({"cmd": "delQueueSong", "options": ["range", 0, el.parentNode.parentNode.getAttribute('data-songpos')]}, t('Remove all upwards')) +
            addMenuItem({"cmd": "delQueueSong", "options": ["range", (parseInt(el.parentNode.parentNode.getAttribute('data-songpos'))-1), -1]}, t('Remove all downwards')) +
            (uri.indexOf('http') === -1 ? addMenuItem({"cmd": "songDetails", "options": [uri]}, t('Song details')) : '');
    }
    else if (app.current.app === 'Queue' && app.current.tab === 'LastPlayed') {
        menu += addMenuItem({"cmd": "appendQueue", "options": [type, uri, name]}, t('Append to queue')) +
            addMenuItem({"cmd": "replaceQueue", "options": [type, uri, name]}, t('Replace queue')) +
            (settings.featPlaylists ? addMenuItem({"cmd": "showAddToPlaylist", "options": [uri, ""]}, t('Add to playlist')) : '') +
            (uri.indexOf('http') === -1 ? addMenuItem({"cmd": "songDetails", "options": [uri]}, t('Song details')) : '');
    }
    else if (app.current.app === 'Queue' && app.current.tab === 'Jukebox') {
        menu += addMenuItem({"cmd": "songDetails", "options": [uri]}, t('Song details')) +
            addMenuItem({"cmd": "delQueueJukeboxSong", "options": [el.parentNode.parentNode.getAttribute('data-pos')]}, t('Remove'));
    }
    else if (app.current.app === 'Home') {
        const pos = parseInt(el.parentNode.getAttribute('data-pos'));
        const href = JSON.parse(el.parentNode.getAttribute('data-href'));
        let actionDesc = '';
        if (href.cmd === 'replaceQueue' && href.options[0] === 'plist') {
            type = 'plist';
            uri = href.options[1];
            actionDesc = t('Add and play playlist');
            name = t('Playlist');
        }
        else if (href.cmd === 'appGoto') {
            type = 'view';
            actionDesc = t('Goto view');
            name = t('View');
        }
        else if (href.cmd === 'execScriptFromOptions') {
            type = 'script';
            actionDesc = t('Execute script');
            name = t('Script');
        }
        menu += '<h6 class="dropdown-header">' + name + '</h6>' +
                addMenuItem({"cmd": "executeHomeIcon", "options": [pos]}, actionDesc) +
                (type === 'plist' ? addMenuItem({"cmd": "playlistDetails", "options": [uri]}, t('View playlist')) : '') +
                addMenuItem({"cmd": "editHomeIcon", "options": [pos]}, t('Edit home icon')) +
                addMenuItem({"cmd": "duplicateHomeIcon", "options": [pos]}, t('Duplicate home icon')) +
                addMenuItem({"cmd": "deleteHomeIcon", "options": [pos]}, t('Delete home icon'));
    }

    new BSN.Popover(el, { trigger: 'click', delay: 0, dismissible: true, template: '<div class="popover" role="tooltip">' +
        '<div class="arrow"></div>' +
        '<div class="popover-content">' + menu + '</div>' +
        '</div>', content: ' '});
    let popoverInit = el.Popover;
    if (el.getAttribute('data-init') === null) {
        el.setAttribute('data-init', 'true');
        el.addEventListener('shown.bs.popover', function(event) {
            event.target.setAttribute('data-popover', 'true');
            document.getElementsByClassName('popover-content')[0].addEventListener('click', function(eventClick) {
                eventClick.preventDefault();
                eventClick.stopPropagation();
                if (eventClick.target.nodeName === 'A') {
                    let dh = eventClick.target.getAttribute('data-href');
                    if (dh) {
                        let cmd = JSON.parse(b64DecodeUnicode(dh));
                        parseCmd(event, cmd);
                        hideMenu();
                    }
                }
            }, false);
            document.getElementsByClassName('popover-content')[0].addEventListener('keydown', function(eventKey) {
                eventKey.preventDefault();
                eventKey.stopPropagation();
                if (eventKey.key === 'ArrowDown' || eventKey.key === 'ArrowUp') {
                    let menuItemsHtml = this.getElementsByTagName('a');
                    let menuItems = Array.prototype.slice.call(menuItemsHtml);
                    let idx = menuItems.indexOf(document.activeElement);
                    do {
                        idx = eventKey.key === 'ArrowUp' ? (idx > 1 ? idx - 1 : 0)
                                                 : eventKey.key === 'ArrowDown' ? ( idx < menuItems.length - 1 ? idx + 1 : idx)
                                                                            : idx;
                        if ( idx === 0 || idx === menuItems.length -1 ) {
                            break;
                        }
                    } while ( !menuItems[idx].offsetHeight )
                    menuItems[idx] && menuItems[idx].focus();
                }
                else if (eventKey.key === 'Enter') {
                    eventKey.target.click();
                }
                else if (eventKey.key === 'Escape') {
                    hideMenu();
                }
            }, false);
            let collapseLink = document.getElementById('advancedMenuLink');
            if (collapseLink) {
                collapseLink.addEventListener('click', function() {
                    let icon = this.getElementsByTagName('span')[0];
                    if (icon.innerText === 'keyboard_arrow_right') {
                        icon.innerText = 'keyboard_arrow_down';
                    }
                    else {
                        icon.innerText = 'keyboard_arrow_right';
                    }
                }, false);
                new BSN.Collapse(collapseLink);
            }
            document.getElementsByClassName('popover-content')[0].firstChild.focus();
        }, false);
    }
    popoverInit.show();
}
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

function parseUpdateQueue(obj) {
    //Set playstate
    if (obj.result.state === 1) {
        for (let i = 0; i < domCache.btnsPlayLen; i++) {
            domCache.btnsPlay[i].innerText = 'play_arrow';
        }
        playstate = 'stop';
        domCache.progressBar.style.transition = 'none';
        domCache.progressBar.style.width = '0px';
        setTimeout(function() {
            domCache.progressBar.style.transition = progressBarTransition;
        }, 10);
    }
    else if (obj.result.state === 2) {
        for (let i = 0; i < domCache.btnsPlayLen; i++) {
            if (settings.footerStop === true) {
                domCache.btnsPlay[i].innerText = 'stop';
            }
            else {
                domCache.btnsPlay[i].innerText = 'pause';
            }
        }
        playstate = 'play';
    }
    else {
        for (let i = 0; i < domCache.btnsPlayLen; i++) {
            domCache.btnsPlay[i].innerText = 'play_arrow';
        }
	playstate = 'pause';
    }

    if (obj.result.queueLength === 0) {
        for (let i = 0; i < domCache.btnsPlayLen; i++) {
            domCache.btnsPlay[i].setAttribute('disabled', 'disabled');
        }
    }
    else {
        for (let i = 0; i < domCache.btnsPlayLen; i++) {
            domCache.btnsPlay[i].removeAttribute('disabled');
        }
    }

    mediaSessionSetState();
    mediaSessionSetPositionState(obj.result.totalTime, obj.result.elapsedTime);

    domCache.badgeQueueItems.innerText = obj.result.queueLength;
    
    if (obj.result.nextSongPos === -1 && settings.jukeboxMode === false) {
        domCache.btnNext.setAttribute('disabled', 'disabled');
    }
    else {
        domCache.btnNext.removeAttribute('disabled');
    }
    
    if (obj.result.songPos <= 0) {
        domCache.btnPrev.setAttribute('disabled', 'disabled');
    }
    else {
        domCache.btnPrev.removeAttribute('disabled');
    }
}

function getQueue() {
    if (app.current.search.length >= 2) {
        sendAPI("MPD_API_QUEUE_SEARCH", {"filter": app.current.filter, "offset": app.current.page, "searchstr": app.current.search, "cols": settings.colsQueueCurrent}, parseQueue, false);
    }
    else {
        sendAPI("MPD_API_QUEUE_LIST", {"offset": app.current.page, "cols": settings.colsQueueCurrent}, parseQueue, false);
    }
}

function parseQueue(obj) {
    if (obj.result.offset < app.current.page) {
        gotoPage(obj.result.offset);
        return;
    }
/*
    if (obj.result.totalTime && obj.result.totalTime > 0 && obj.result.totalEntities <= settings.maxElementsPerPage ) {
        document.getElementById('cardFooterQueue').innerText = t('Num songs', obj.result.totalEntities) + ' – ' + beautifyDuration(obj.result.totalTime);
    }
    else if (obj.result.totalEntities > 0) {
        document.getElementById('cardFooterQueue').innerText = t('Num songs', obj.result.totalEntities);
    }
    else {
        document.getElementById('cardFooterQueue').innerText = '';
    }
*/
    if (obj.result.totalEntities > settings.maxElementsPerPage) {
        document.getElementById('btnQueueGotoPlayingSong').parentNode.classList.remove('hide');
    }
    else {
        document.getElementById('btnQueueGotoPlayingSong').parentNode.classList.add('hide');
    }

    let nrItems = obj.result.returnedEntities;
    let table = document.getElementById('QueueCurrentList');
    let navigate = document.activeElement.parentNode.parentNode === table ? true : false;
    let activeRow = 0;
    table.setAttribute('data-version', obj.result.queueVersion);
    let tbody = table.getElementsByTagName('tbody')[0];
    let tr = tbody.getElementsByTagName('tr');
    for (let i = 0; i < nrItems; i++) {
        obj.result.data[i].Duration = beautifySongDuration(obj.result.data[i].Duration);
        obj.result.data[i].Pos++;
        let row = document.createElement('tr');
        row.setAttribute('draggable', 'true');
        row.setAttribute('data-trackid', obj.result.data[i].id);
        row.setAttribute('id','queueTrackId' + obj.result.data[i].id);
        row.setAttribute('data-songpos', obj.result.data[i].Pos);
        row.setAttribute('data-duration', obj.result.data[i].Duration);
        row.setAttribute('data-uri', obj.result.data[i].uri);
        row.setAttribute('tabindex', 0);
        let tds = '';
        for (let c = 0; c < settings.colsQueueCurrent.length; c++) {
            tds += '<td data-col="' + settings.colsQueueCurrent[c] + '">' + e(obj.result.data[i][settings.colsQueueCurrent[c]]) + '</td>';
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
    let trLen = tr.length - 1;
    for (let i = trLen; i >= nrItems; i --) {
        tr[i].remove();
    }

    let colspan = settings['colsQueueCurrent'].length;
    colspan--;

    if (obj.result.method === 'MPD_API_QUEUE_SEARCH' && nrItems === 0) {
        tbody.innerHTML = '<tr><td><span class="material-icons">error_outline</span></td>' +
                          '<td colspan="' + colspan + '">' + t('No results, please refine your search') + '</td></tr>';
    }
    else if (obj.result.method === 'MPD_API_QUEUE_ADD_TRACK' && nrItems === 0) {
        tbody.innerHTML = '<tr><td><span class="material-icons">error_outline</span></td>' +
                          '<td colspan="' + colspan + '">' + t('Empty queue') + '</td></tr>';
    }

    if (navigate === true) {
        focusTable(activeRow);
    }
    
    setPagination(obj.result.totalEntities, obj.result.returnedEntities);
    document.getElementById('QueueCurrentList').classList.remove('opacity05');
}

function parseLastPlayed(obj) {
    //document.getElementById('cardFooterQueue').innerText = t('Num songs', obj.result.totalEntities);
    let nrItems = obj.result.returnedEntities;
    let table = document.getElementById('QueueLastPlayedList');
    let navigate = document.activeElement.parentNode.parentNode === table ? true : false;
    let activeRow = 0;
    let tbody = table.getElementsByTagName('tbody')[0];
    let tr = tbody.getElementsByTagName('tr');
    for (let i = 0; i < nrItems; i++) {
        obj.result.data[i].Duration = beautifySongDuration(obj.result.data[i].Duration);
        obj.result.data[i].LastPlayed = localeDate(obj.result.data[i].LastPlayed);
        let row = document.createElement('tr');
        row.setAttribute('data-uri', obj.result.data[i].uri);
        row.setAttribute('data-name', obj.result.data[i].Title);
        row.setAttribute('data-type', 'song');
        row.setAttribute('tabindex', 0);
        let tds = '';
        for (let c = 0; c < settings.colsQueueLastPlayed.length; c++) {
            tds += '<td data-col="' + settings.colsQueueLastPlayed[c] + '">' + e(obj.result.data[i][settings.colsQueueLastPlayed[c]]) + '</td>';
        }
        tds += '<td data-col="Action">';
        if (obj.result.data[i].uri !== '') {
            tds += '<a href="#" class="material-icons color-darkgrey">' + ligatureMore + '</a>';
        }
        tds += '</td>';
        row.innerHTML = tds;
        if (i < tr.length) {
            activeRow = replaceTblRow(tr[i], row) === true ? i : activeRow;
        }
        else {
            tbody.append(row);
        }
    }
    let trLen = tr.length - 1;
    for (let i = trLen; i >= nrItems; i --) {
        tr[i].remove();
    }                    

    let colspan = settings['colsQueueLastPlayed'].length;
    colspan--;
    
    if (nrItems === 0) {
        tbody.innerHTML = '<tr><td><span class="material-icons">error_outline</span></td>' +
            '<td colspan="' + colspan + '">' + t('Empty list') + '</td></tr>';
    }

    if (navigate === true) {
        focusTable(activeRow);
    }

    setPagination(obj.result.totalEntities, obj.result.returnedEntities);
    document.getElementById('QueueLastPlayedList').classList.remove('opacity05');
}

//eslint-disable-next-line no-unused-vars
function queueSelectedItem(append) {
    let item = document.activeElement;
    if (item) {
        if (item.parentNode.parentNode.id === 'QueueCurrentList') {
            return;
        }
        if (append === true) {
            appendQueue(item.getAttribute('data-type'), item.getAttribute('data-uri'), item.getAttribute('data-name'));
        }
        else {
            replaceQueue(item.getAttribute('data-type'), item.getAttribute('data-uri'), item.getAttribute('data-name'));
        }
    }
}

//eslint-disable-next-line no-unused-vars
function dequeueSelectedItem() {
    let item = document.activeElement;
    if (item) {
        if (item.parentNode.parentNode.id !== 'QueueCurrentList') {
            return;
        }
        delQueueSong('single', item.getAttribute('data-trackid'));
    }
}

function appendQueue(type, uri, name) {
    switch(type) {
        case 'song':
        case 'dir':
            sendAPI("MPD_API_QUEUE_ADD_TRACK", {"uri": uri});
            showNotification(t('%{name} added to queue', {"name": name}), '', '', 'success');
            break;
        case 'plist':
            sendAPI("MPD_API_QUEUE_ADD_PLAYLIST", {"plist": uri});
            showNotification(t('%{name} added to queue', {"name": name}), '', '', 'success');
            break;
    }
}

//eslint-disable-next-line no-unused-vars
function appendAfterQueue(type, uri, to, name) {
    switch(type) {
        case 'song':
            sendAPI("MPD_API_QUEUE_ADD_TRACK_AFTER", {"uri": uri, "to": to});
            to++;
            showNotification(t('%{name} added to queue position %{to}', {"name": name, "to": to}), '', '', 'success');
            break;
    }
}

function replaceQueue(type, uri, name) {
    switch(type) {
        case 'song':
        case 'dir':
            sendAPI("MPD_API_QUEUE_REPLACE_TRACK", {"uri": uri});
            showNotification(t('Queue replaced with %{name}', {"name": name}), '', '', 'success');
            break;
        case 'plist':
            sendAPI("MPD_API_QUEUE_REPLACE_PLAYLIST", {"plist": uri});
            showNotification(t('Queue replaced with %{name}', {"name": name}), '', '', 'success');
            break;
    }
}

//eslint-disable-next-line no-unused-vars
function addToQueue() {
    let formOK = true;
    let inputAddToQueueQuantityEl = document.getElementById('inputAddToQueueQuantity');
    if (!validateInt(inputAddToQueueQuantityEl)) {
        formOK = false;
    }
    
    let selectAddToQueueMode = document.getElementById('selectAddToQueueMode');
    let jukeboxMode = selectAddToQueueMode.options[selectAddToQueueMode.selectedIndex].value

    let selectAddToQueuePlaylist = document.getElementById('selectAddToQueuePlaylist');
    let jukeboxPlaylist = selectAddToQueuePlaylist.options[selectAddToQueuePlaylist.selectedIndex].value;
    
    if (jukeboxMode === '1' && settings.featSearchwindow === false && jukeboxPlaylist === 'Database') {
        document.getElementById('warnJukeboxPlaylist2').classList.remove('hide');
        formOK = false;
    }
    
    if (formOK === true) {
        sendAPI("MPD_API_QUEUE_ADD_RANDOM", {
            "mode": jukeboxMode,
            "playlist": jukeboxPlaylist,
            "quantity": document.getElementById('inputAddToQueueQuantity').value
        });
        modalAddToQueue.hide();
    }
}

//eslint-disable-next-line no-unused-vars
function saveQueue() {
    let plName = document.getElementById('saveQueueName').value;
    if (validatePlname(plName) === true) {
        sendAPI("MPD_API_QUEUE_SAVE", {"plist": plName});
        modalSaveQueue.hide();
    }
    else {
        document.getElementById('saveQueueName').classList.add('is-invalid');
    }
}

function delQueueSong(mode, start, end) {
    if (mode === 'range') {
        sendAPI("MPD_API_QUEUE_RM_RANGE", {"start": start, "end": end});
    }
    else if (mode === 'single') {
        sendAPI("MPD_API_QUEUE_RM_TRACK", { "track": start});
    }
}

//eslint-disable-next-line no-unused-vars
function gotoPlayingSong() {
    let page = lastState.songPos < settings.maxElementsPerPage ? 0 : Math.floor(lastState.songPos / settings.maxElementsPerPage) * settings.maxElementsPerPage;
    console.log(page);
    gotoPage(page);
}
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

//eslint-disable-next-line no-unused-vars
function saveScript() {
    let formOK = true;
    
    let nameEl = document.getElementById('inputScriptName');
    if (!validatePlnameEl(nameEl)) {
        formOK = false;
    }
    
    let orderEl = document.getElementById('inputScriptOrder');
    if (!validateInt(orderEl)) {
        formOK = false;
    }
    
    if (formOK === true) {
        let args = [];
        let argSel = document.getElementById('selectScriptArguments');
        for (let i = 0; i < argSel.options.length; i++) {
            args.push(argSel.options[i].text);
        }
        sendAPI("MYMPD_API_SCRIPT_SAVE", {
            "oldscript": document.getElementById('inputOldScriptName').value,
            "script": nameEl.value,
            "order": parseInt(orderEl.value),
            "content": document.getElementById('textareaScriptContent').value,
            "arguments": args
            }, showListScripts, false);
    }
}

function addScriptArgument() {
    let el = document.getElementById('inputScriptArgument');
    if (validatePlnameEl(el)) {
        let o = document.createElement('option');
        o.text = el.value;
        document.getElementById('selectScriptArguments').appendChild(o);
        el.value = '';
    }
}

function removeScriptArgument(ev) {
    let el = document.getElementById('inputScriptArgument');
    el.value = ev.target.text;
    ev.target.remove();
    el.focus();  
}

//eslint-disable-next-line no-unused-vars
function showEditScript(script) {
    document.getElementById('listScripts').classList.remove('active');
    document.getElementById('editScript').classList.add('active');
    document.getElementById('listScriptsFooter').classList.add('hide');
    document.getElementById('editScriptFooter').classList.remove('hide');
    
    document.getElementById('inputScriptName').classList.remove('is-invalid');
    document.getElementById('inputScriptOrder').classList.remove('is-invalid');
    document.getElementById('inputScriptArgument').classList.remove('is-invalid');
    
    if (script !== '') {
        sendAPI("MYMPD_API_SCRIPT_GET", {"script": script}, parseEditScript, false);
    }
    else {
        document.getElementById('inputOldScriptName').value = '';
        document.getElementById('inputScriptName').value = '';
        document.getElementById('inputScriptOrder').value = '1';
        document.getElementById('inputScriptArgument').value = '';
        document.getElementById('selectScriptArguments').innerText = '';
        document.getElementById('textareaScriptContent').value = '';
    }
}

function parseEditScript(obj) {
    document.getElementById('inputOldScriptName').value = obj.result.script;
    document.getElementById('inputScriptName').value = obj.result.script;
    document.getElementById('inputScriptOrder').value = obj.result.metadata.order;
    document.getElementById('inputScriptArgument').value = '';
    let selSA = document.getElementById('selectScriptArguments');
    selSA.innerText = '';
    for (let i = 0; i < obj.result.metadata.arguments.length; i++) {
        let o = document.createElement('option');
        o.innerText = obj.result.metadata.arguments[i];
        selSA.appendChild(o);
    }
    document.getElementById('textareaScriptContent').value = obj.result.content;
}

function showListScripts() {
    document.getElementById('listScripts').classList.add('active');
    document.getElementById('editScript').classList.remove('active');
    document.getElementById('listScriptsFooter').classList.remove('hide');
    document.getElementById('editScriptFooter').classList.add('hide');
    sendAPI("MYMPD_API_SCRIPT_LIST", {"all": true}, parseScriptList);
}

function deleteScript(script) {
    sendAPI("MYMPD_API_SCRIPT_DELETE", {"script": script}, function() {
        getScriptList(true);
    }, false);
}

function getScriptList(all) {
    sendAPI("MYMPD_API_SCRIPT_LIST", {"all": all}, parseScriptList, false);
}

function parseScriptList(obj) {
    let timerActions = document.createElement('optgroup');
    timerActions.setAttribute('data-value', 'script');
    timerActions.setAttribute('label', t('Script'));
    let scriptMaxListLen = 4;
    let scriptListMain = ''; //list in main menu
    let scriptList = ''; //list in scripts dialog
    let scriptListLen = obj.result.data.length;
    if (scriptListLen > 0) {
        obj.result.data.sort(function(a, b) {
            return a.metadata.order - b.metadata.order;
        });
        let mi = 0;
        for (let i = 0; i < scriptListLen; i++) {
            let arglist = '';
            if (obj.result.data[i].metadata.arguments.length > 0) {
                for (let j = 0; j < obj.result.data[i].metadata.arguments.length; j++) {
                    obj.result.data[i].metadata.arguments[j] = e(obj.result.data[i].metadata.arguments[j]);
                }
                arglist = '"' + obj.result.data[i].metadata.arguments.join('","') + '"';
            }
            if (obj.result.data[i].metadata.order > 0) {
                if (mi === 0) {
                    scriptListMain = scriptListLen > scriptMaxListLen ? '' : '<div class="dropdown-divider"></div>';
                }
                mi++;
                
                scriptListMain += '<a class="dropdown-item text-light alwaysEnabled" href="#" data-href=\'{"script": "' + 
                    e(obj.result.data[i].name) + '", "arguments": [' + arglist + ']}\'>' + e(obj.result.data[i].name) + '</a>';
                
            }
            scriptList += '<tr data-script="' + encodeURI(obj.result.data[i].name) + '"><td>' + e(obj.result.data[i].name) + '</td>' +
                '<td data-col="Action">' +
                    (settings.featScripteditor === true ? 
                        '<a href="#" title="' + t('Delete') + '" data-action="delete" class="material-icons color-darkgrey">delete</a>' : '') +
                        '<a href="#" title="' + t('Execute') + '" data-action="execute" class="material-icons color-darkgrey" ' +
                            ' data-href=\'{"script": "' + e(obj.result.data[i].name) + '", "arguments": [' + arglist + ']}\'>play_arrow</a>' +
                        '<a href="#" title="' + t('Add to homescreen') + '" data-action="add2home" class="material-icons color-darkgrey" ' +
                            ' data-href=\'{"script": "' + e(obj.result.data[i].name) + '", "arguments": [' + arglist + ']}\'>add_to_home_screen</a>' +
                '</td></tr>';
            timerActions.innerHTML += '<option data-arguments=\'{"arguments":[' + arglist + ']}\' value="' + 
                e(obj.result.data[i].name) + '">' + e(obj.result.data[i].name) + '</option>';
        }
        document.getElementById('listScriptsList').innerHTML = scriptList;
    }
    else {
        document.getElementById('listScriptsList').innerHTML = '<tr class="not-clickable"><td><span class="material-icons">error_outline</span></td>' +
            '<td colspan="2">' + t('Empty list') + '</td></tr>';
    }
    document.getElementById('scripts').innerHTML = scriptListMain;
        
    if (scriptListLen > scriptMaxListLen) {
        document.getElementById('navScripting').classList.remove('hide');
        document.getElementById('scripts').classList.add('collapse', 'menu-indent');
    }
    else {
        document.getElementById('navScripting').classList.add('hide');
        document.getElementById('scripts').classList.remove('collapse', 'menu-indent');
    }

    document.getElementById('selectTriggerScript').innerHTML = timerActions.innerHTML;
    
    let old = document.getElementById('selectTimerAction').querySelector('optgroup[data-value="script"]');
    if (old) {
        old.replaceWith(timerActions);
    }
    else {
        document.getElementById('selectTimerAction').appendChild(timerActions);
    }
}

//eslint-disable-next-line no-unused-vars
function execScriptFromOptions(cmd, options) {
    let args = options !== undefined ? options.split(',') : [];
    let script = {"script": cmd, "arguments": args};
    execScript(JSON.stringify(script));
}

function execScript(href) {
    let cmd = JSON.parse(href);
    if (cmd.arguments.length === 0) {
        sendAPI("MYMPD_API_SCRIPT_EXECUTE", {"script": cmd.script, "arguments": {}});
    }
    else {
        let arglist ='';
        for (let i = 0; i < cmd.arguments.length; i++) {
            arglist += '<div class="form-group row">' +
                  '<label class="col-sm-4 col-form-label" for="inputScriptArg' + i + '">' + e(cmd.arguments[i]) +'</label>' +
                  '<div class="col-sm-8">' +
                     '<input name="' + e(cmd.arguments[i]) + '" id="inputScriptArg' + i + '" type="text" class="form-control border-secondary" value="">' +
                  '</div>' +
                '</div>';

        }
        document.getElementById('execScriptArguments').innerHTML = arglist;
        document.getElementById('modalExecScriptScriptname').value = cmd.script;
        modalExecScript.show();
    }
}

//eslint-disable-next-line no-unused-vars
function execScriptArgs() {
    let script = document.getElementById('modalExecScriptScriptname').value;
    let args = {};
    let inputs = document.getElementById('execScriptArguments').getElementsByTagName('input');
    for (let i = 0; i < inputs.length; i++) {
        args[inputs[i].name] = inputs[i].value;
    }
    sendAPI("MYMPD_API_SCRIPT_EXECUTE", {"script": script, "arguments": args});
    modalExecScript.hide();
}
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

function doSearch(x) {
    if (settings.featAdvsearch) {
        let expression = '(';
        let crumbs = domCache.searchCrumb.children;
        for (let i = 0; i < crumbs.length; i++) {
            expression += '(' + decodeURI(crumbs[i].getAttribute('data-filter')) + ')';
            if (x !== '') {
                expression += ' AND ';
            }
        }
        if (x !== '') {
            let match = document.getElementById('searchMatch');
            expression += '(' + app.current.filter + ' ' + match.options[match.selectedIndex].value + ' \'' + x +'\'))';
        }
        else {
            expression += ')';
        }
        if (expression.length <= 2) {
            expression = '';
        }
        appGoto('Search', undefined, undefined, '0', app.current.filter, app.current.sort, '-', expression);
    }
    else {
        appGoto('Search', undefined, undefined, '0', app.current.filter, app.current.sort, '-', x);
    }
}

function parseSearch(obj) {
    //document.getElementById('panel-heading-search').innerText = gtPage('Num songs', obj.result.returnedEntities, obj.result.totalEntities);
    //document.getElementById('cardFooterSearch').innerText = gtPage('Num songs', obj.result.returnedEntities, obj.result.totalEntities);
    
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
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

//eslint-disable-next-line no-unused-vars
function saveConnection() {
    let formOK = true;
    let mpdHostEl = document.getElementById('inputMpdHost');
    let mpdPortEl = document.getElementById('inputMpdPort');
    let mpdPassEl = document.getElementById('inputMpdPass');
    let musicDirectoryEl  = document.getElementById('selectMusicDirectory');
    let musicDirectory = musicDirectoryEl.options[musicDirectoryEl.selectedIndex].value;
    
    if (musicDirectory === 'custom') {
        let musicDirectoryValueEl  = document.getElementById('inputMusicDirectory');
        if (!validatePath(musicDirectoryValueEl)) {
            formOK = false;        
        }
        musicDirectory = musicDirectoryValueEl.value;
    }    
    
    if (mpdPortEl.value === '') {
        mpdPortEl.value = '6600';
    }
    if (mpdHostEl.value.indexOf('/') !== 0) {
        if (!validateInt(mpdPortEl)) {
            formOK = false;        
        }
        if (!validateHost(mpdHostEl)) {
            formOK = false;        
        }
    }
    if (formOK === true) {
        sendAPI("MYMPD_API_CONNECTION_SAVE", {
            "mpdHost": mpdHostEl.value,
            "mpdPort": mpdPortEl.value,
            "mpdPass": mpdPassEl.value,
            "musicDirectory": musicDirectory
        }, getSettings);
        modalConnection.hide();    
    }
}

function getSettings(onerror) {
    if (settingsLock === false) {
        settingsLock = true;
        sendAPI("MYMPD_API_SETTINGS_GET", {}, getMpdSettings, onerror);
    }
}

function getMpdSettings(obj) {
    if (obj !== '' && obj.result) {
        settingsNew = obj.result;
        document.getElementById('splashScreenAlert').innerText = t('Fetch MPD settings');
        sendAPI("MPD_API_SETTINGS_GET", {}, joinSettings, true);
    }
    else {
        settingsParsed = 'error';
        if (appInited === false) {
            showAppInitAlert(obj === '' ? t('Can not parse settings') : t(obj.error.message));
        }
        return false;
    }
}

function joinSettings(obj) {
    if (obj !== '' && obj.result) {
        for (let key in obj.result) {
            settingsNew[key] = obj.result[key];
        }
    }
    else {
        settingsParsed = 'error';
        if (appInited === false) {
            showAppInitAlert(obj === '' ? t('Can not parse settings') : t(obj.error.message));
        }
        settingsNew.mpdConnected = false;
    }
    settings = Object.assign({}, settingsNew);
    settingsLock = false;
    parseSettings();
    toggleUI();
    if (settings.mpdConnected === true) {
        sendAPI("MPD_API_URLHANDLERS", {}, parseUrlhandlers,false);
    }
    btnWaiting(document.getElementById('btnApplySettings'), false);
}

function parseUrlhandlers(obj) {
    let storagePlugins = '';
    for (let i = 0; i < obj.result.data.length; i++) {
        switch(obj.result.data[i]) {
            case 'http://':
            case 'https://':
            case 'nfs://':
            case 'smb://':
                storagePlugins += '<option value="' + obj.result.data[i] + '">' + obj.result.data[i] + '</option>';
                break;
        }
    }
    document.getElementById('selectMountUrlhandler').innerHTML = storagePlugins;
}

function checkConsume() {
    let stateConsume = document.getElementById('btnConsume').classList.contains('active') ? true : false;
    let stateJukeboxMode = getBtnGroupValue('btnJukeboxModeGroup');
    if (stateJukeboxMode > 0 && stateConsume === false) {
        document.getElementById('warnConsume').classList.remove('hide');
    }
    else {
        document.getElementById('warnConsume').classList.add('hide');
    }
}

function parseSettings() {
    if (settings.locale === 'default') {
        locale = navigator.language || navigator.userLanguage;
    }
    else {
        locale = settings.locale;
    }
    
    if (isMobile === true) {    
        document.getElementById('inputScaleRatio').value = scale;
    }

    let setTheme = settings.theme;
    if (settings.theme === 'theme-autodetect') {
        setTheme = window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches ? 'theme-dark' : 'theme-default';
    }    

    Object.keys(themes).forEach(function(key) {
        if (key === setTheme) {
            domCache.body.classList.add(key);
        }
        else {
            domCache.body.classList.remove(key);
        }
    });
    
    document.getElementById('selectTheme').value = settings.theme;
    
    if (settings.mpdConnected === true) {
        parseMPDSettings();
    }
    
    if (settings.mpdHost.indexOf('/') !== 0) {
        document.getElementById('mpdInfo_host').innerText = settings.mpdHost + ':' + settings.mpdPort;
    }
    else {
        document.getElementById('mpdInfo_host').innerText = settings.mpdHost;
    }
    
    document.getElementById('inputMpdHost').value = settings.mpdHost;
    document.getElementById('inputMpdPort').value = settings.mpdPort;
    document.getElementById('inputMpdPass').value = settings.mpdPass;

    let btnNotifyWeb = document.getElementById('btnNotifyWeb');
    document.getElementById('warnNotifyWeb').classList.add('hide');
    if (notificationsSupported()) {
        if (Notification.permission !== 'granted') {
            if (settings.notificationWeb === true) {
                document.getElementById('warnNotifyWeb').classList.remove('hide');
            }
            settings.notificationWeb = false;
        }
        if (Notification.permission === 'denied') {
            document.getElementById('warnNotifyWeb').classList.remove('hide');
        }
        toggleBtnChk('btnNotifyWeb', settings.notificationWeb);
        btnNotifyWeb.removeAttribute('disabled');
    }
    else {
        btnNotifyWeb.setAttribute('disabled', 'disabled');
        toggleBtnChk('btnNotifyWeb', false);
    }
    
    toggleBtnChk('btnNotifyPage', settings.notificationPage);
    toggleBtnChk('btnMediaSession', settings.mediaSession);
    toggleBtnChkCollapse('btnFeatLocalplayer', 'collapseLocalplayer', settings.featLocalplayer);
    toggleBtnChk('btnFeatTimer', settings.featTimer);
    toggleBtnChk('btnBookmarks', settings.featBookmarks);
    toggleBtnChk('btnFeatLyrics', settings.featLyrics);

    if (settings.streamUrl === '') {
        document.getElementById('selectStreamMode').value = 'port';
        document.getElementById('inputStreamUrl').value = settings.streamPort;
    }
    else {
        document.getElementById('selectStreamMode').value = 'url';
        document.getElementById('inputStreamUrl').value = settings.streamUrl;
    }
    toggleBtnChkCollapse('btnCoverimage', 'collapseAlbumart', settings.coverimage);

    document.getElementById('inputBookletName').value = settings.bookletName;
    
    document.getElementById('selectLocale').value = settings.locale;
    document.getElementById('inputCoverimageName').value = settings.coverimageName;

    document.getElementById('inputCoverimageSize').value = settings.coverimageSize;
    document.getElementById('inputCoverimageSizeSmall').value = settings.coverimageSizeSmall;

    document.documentElement.style.setProperty('--mympd-coverimagesize', settings.coverimageSize + "px");
    document.documentElement.style.setProperty('--mympd-coverimagesizesmall', settings.coverimageSizeSmall + "px");
    document.documentElement.style.setProperty('--mympd-highlightcolor', settings.highlightColor);
    
    document.getElementById('inputHighlightColor').value = settings.highlightColor;
    document.getElementById('inputBgColor').value = settings.bgColor;
    document.getElementsByTagName('body')[0].style.backgroundColor = settings.bgColor;
    
    document.getElementById('highlightColorPreview').style.backgroundColor = settings.highlightColor;
    document.getElementById('bgColorPreview').style.backgroundColor = settings.bgColor;

    toggleBtnChkCollapse('btnBgCover', 'collapseBackground', settings.bgCover);
    document.getElementById('inputBgCssFilter').value = settings.bgCssFilter;    

    let albumartbg = document.querySelectorAll('.albumartbg');
    for (let i = 0; i < albumartbg.length; i++) {
	albumartbg[i].style.filter = settings.bgCssFilter;
    }

    toggleBtnChkCollapse('btnLoveEnable', 'collapseLove', settings.love);
    document.getElementById('inputLoveChannel').value = settings.loveChannel;
    document.getElementById('inputLoveMessage').value = settings.loveMessage;
    
    document.getElementById('inputMaxElementsPerPage').value = settings.maxElementsPerPage;
    toggleBtnChk('btnStickers', settings.stickers);
    document.getElementById('inputLastPlayedCount').value = settings.lastPlayedCount;
    
    toggleBtnChkCollapse('btnSmartpls', 'collapseSmartpls', settings.smartpls);
    
    let features = ["featLocalplayer", "featSyscmds", "featMixramp", "featCacert", "featBookmarks", 
        "featRegex", "featTimer", "featLyrics", "featScripting", "featScripteditor", "featHome"];
    for (let j = 0; j < features.length; j++) {
        let Els = document.getElementsByClassName(features[j]);
        let ElsLen = Els.length;
        let displayEl = settings[features[j]] === true ? '' : 'none';
        for (let i = 0; i < ElsLen; i++) {
            Els[i].style.display = displayEl;
        }
    }
    
    let readonlyEls = document.getElementsByClassName('warnReadonly');
    for (let i = 0; i < readonlyEls.length; i++) {
        if (settings.readonly === false) {
            readonlyEls[i].classList.add('hide');
        }
        else {
            readonlyEls[i].classList.remove('hide');
        }
    }
    if (settings.readonly === true) {
        document.getElementById('btnBookmarks').setAttribute('disabled', 'disabled');
        document.getElementsByClassName('groupClearCovercache')[0].classList.add('hide');
    }
    else {
        document.getElementById('btnBookmarks').removeAttribute('disabled');
        document.getElementsByClassName('groupClearCovercache')[0].classList.remove('hide');
    }
    
    let timerActions = '<optgroup data-value="player" label="' + t('Playback') + '">' +
        '<option value="startplay">' + t('Start playback') + '</option>' +
        '<option value="stopplay">' + t('Stop playback') + '</option>' +
        '</optgroup>';

    if (settings.featSyscmds === true) {
        let syscmdsMaxListLen = 4;
        let syscmdsList = '';
        let syscmdsListLen = settings.syscmdList.length;
        if (syscmdsListLen > 0) {
            timerActions += '<optgroup data-value="syscmd" label="' + t('System command') + '">';
            syscmdsList = syscmdsListLen > syscmdsMaxListLen ? '' : '<div class="dropdown-divider"></div>';
            for (let i = 0; i < syscmdsListLen; i++) {
                if (settings.syscmdList[i] === 'HR') {
                    syscmdsList += '<div class="dropdown-divider"></div>';
                }
                else {
                    syscmdsList += '<a class="dropdown-item text-light alwaysEnabled" href="#" data-href=\'{"cmd": "execSyscmd", "options": ["' + 
                        e(settings.syscmdList[i]) + '"]}\'>' + e(settings.syscmdList[i]) + '</a>';
                    timerActions += '<option value="' + e(settings.syscmdList[i]) + '">' + e(settings.syscmdList[i]) + '</option>';
                }
            }
        }
        document.getElementById('syscmds').innerHTML = syscmdsList;
        timerActions += '</optgroup>';
        
        if (syscmdsListLen > syscmdsMaxListLen) {
            document.getElementById('navSyscmds').classList.remove('hide');
            document.getElementById('syscmds').classList.add('collapse', 'menu-indent');
        }
        else {
            document.getElementById('navSyscmds').classList.add('hide');
            document.getElementById('syscmds').classList.remove('collapse', 'menu-indent');
        }
    }
    else {
        document.getElementById('syscmds').innerHTML = '';
    }

    if (settings.featScripting === true) {
        getScriptList(true);
    }
    else {
        document.getElementById('scripts').innerHTML = '';
    }

    document.getElementById('selectTimerAction').innerHTML = timerActions;
    
    //dropdownMainMenu = new BSN.Dropdown(document.getElementById('mainMenu'));
    
    toggleBtnGroupValueCollapse(document.getElementById('btnJukeboxModeGroup'), 'collapseJukeboxMode', settings.jukeboxMode);
    document.getElementById('selectJukeboxUniqueTag').value = settings.jukeboxUniqueTag;
    document.getElementById('inputJukeboxQueueLength').value = settings.jukeboxQueueLength;
    document.getElementById('inputJukeboxLastPlayed').value = settings.jukeboxLastPlayed;
    
    if (settings.jukeboxMode === 0) {
        document.getElementById('inputJukeboxQueueLength').setAttribute('disabled', 'disabled');
        document.getElementById('selectJukeboxPlaylist').setAttribute('disabled', 'disabled');
    }
    else if (settings.jukeboxMode === 2) {
        document.getElementById('inputJukeboxQueueLength').setAttribute('disabled', 'disabled');
        document.getElementById('selectJukeboxPlaylist').setAttribute('disabled', 'disabled');
        document.getElementById('selectJukeboxPlaylist').value = 'Database';
    }
    else if (settings.jukeboxMode === 1) {
        document.getElementById('inputJukeboxQueueLength').removeAttribute('disabled');
        document.getElementById('selectJukeboxPlaylist').removeAttribute('disabled');
    }

    document.getElementById('inputSmartplsPrefix').value = settings.smartplsPrefix;
    document.getElementById('inputSmartplsInterval').value = settings.smartplsInterval / 60 / 60;
    document.getElementById('selectSmartplsSort').value = settings.smartplsSort;

    if (settings.featLocalplayer === true) {
        setLocalPlayerUrl();
    }
    
    if (settings.musicDirectory === 'auto') {
        document.getElementById('selectMusicDirectory').value = settings.musicDirectory;
        document.getElementById('inputMusicDirectory').value = settings.musicDirectoryValue !== undefined ? settings.musicDirectoryValue : '';
        document.getElementById('inputMusicDirectory').setAttribute('readonly', 'readonly');
    }
    else if (settings.musicDirectory === 'none') {
        document.getElementById('selectMusicDirectory').value = settings.musicDirectory;
        document.getElementById('inputMusicDirectory').value = '';
        document.getElementById('inputMusicDirectory').setAttribute('readonly', 'readonly');
    }
    else {
        document.getElementById('selectMusicDirectory').value = 'custom';
        document.getElementById('inputMusicDirectory').value = settings.musicDirectoryValue;
        document.getElementById('inputMusicDirectory').removeAttribute('readonly');
    }

    //update columns
    if (app.current.app === 'Queue' && app.current.tab === 'Current') {
        getQueue();
    }
    else if (app.current.app === 'Queue' && app.current.tab === 'LastPlayed') {
        appRoute();
    }
    else if (app.current.app === 'Queue' && app.current.tab === 'Jukebox') {
        appRoute();
    }
    else if (app.current.app === 'Search') {
        appRoute();
    }
    else if (app.current.app === 'Browse' && app.current.tab === 'Filesystem') {
        appRoute();
    }
    else if (app.current.app === 'Browse' && app.current.tab === 'Playlists' && app.current.view === 'Detail') {
        appRoute();
    }
    else if (app.current.app === 'Browse' && app.current.tab === 'Database' && app.current.search !== '') {
        appRoute();
    }

    i18nHtml(document.getElementsByTagName('body')[0]);

    checkConsume();

    if (settings.mediaSession === true && 'mediaSession' in navigator) {
        navigator.mediaSession.setActionHandler('play', clickPlay);
        navigator.mediaSession.setActionHandler('pause', clickPlay);
        navigator.mediaSession.setActionHandler('stop', clickStop);
        navigator.mediaSession.setActionHandler('seekbackward', seekRelativeBackward);
        navigator.mediaSession.setActionHandler('seekforward', seekRelativeForward);
        navigator.mediaSession.setActionHandler('previoustrack', clickPrev);
        navigator.mediaSession.setActionHandler('nexttrack', clickNext);
        
        if (!navigator.mediaSession.setPositionState) {
            logDebug('mediaSession.setPositionState not supported by browser');
        }
    }
    else {
        logDebug('mediaSession not supported by browser');
    }

    settingsParsed = 'true';
}

function parseMPDSettings() {
    toggleBtnChk('btnRandom', settings.random);
    toggleBtnChk('btnConsume', settings.consume);
    toggleBtnChk('btnRepeat', settings.repeat);
    toggleBtnChk('btnAutoPlay', settings.autoPlay);

    toggleBtnGroupValue(document.getElementById('btnSingleGroup'), settings.single);
    toggleBtnGroupValue(document.getElementById('btnReplaygainGroup'), settings.replaygain);

    document.getElementById('partitionName').innerText = settings.partition;
    
    document.getElementById('inputCrossfade').value = settings.crossfade;
    document.getElementById('inputMixrampdb').value = settings.mixrampdb;
    document.getElementById('inputMixrampdelay').value = settings.mixrampdelay;
    
    if (settings.featLibrary === true && settings.publish === true) {
        settings['featBrowse'] = true;    
    }
    else {
        settings['featBrowse'] = false;
    }

    let features = ['featStickers', 'featSmartpls', 'featPlaylists', 'featTags', 'featCoverimage', 'featAdvsearch',
        'featLove', 'featSingleOneshot', 'featBrowse', 'featMounts', 'featNeighbors',
        'featPartitions'];
    for (let j = 0; j < features.length; j++) {
        let Els = document.getElementsByClassName(features[j]);
        let ElsLen = Els.length;
        let displayEl = settings[features[j]] === true ? '' : 'none';
        if (features[j] === 'featCoverimage' && settings.coverimage === false) {
            displayEl = 'none';
        }
        for (let i = 0; i < ElsLen; i++) {
            Els[i].style.display = displayEl;
        }
    }
    
    if (settings.featPlaylists === false && settings.smartpls === true) {
        document.getElementById('warnSmartpls').classList.remove('hide');
    }
    else {
        document.getElementById('warnSmartpls').classList.add('hide');
    }
    
    if (settings.featPlaylists === true && settings.readonly === false) {
        document.getElementById('btnSmartpls').removeAttribute('disabled');
    }
    else {
        document.getElementById('btnSmartpls').setAttribute('disabled', 'disabled');
    }

    if (settings.featStickers === false && settings.stickers === true) {
        document.getElementById('warnStickers').classList.remove('hide');
    }
    else {
        document.getElementById('warnStickers').classList.add('hide');
    }
    
    if (settings.featStickers === false || settings.stickers === false || settings.featStickerCache === false) {
        document.getElementById('warnPlaybackStatistics').classList.remove('hide');
        document.getElementById('inputJukeboxLastPlayed').setAttribute('disabled', 'disabled');
    }
    else {
        document.getElementById('warnPlaybackStatistics').classList.add('hide');
        document.getElementById('inputJukeboxLastPlayed').removeAttribute('disabled');
    }
    
    if (settings.featLove === false && settings.love === true) {
        document.getElementById('warnScrobbler').classList.remove('hide');
    }
    else {
        document.getElementById('warnScrobbler').classList.add('hide');
    }
    
    if (settings.featLibrary === false && settings.coverimage === true) {
        document.getElementById('warnAlbumart').classList.remove('hide');
    }
    else {
        document.getElementById('warnAlbumart').classList.add('hide');
    }
    if (settings.musicDirectoryValue === '' && settings.musicDirectory !== 'none') {
        document.getElementById('warnMusicDirectory').classList.remove('hide');
    }
    else {
        document.getElementById('warnMusicDirectory').classList.add('hide');
    }

    document.getElementById('warnJukeboxPlaylist').classList.add('hide');

    if (settings.bgCover === true && settings.featCoverimage === true && settings.coverimage === true) {
        setBackgroundImage(lastSongObj.uri);
    }
    else {
        clearBackgroundImage();
    }

    let triggerEventList = '';
    Object.keys(settings.triggers).forEach(function(key) {
        triggerEventList += '<option value="' + e(settings.triggers[key]) + '">' + t(key) + '</option>';
    });
    document.getElementById('selectTriggerEvent').innerHTML = triggerEventList;
    
    settings.tags.sort();
    settings.searchtags.sort();
    settings.browsetags.sort();
    filterCols('colsSearch');
    filterCols('colsQueueCurrent');
    filterCols('colsQueueLastPlayed');
    filterCols('colsQueueJukebox');
    filterCols('colsBrowsePlaylistsDetail');
    filterCols('colsBrowseFilesystem');
    filterCols('colsBrowseDatabaseDetail');
    filterCols('colsPlayback');
    
    if (settings.featTags === false) {
        app.apps.Browse.active = 'Filesystem';
        app.apps.Search.state = '0/filename/-/-/';
        app.apps.Queue.state = '0/filename/-/-/';
        settings.colsQueueCurrent = ["Pos", "Title", "Duration"];
        settings.colsQueueLastPlayed = ["Pos", "Title", "LastPlayed"];
        settings.colsQueueJukebox = ["Pos", "Title"];
        settings.colsSearch = ["Title", "Duration"];
        settings.colsBrowseFilesystem = ["Type", "Title", "Duration"];
        settings.colsBrowseDatabase = ["Track", "Title", "Duration"];
        settings.colsPlayback = [];
    }
    else {
        let pbtl = '';
        for (let i = 0; i < settings.colsPlayback.length; i++) {
            pbtl += '<div id="current' + settings.colsPlayback[i]  + '" data-tag="' + settings.colsPlayback[i] + '" ' +
                    (settings.colsPlayback[i] === 'Lyrics' ? '' : 'data-name="' + (lastSongObj[settings.colsPlayback[i]] ? encodeURI(lastSongObj[settings.colsPlayback[i]]) : '') + '"') +
                    '>' +
                    '<small>' + t(settings.colsPlayback[i]) + '</small>' +
                    '<p';
            if (settings.browsetags.includes(settings.colsPlayback[i])) {
                pbtl += ' class="clickable"';
            }
            pbtl += '>';
            if (settings.colsPlayback[i] === 'Duration') {
                pbtl += (lastSongObj[settings.colsPlayback[i]] ? beautifySongDuration(lastSongObj[settings.colsPlayback[i]]) : '');
            }
            else if (settings.colsPlayback[i] === 'LastModified') {
                pbtl += (lastSongObj[settings.colsPlayback[i]] ? localeDate(lastSongObj[settings.colsPlayback[i]]) : '');
            }
            else if (settings.colsPlayback[i] === 'Fileformat') {
                pbtl += (lastState ? fileformat(lastState.audioFormat) : '');
            }
            else {
                pbtl += (lastSongObj[settings.colsPlayback[i]] ? e(lastSongObj[settings.colsPlayback[i]]) : '');
            }
            pbtl += '</p></div>';
        }
        document.getElementById('cardPlaybackTags').innerHTML = pbtl;
        let cl = document.getElementById('currentLyrics');
        if (cl && lastSongObj.uri) {
            let el = cl.getElementsByTagName('small')[0];
            el.classList.add('clickable');
            el.addEventListener('click', function(event) {
                event.target.parentNode.children[1].classList.toggle('expanded');
            }, false);
            getLyrics(lastSongObj.uri, cl.getElementsByTagName('p')[0]);
        }
    }

    if (settings.tags.includes('Title')) {
        app.apps.Search.state = '0/any/Title/-/';
    }
    
    if (!settings.tags.includes('AlbumArtist')) {
        app.apps.Browse.tabs.Database.state = '0/Artist/Artist/' + settings.dbDefaultTag + '/';
    }
    
    if (settings.featPlaylists === true) {
        sendAPI("MPD_API_PLAYLIST_LIST_ALL", {"searchstr": ""}, function(obj) {
            getAllPlaylists(obj, 'selectJukeboxPlaylist', settings.jukeboxPlaylist);
        });
    }
    else {
        document.getElementById('selectJukeboxPlaylist').innerHTML = '<option value="Database">' + t('Database') + '</option>';
    }

    setCols('QueueCurrent');
    setCols('Search');
    setCols('QueueLastPlayed');
    setCols('QueueJukebox');
    setCols('BrowseFilesystem');
    setCols('BrowsePlaylistsDetail');
    setCols('BrowseDatabaseDetail');
    setCols('Playback');

    addTagList('BrowseDatabaseByTagDropdown', 'browsetags');
    addTagList('BrowseNavPlaylistsDropdown', 'browsetags');
    addTagList('BrowseNavFilesystemDropdown', 'browsetags');
    
    addTagList('searchqueuetags', 'searchtags');
    addTagList('searchtags', 'searchtags');
    addTagList('searchDatabaseTags', 'browsetags');
    addTagList('databaseSortTagsList', 'browsetags');
    addTagList('dropdownSortPlaylistTags', 'tags');
    addTagList('saveSmartPlaylistSort', 'tags');
    
    addTagListSelect('selectSmartplsSort', 'tags');
    addTagListSelect('saveSmartPlaylistSort', 'tags');
    addTagListSelect('selectJukeboxUniqueTag', 'browsetags');
    
    initTagMultiSelect('inputEnabledTags', 'listEnabledTags', settings.allmpdtags, settings.tags);
    initTagMultiSelect('inputSearchTags', 'listSearchTags', settings.tags, settings.searchtags);
    initTagMultiSelect('inputBrowseTags', 'listBrowseTags', settings.tags, settings.browsetags);
    initTagMultiSelect('inputGeneratePlsTags', 'listGeneratePlsTags', settings.browsetags, settings.generatePlsTags);
}

//eslint-disable-next-line no-unused-vars
function resetSettings() {
    sendAPI("MYMPD_API_SETTINGS_RESET", {}, getSettings);
}

//eslint-disable-next-line no-unused-vars
function saveSettings(closeModal) {
    let formOK = true;

    let inputCrossfade = document.getElementById('inputCrossfade');
    if (!inputCrossfade.getAttribute('disabled')) {
        if (!validateInt(inputCrossfade)) {
            formOK = false;
        }
    }

    let inputJukeboxQueueLength = document.getElementById('inputJukeboxQueueLength');
    if (!validateInt(inputJukeboxQueueLength)) {
        formOK = false;
    }

    let inputJukeboxLastPlayed = document.getElementById('inputJukeboxLastPlayed');
    if (!validateInt(inputJukeboxLastPlayed)) {
        formOK = false;
    }
    
    let selectStreamModeEl = document.getElementById('selectStreamMode');
    let streamUrl = '';
    let streamPort = '';
    let inputStreamUrl = document.getElementById('inputStreamUrl');
    if (selectStreamModeEl.options[selectStreamModeEl.selectedIndex].value === 'port') {
        streamPort = inputStreamUrl.value;
        if (!validateInt(inputStreamUrl)) {
            formOK = false;
        }
    }
    else {
        streamUrl = inputStreamUrl.value;
        if (!validateStream(inputStreamUrl)) {
            formOK = false;
        }
    }

    let inputCoverimageSizeSmall = document.getElementById('inputCoverimageSizeSmall');
    if (!validateInt(inputCoverimageSizeSmall)) {
        formOK = false;
    }

    let inputCoverimageSize = document.getElementById('inputCoverimageSize');
    if (!validateInt(inputCoverimageSize)) {
        formOK = false;
    }
    
    let inputCoverimageName = document.getElementById('inputCoverimageName');
    if (!validateFilenameList(inputCoverimageName)) {
        formOK = false;
    }
    
    let inputBookletName = document.getElementById('inputBookletName');
    if (!validateFilename(inputBookletName)) {
        formOK = false;
    }
    
    let inputMaxElementsPerPage = document.getElementById('inputMaxElementsPerPage');
    if (!validateInt(inputMaxElementsPerPage)) {
        formOK = false;
    }
    
    if (isMobile === true) {
        let inputScaleRatio = document.getElementById('inputScaleRatio');
        if (!validateFloat(inputScaleRatio)) {
            formOK = false;
        }
        else {
            scale = parseFloat(inputScaleRatio.value);
            setViewport(true);
        }
    }

    if (parseInt(inputMaxElementsPerPage.value) > 200) {
        formOK = false;
    }
    
    let inputLastPlayedCount = document.getElementById('inputLastPlayedCount');
    if (!validateInt(inputLastPlayedCount)) {
        formOK = false;
    }
    
    if (document.getElementById('btnLoveEnable').classList.contains('active')) {
        let inputLoveChannel = document.getElementById('inputLoveChannel');
        let inputLoveMessage = document.getElementById('inputLoveMessage');
        if (!validateNotBlank(inputLoveChannel) || !validateNotBlank(inputLoveMessage)) {
            formOK = false;
        }
    }

    if (settings.featMixramp === true) {
        let inputMixrampdb = document.getElementById('inputMixrampdb');
        if (!inputMixrampdb.getAttribute('disabled')) {
            if (!validateFloat(inputMixrampdb)) {
                formOK = false;
            } 
        }
        let inputMixrampdelay = document.getElementById('inputMixrampdelay');
        if (!inputMixrampdelay.getAttribute('disabled')) {
            if (inputMixrampdelay.value === 'nan') {
                inputMixrampdelay.value = '-1';
            }
            if (!validateFloat(inputMixrampdelay)) {
                formOK = false;
            }
        }
    }
    
    let inputSmartplsInterval = document.getElementById('inputSmartplsInterval');
    if (!validateInt(inputSmartplsInterval)) {
        formOK = false;
    }
    let smartplsInterval = document.getElementById('inputSmartplsInterval').value * 60 * 60;

    let singleState = getBtnGroupValue('btnSingleGroup');
    let jukeboxMode = getBtnGroupValue('btnJukeboxModeGroup');
    let replaygain = getBtnGroupValue('btnReplaygainGroup');
    let jukeboxUniqueTag = document.getElementById('selectJukeboxUniqueTag');
    let jukeboxUniqueTagValue = jukeboxUniqueTag.options[jukeboxUniqueTag.selectedIndex].value;

    let selectJukeboxPlaylist = document.getElementById('selectJukeboxPlaylist');
    let jukeboxPlaylist = selectJukeboxPlaylist.options[selectJukeboxPlaylist.selectedIndex].value;
    
    if (jukeboxMode === '2') {
        jukeboxUniqueTagValue = 'Album';
    }
    
    if (jukeboxMode === '1' && settings.featSearchwindow === false && jukeboxPlaylist === 'Database') {
        formOK = false;
        document.getElementById('warnJukeboxPlaylist').classList.remove('hide');
    }
    
    if (formOK === true) {
        let selectLocale = document.getElementById('selectLocale');
        let selectTheme = document.getElementById('selectTheme');
        sendAPI("MYMPD_API_SETTINGS_SET", {
            "consume": (document.getElementById('btnConsume').classList.contains('active') ? 1 : 0),
            "random": (document.getElementById('btnRandom').classList.contains('active') ? 1 : 0),
            "single": parseInt(singleState),
            "repeat": (document.getElementById('btnRepeat').classList.contains('active') ? 1 : 0),
            "replaygain": replaygain,
            "crossfade": document.getElementById('inputCrossfade').value,
            "mixrampdb": (settings.featMixramp === true ? document.getElementById('inputMixrampdb').value : settings.mixrampdb),
            "mixrampdelay": (settings.featMixramp === true ? document.getElementById('inputMixrampdelay').value : settings.mixrampdelay),
            "notificationWeb": (document.getElementById('btnNotifyWeb').classList.contains('active') ? true : false),
            "notificationPage": (document.getElementById('btnNotifyPage').classList.contains('active') ? true : false),
            "mediaSession": (document.getElementById('btnMediaSession').classList.contains('active') ? true : false),
            "jukeboxMode": parseInt(jukeboxMode),
            "jukeboxPlaylist": jukeboxPlaylist,
            "jukeboxQueueLength": parseInt(document.getElementById('inputJukeboxQueueLength').value),
            "jukeboxLastPlayed": parseInt(document.getElementById('inputJukeboxLastPlayed').value),
            "jukeboxUniqueTag": jukeboxUniqueTagValue,
            "autoPlay": (document.getElementById('btnAutoPlay').classList.contains('active') ? true : false),
            "bgCover": (document.getElementById('btnBgCover').classList.contains('active') ? true : false),
            "bgColor": document.getElementById('inputBgColor').value,
            "bgCssFilter": document.getElementById('inputBgCssFilter').value,
            "featLocalplayer": (document.getElementById('btnFeatLocalplayer').classList.contains('active') ? true : false),
            "streamUrl": streamUrl,
            "streamPort": parseInt(streamPort),
            "coverimage": (document.getElementById('btnCoverimage').classList.contains('active') ? true : false),
            "coverimageName": document.getElementById('inputCoverimageName').value,
            "coverimageSize": document.getElementById('inputCoverimageSize').value,
            "coverimageSizeSmall": document.getElementById('inputCoverimageSizeSmall').value,
            "locale": selectLocale.options[selectLocale.selectedIndex].value,
            "love": (document.getElementById('btnLoveEnable').classList.contains('active') ? true : false),
            "loveChannel": document.getElementById('inputLoveChannel').value,
            "loveMessage": document.getElementById('inputLoveMessage').value,
            "bookmarks": (document.getElementById('btnBookmarks').classList.contains('active') ? true : false),
            "maxElementsPerPage": document.getElementById('inputMaxElementsPerPage').value,
            "stickers": (document.getElementById('btnStickers').classList.contains('active') ? true : false),
            "lastPlayedCount": document.getElementById('inputLastPlayedCount').value,
            "smartpls": (document.getElementById('btnSmartpls').classList.contains('active') ? true : false),
            "smartplsPrefix": document.getElementById('inputSmartplsPrefix').value,
            "smartplsInterval": smartplsInterval,
            "smartplsSort": document.getElementById('selectSmartplsSort').value,
            "taglist": getTagMultiSelectValues(document.getElementById('listEnabledTags'), false),
            "searchtaglist": getTagMultiSelectValues(document.getElementById('listSearchTags'), false),
            "browsetaglist": getTagMultiSelectValues(document.getElementById('listBrowseTags'), false),
            "generatePlsTags": getTagMultiSelectValues(document.getElementById('listGeneratePlsTags'), false),
            "theme": selectTheme.options[selectTheme.selectedIndex].value,
            "highlightColor": document.getElementById('inputHighlightColor').value,
            "timer": (document.getElementById('btnFeatTimer').classList.contains('active') ? true : false),
            "bookletName": document.getElementById('inputBookletName').value,
            "lyrics": (document.getElementById('btnFeatLyrics').classList.contains('active') ? true : false)
        }, getSettings);
        if (closeModal === true) {
            modalSettings.hide();
        }
        else {
            btnWaiting(document.getElementById('btnApplySettings'), true);
        }
    }
}

function getTagMultiSelectValues(taglist, translated) {
    let values = [];
    let chkBoxes = taglist.getElementsByTagName('button');
    for (let i = 0; i < chkBoxes.length; i++) {
        if (chkBoxes[i].classList.contains('active')) {
            if (translated === true) {
                values.push(t(chkBoxes[i].name));
            }
            else {
                values.push(chkBoxes[i].name);
            }
        }
    }
    if (translated === true) {
        return values.join(', ');
    }
    return values.join(',');
}

function initTagMultiSelect(inputId, listId, allTags, enabledTags) {
    let values = [];
    let list = '';
    for (let i = 0; i < allTags.length; i++) {
        if (enabledTags.includes(allTags[i])) {
            values.push(t(allTags[i]));
        }
        list += '<div class="form-check">' +
            '<button class="btn btn-secondary btn-xs clickable material-icons material-icons-small' + 
            (enabledTags.includes(allTags[i]) ? ' active' : '') + '" name="' + allTags[i] + '">' +
            (enabledTags.includes(allTags[i]) ? 'check' : 'radio_button_unchecked') + '</button>' +
            '<label class="form-check-label" for="' + allTags[i] + '">&nbsp;&nbsp;' + t(allTags[i]) + '</label>' +
            '</div>';
    }
    document.getElementById(listId).innerHTML = list;

    let inputEl = document.getElementById(inputId);
    inputEl.value = values.join(', ');
    if (inputEl.getAttribute('data-init') === 'true') {
        return;
    }
    inputEl.setAttribute('data-init', 'true');
    document.getElementById(listId).addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (event.target.nodeName === 'BUTTON') {
            toggleBtnChk(event.target);
            event.target.parentNode.parentNode.parentNode.previousElementSibling.value = getTagMultiSelectValues(event.target.parentNode.parentNode, true);
        }
    });
}

function filterCols(x) {
    let tags = settings.tags.slice();
    if (settings.featTags === false) {
        tags.push('Title');
    }
    tags.push('Duration');
    if (x === 'colsQueueCurrent' || x === 'colsBrowsePlaylistsDetail' || x === 'colsQueueLastPlayed' || x === 'colsQueueJukebox') {
        tags.push('Pos');
    }
    else if (x === 'colsBrowseFilesystem') {
        tags.push('Type');
    }
    if (x === 'colsQueueLastPlayed') {
        tags.push('LastPlayed');
    }
    if (x === 'colsPlayback') {
        tags.push('Filetype');
        tags.push('Fileformat');
        tags.push('LastModified');
        if (settings.featLyrics === true) {
            tags.push('Lyrics');
        }
    }
    let cols = [];
    for (let i = 0; i < settings[x].length; i++) {
        if (tags.includes(settings[x][i])) {
            cols.push(settings[x][i]);
        }
    }
    settings[x] = cols;
    logDebug('Columns for ' + x + ': ' + cols);
}

//eslint-disable-next-line no-unused-vars
function toggleBtnNotifyWeb() {
    let btnNotifyWeb = document.getElementById('btnNotifyWeb');
    let notifyWebState = btnNotifyWeb.classList.contains('active') ? true : false;
    if (notificationsSupported()) {
        if (notifyWebState === false) {
            Notification.requestPermission(function (permission) {
                if (!('permission' in Notification)) {
                    Notification.permission = permission;
                }
                if (permission === 'granted') {
                    toggleBtnChk('btnNotifyWeb', true);
                    settings.notificationWeb = true;
                    document.getElementById('warnNotifyWeb').classList.add('hide');
                } 
                else {
                    toggleBtnChk('btnNotifyWeb', false);
                    settings.notificationWeb = false;
                    document.getElementById('warnNotifyWeb').classList.remove('hide');
                }
            });
        }
        else {
            toggleBtnChk('btnNotifyWeb', false);
            settings.notificationWeb = false;
            document.getElementById('warnNotifyWeb').classList.add('hide');
        }
    }
    else {
        toggleBtnChk('btnNotifyWeb', false);
        settings.notificationWeb = false;
    }
}

//eslint-disable-next-line no-unused-vars
function setPlaySettings(el) {
    if (el.parentNode.classList.contains('btn-group')) {
        toggleBtnGroup(el);
    }
    else {
        toggleBtnChk(el);
    }
    if (el.parentNode.id === 'playDropdownBtnJukeboxModeGroup') {
        if (el.parentNode.getElementsByClassName('active')[0].getAttribute('data-value') !== '0') {
            toggleBtnChk('playDropdownBtnConsume', true);            
        }
    }
    else if (el.id === 'playDropdownBtnConsume') {
        if (el.classList.contains('active') === false) {
            toggleBtnGroupValue(document.getElementById('playDropdownBtnJukeboxModeGroup'), 0);
        }
    }

    savePlaySettings();
}

function showPlayDropdown() {
    toggleBtnChk(document.getElementById('playDropdownBtnRandom'), settings.random);
    toggleBtnChk(document.getElementById('playDropdownBtnConsume'), settings.consume);
    toggleBtnChk(document.getElementById('playDropdownBtnRepeat'), settings.repeat);
    toggleBtnChk(document.getElementById('playDropdownBtnRandom'), settings.random);
    toggleBtnGroupValue(document.getElementById('playDropdownBtnSingleGroup'), settings.single);
    toggleBtnGroupValue(document.getElementById('playDropdownBtnJukeboxModeGroup'), settings.jukeboxMode);
}

function savePlaySettings() {
    let singleState = document.getElementById('playDropdownBtnSingleGroup').getElementsByClassName('active')[0].getAttribute('data-value');
    let jukeboxMode = document.getElementById('playDropdownBtnJukeboxModeGroup').getElementsByClassName('active')[0].getAttribute('data-value');
    sendAPI("MYMPD_API_SETTINGS_SET", {
        "consume": (document.getElementById('playDropdownBtnConsume').classList.contains('active') ? 1 : 0),
        "random": (document.getElementById('playDropdownBtnRandom').classList.contains('active') ? 1 : 0),
        "single": parseInt(singleState),
        "repeat": (document.getElementById('playDropdownBtnRepeat').classList.contains('active') ? 1 : 0),
        "jukeboxMode": parseInt(jukeboxMode)
        }, getSettings);
}
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

function songDetails(uri) {
    sendAPI("MPD_API_DATABASE_SONGDETAILS", {"uri": uri}, parseSongDetails);
    modalSongDetails.show();
}

function parseFingerprint(obj) {
    let textarea = document.createElement('textarea');
    textarea.value = obj.result.fingerprint;
    textarea.classList.add('form-control', 'text-monospace', 'small');
    let fpTd = document.getElementById('fingerprint');
    fpTd.innerHTML = '';
    fpTd.appendChild(textarea);
}

function parseSongDetails(obj) {
    let modal = document.getElementById('modalSongDetails');
    modal.getElementsByClassName('album-cover')[0].style.backgroundImage = 'url("' + subdir + '/albumart/' + obj.result.uri + '"), url("' + subdir + '/assets/coverimage-loading.svg")';
    
    let elH1s = modal.getElementsByTagName('h1');
    for (let i = 0; i < elH1s.length; i++) {
        elH1s[i].innerText = obj.result.Title;
    }
    
    let songDetailsHTML = '';
    for (let i = 0; i < settings.tags.length; i++) {
        if (settings.tags[i] === 'Title' || obj.result[settings.tags[i]] === '-') {
            continue;
        }
        songDetailsHTML += '<tr><th>' + t(settings.tags[i]) + '</th><td data-tag="' + settings.tags[i] + '" data-name="' + encodeURI(obj.result[settings.tags[i]]) + '">';
        if (settings.browsetags.includes(settings.tags[i]) && obj.result[settings.tags[i]] !== '-') {
            songDetailsHTML += '<a class="text-success" href="#">' + e(obj.result[settings.tags[i]]) + '</a>';
        }
        else {
            songDetailsHTML += obj.result[settings.tags[i]];
        }
        songDetailsHTML += '</td></tr>';
    }
    songDetailsHTML += '<tr><th>' + t('Duration') + '</th><td>' + beautifyDuration(obj.result.Duration) + '</td></tr>';
    if (settings.featLibrary === true && settings.publish === true) {
        songDetailsHTML += '<tr><th>' + t('Filename') + '</th><td><a class="breakAll text-success" href="/browse/music/' + 
            encodeURI(obj.result.uri) + '" target="_blank" title="' + e(obj.result.uri) + '">' + 
            e(basename(obj.result.uri, true)) + '</a></td></tr>';
    }
    else {
        songDetailsHTML += '<tr><th>' + t('Filename') + '</th><td class="breakAll"><span title="' + e(obj.result.uri) + '">' + 
            e(basename(obj.result.uri, true)) + '</span></td></tr>';
    }
    songDetailsHTML += '<tr><th>' + t('Filetype') + '</th><td>' + filetype(obj.result.uri) + '</td></tr>';
    songDetailsHTML += '<tr><th>' + t('LastModified') + '</th><td>' + localeDate(obj.result.LastModified) + '</td></tr>';
    if (settings.featFingerprint === true) {
        songDetailsHTML += '<tr><th>' + t('Fingerprint') + '</th><td class="breakAll" id="fingerprint"><a class="text-success" data-uri="' + 
            encodeURI(obj.result.uri) + '" id="calcFingerprint" href="#">' + t('Calculate') + '</a></td></tr>';
    }
    if (obj.result.bookletPath !== '' && settings.publish === true) {
        songDetailsHTML += '<tr><th>' + t('Booklet') + '</th><td><a class="text-success" href="' + subdir + '/browse/music/' + dirname(obj.result.uri) + '/' + settings.bookletName + '" target="_blank">' + t('Download') + '</a></td></tr>';
    }
    if (settings.featStickers === true) {
        songDetailsHTML += '<tr><th colspan="2" class="pt-3"><h5>' + t('Statistics') + '</h5></th></tr>' +
            '<tr><th>' + t('Play count') + '</th><td>' + obj.result.playCount + '</td></tr>' +
            '<tr><th>' + t('Skip count') + '</th><td>' + obj.result.skipCount + '</td></tr>' +
            '<tr><th>' + t('Last played') + '</th><td>' + (obj.result.lastPlayed === 0 ? t('never') : localeDate(obj.result.lastPlayed)) + '</td></tr>' +
            '<tr><th>' + t('Last skipped') + '</th><td>' + (obj.result.lastSkipped === 0 ? t('never') : localeDate(obj.result.lastSkipped)) + '</td></tr>' +
            '<tr><th>' + t('Like') + '</th><td>' +
              '<div class="btn-group btn-group-sm">' +
                '<button title="' + t('Dislike song') + '" id="btnVoteDown2" data-href=\'{"cmd": "voteSong", "options": [0]}\' class="btn btn-sm btn-light material-icons">thumb_down</button>' +
                '<button title="' + t('Like song') + '" id="btnVoteUp2" data-href=\'{"cmd": "voteSong", "options": [2]}\' class="btn btn-sm btn-light material-icons">thumb_up</button>' +
              '</div>' +
            '</td></tr>';
    }
    
    document.getElementById('tbodySongDetails').innerHTML = songDetailsHTML;
    setVoteSongBtns(obj.result.like, obj.result.uri);
    
    if (settings.featLyrics === true) {
        getLyrics(obj.result.uri, document.getElementById('lyricsText'));
    }

    let showPictures = false;
    if (obj.result.images.length > 0 && settings.featLibrary === true && settings.publish === true) {
        showPictures = true;
    }
    else if (settings.coverimage === true) {
        showPictures = true;
    }
    
    let pictureEls = document.getElementsByClassName('featPictures');
    for (let i = 0; i < pictureEls.length; i++) {
        if (showPictures === true) {
            pictureEls[i].classList.remove('hide');
        }
        else {
            pictureEls[i].classList.add('hide');
        }
    }
    
    if (showPictures === true) {
        //add uri to image list to get embedded albumart
        let images = [ subdir + '/albumart/' + obj.result.uri ];
        //add all but coverfiles to image list
        for (let i = 0; i < obj.result.images.length; i++) {
            if (isCoverfile(obj.result.images[i]) === false) {
                images.push(subdir + '/browse/music/' + obj.result.images[i]);
            }
        }
        const imgEl = document.getElementById('tabSongPics');
        createImgCarousel(imgEl, 'songPicsCarousel', images);
    }
    else {
        document.getElementById('tabSongPics').innerText = '';
    }
}

function isCoverfile(uri) {
    let filename = basename(uri).toLowerCase();
    let fileparts = filename.split('.');
    
    let extensions = ['png', 'jpg', 'jpeg', 'svg', 'webp', 'tiff', 'bmp'];
    let coverimageNames = settings.coverimageName.split(',');
    for (let i = 0; i < coverimageNames.length; i++) {
        let name = coverimageNames[i].trim();
        if (filename === name) {
            return true;
        }
        if (fileparts[1]) {
            if (name === fileparts[0] && extensions.includes(fileparts[1])) {
                return true;
            }
        }
    }
    return false;
}

function getLyrics(uri, el) {
    el.classList.add('opacity05');
    let ajaxRequest=new XMLHttpRequest();
    
    ajaxRequest.open('GET', subdir + '/lyrics/' + uri, true);
    ajaxRequest.onreadystatechange = function() {
        if (ajaxRequest.readyState === 4) {
            el.innerText = ajaxRequest.responseText === 'No lyrics found' ? t(ajaxRequest.responseText) : ajaxRequest.responseText;
            el.classList.remove('opacity05');
        }
    };
    ajaxRequest.send();
}

//eslint-disable-next-line no-unused-vars
function loveSong() {
    sendAPI("MPD_API_LOVE", {});
}

//eslint-disable-next-line no-unused-vars
function voteSong(vote) {
    let uri = decodeURI(domCache.currentTitle.getAttribute('data-uri'));
    if (uri === '') {
        return;
    }
        
    if (vote === 2 && domCache.btnVoteUp.classList.contains('highlight')) {
        vote = 1;
    }
    else if (vote === 0 && domCache.btnVoteDown.classList.contains('highlight')) {
        vote = 1;
    }
    sendAPI("MPD_API_LIKE", {"uri": uri, "like": vote});
    setVoteSongBtns(vote, uri);
}

function setVoteSongBtns(vote, uri) {
    domCache.btnVoteUp2 = document.getElementById('btnVoteUp2');
    domCache.btnVoteDown2 = document.getElementById('btnVoteDown2');

    if (uri === '' || uri.indexOf('://') > -1) {
        domCache.btnVoteUp.setAttribute('disabled', 'disabled');
        domCache.btnVoteDown.setAttribute('disabled', 'disabled');
        if (domCache.btnVoteUp2) {
            domCache.btnVoteUp2.setAttribute('disabled', 'disabled');
            domCache.btnVoteDown2.setAttribute('disabled', 'disabled');
        }
    } else {
        domCache.btnVoteUp.removeAttribute('disabled');
        domCache.btnVoteDown.removeAttribute('disabled');
        if (domCache.btnVoteUp2) {
            domCache.btnVoteUp2.removeAttribute('disabled');
            domCache.btnVoteDown2.removeAttribute('disabled');
        }
    }
    
    if (vote === 0) {
        domCache.btnVoteUp.classList.remove('highlight');
        domCache.btnVoteDown.classList.add('highlight');
        if (domCache.btnVoteUp2) {
            domCache.btnVoteUp2.classList.remove('highlight');
            domCache.btnVoteDown2.classList.add('highlight');
        }
    } else if (vote === 1) {
        domCache.btnVoteUp.classList.remove('highlight');
        domCache.btnVoteDown.classList.remove('highlight');
        if (domCache.btnVoteUp2) {
            domCache.btnVoteUp2.classList.remove('highlight');
            domCache.btnVoteDown2.classList.remove('highlight');
        }
    } else if (vote === 2) {
        domCache.btnVoteUp.classList.add('highlight');
        domCache.btnVoteDown.classList.remove('highlight');
        if (domCache.btnVoteUp2) {
            domCache.btnVoteUp2.classList.add('highlight');
            domCache.btnVoteDown2.classList.remove('highlight');
        }
    }
}
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

function parseStats(obj) {
    document.getElementById('mpdstats_artists').innerText =  obj.result.artists;
    document.getElementById('mpdstats_albums').innerText = obj.result.albums;
    document.getElementById('mpdstats_songs').innerText = obj.result.songs;
    document.getElementById('mpdstats_dbPlaytime').innerText = beautifyDuration(obj.result.dbPlaytime);
    document.getElementById('mpdstats_playtime').innerText = beautifyDuration(obj.result.playtime);
    document.getElementById('mpdstats_uptime').innerText = beautifyDuration(obj.result.uptime);
    document.getElementById('mpdstats_mympd_uptime').innerText = beautifyDuration(obj.result.myMPDuptime);
    document.getElementById('mpdstats_dbUpdated').innerText = localeDate(obj.result.dbUpdated);
    document.getElementById('mympdVersion').innerText = obj.result.mympdVersion;
    document.getElementById('mpdInfo_version').innerText = obj.result.mpdVersion;
    document.getElementById('mpdInfo_libmpdclientVersion').innerText = obj.result.libmpdclientVersion;
    document.getElementById('mpdInfo_libmympdclientVersion').innerText = obj.result.libmympdclientVersion;
}

function getServerinfo() {
    let ajaxRequest=new XMLHttpRequest();
    ajaxRequest.open('GET', subdir + '/api/serverinfo', true);
    ajaxRequest.onreadystatechange = function() {
        if (ajaxRequest.readyState === 4) {
            let obj = JSON.parse(ajaxRequest.responseText);
            document.getElementById('wsIP').innerText = obj.result.ip;
            document.getElementById('wsMongooseVersion').innerText = obj.result.version;
        }
    };
    ajaxRequest.send();
}

function parseOutputs(obj) {
    let btns = '';
    let nr = 0;
    for (let i = 0; i < obj.result.numOutputs; i++) {
        if (obj.result.data[i].plugin !== 'dummy') {
            nr++;
            btns += '<button id="btnOutput' + obj.result.data[i].id +'" data-output-name="' + encodeURI(obj.result.data[i].name) + '" data-output-id="' + obj.result.data[i].id + '" class="btn btn-secondary btn-block';
            if (obj.result.data[i].state === 1) {
                btns += ' active';
            }
            btns += '"><span class="material-icons float-left">volume_up</span> ' + e(obj.result.data[i].name);
            if (Object.keys(obj.result.data[i].attributes).length > 0) {
                btns += '<a class="material-icons float-right text-white" title="' + t('Edit attributes') + '">settings</a>';
            }
            else {
                btns += '<a class="material-icons float-right text-white" title="' + t('Show attributes') + '">settings</a>';
            }
            btns += '</button>';
        }
    }
    if (nr === 0) {
        btns = '<span class="material-icons">error_outline</span> ' + t('No outputs');
    }
    domCache.outputs.innerHTML = btns;
}

function showListOutputAttributes(outputName) {
    sendAPI("MPD_API_PLAYER_OUTPUT_LIST", {}, function(obj) {
        modalOutputAttributes.show();
        let output;
        for (let i = 0; i < obj.result.data.length; i++) {
            if (obj.result.data[i].name === outputName) {
                output = obj.result.data[i];
                break;
            }
        }
        document.getElementById('modalOutputAttributesId').value = e(output.id);        
        let list = '<tr><td>' + t('Name') + '</td><td>' + e(output.name) + '</td></tr>' +
            '<tr><td>' + t('State') + '</td><td>' + (output.state === 1 ? t('enabled') : t('disabled')) + '</td></tr>' +
            '<tr><td>' + t('Plugin') + '</td><td>' + e(output.plugin) + '</td></tr>';
        let i = 0;
        Object.keys(output.attributes).forEach(function(key) {
            i++;
            list += '<tr><td>' + e(key) + '</td><td><input name="' + e(key) + '" class="form-control border-secondary" type="text" value="' + 
                e(output.attributes[key]) + '"/></td></tr>';
        });
        if (i > 0) {
            document.getElementById('btnOutputAttributesSave').removeAttribute('disabled');
        }
        else {
            document.getElementById('btnOutputAttributesSave').setAttribute('disabled', 'disabled');
        }
        document.getElementById('outputAttributesList').innerHTML = list;
    });
}

//eslint-disable-next-line no-unused-vars
function saveOutputAttributes() {
    let params = {};
    params.outputId =  parseInt(document.getElementById('modalOutputAttributesId').value);
    params.attributes = {};
    let el = document.getElementById('outputAttributesList').getElementsByTagName('input');
    for (let i = 0; i < el.length; i++) {
        params.attributes[el[i].name] = el[i].value;
    }
    sendAPI('MPD_API_PLAYER_OUTPUT_ATTRIBUTS_SET', params);
    modalOutputAttributes.hide();
}

function setCounter(currentSongId, totalTime, elapsedTime) {
    currentSong.totalTime = totalTime;
    currentSong.elapsedTime = elapsedTime;
    currentSong.currentSongId = currentSongId;

    const progressPx = totalTime > 0 ? Math.ceil(domCache.progress.offsetWidth * elapsedTime / totalTime) : 0;
    if (progressPx === 0) {
        domCache.progressBar.style.transition = 'none';
    }
    domCache.progressBar.style.width = progressPx + 'px'; 
    if (progressPx === 0) {    
        setTimeout(function() {
            domCache.progressBar.style.transition = progressBarTransition;
        }, 10);
    }
    
    if (totalTime <= 0) {
        domCache.progress.style.cursor = 'default';
    }
    else {
        domCache.progress.style.cursor = 'pointer';
    }

    let counterText = beautifySongDuration(elapsedTime) + "&nbsp;/&nbsp;" + beautifySongDuration(totalTime);
    domCache.counter.innerHTML = counterText;
    
    //Set playing track in queue view
    if (lastState) {
        if (lastState.currentSongId !== currentSongId) {
            let tr = document.getElementById('queueTrackId' + lastState.currentSongId);
            if (tr) {
                let durationTd = tr.querySelector('[data-col=Duration]');
                if (durationTd) {
                    durationTd.innerText = tr.getAttribute('data-duration');
                }
                let posTd = tr.querySelector('[data-col=Pos]');
                if (posTd) {
                    posTd.classList.remove('material-icons');
                    posTd.innerText = tr.getAttribute('data-songpos');
                }
                tr.classList.remove('font-weight-bold');
            }
        }
    }
    let tr = document.getElementById('queueTrackId' + currentSongId);
    if (tr) {
        let durationTd = tr.querySelector('[data-col=Duration]');
        if (durationTd) {
            durationTd.innerHTML = counterText;
        }
        let posTd = tr.querySelector('[data-col=Pos]');
        if (posTd) {
            if (!posTd.classList.contains('material-icons')) {
                posTd.classList.add('material-icons');
                posTd.innerText = 'play_arrow';
            }
        }
        tr.classList.add('font-weight-bold');
    }
    
    if (progressTimer) {
        clearTimeout(progressTimer);
    }
    if (playstate === 'play') {
        progressTimer = setTimeout(function() {
            currentSong.elapsedTime ++;
            requestAnimationFrame(function() {
                setCounter(currentSong.currentSongId, currentSong.totalTime, currentSong.elapsedTime);
            });
        }, 1000);
    }
}

function parseState(obj) {
    if (JSON.stringify(obj.result) === JSON.stringify(lastState)) {
        toggleUI();
        return;
    }

    //Set play and queue state
    parseUpdateQueue(obj);
    
    //Set volume
    parseVolume(obj);

    //Set play counters
    setCounter(obj.result.currentSongId, obj.result.totalTime, obj.result.elapsedTime);
    
    //Get current song
    if (!lastState || lastState.currentSongId !== obj.result.currentSongId ||
        lastState.queueVersion !== obj.result.queueVersion)
    {
        sendAPI("MPD_API_PLAYER_CURRENT_SONG", {}, songChange);
    }
    //clear playback card if no current song
    if (obj.result.songPos === '-1') {
        domCache.currentTitle.innerText = 'Not playing';
        document.title = 'myMPD';
        domCache.footerTitle.innerText = '';
        domCache.footerTitle.removeAttribute('title');
        domCache.footerTitle.classList.remove('clickable');
        clearCurrentCover();
        if (settings.bgCover === true) {
            clearBackgroundImage();
        }
        let pb = document.getElementById('cardPlaybackTags').getElementsByTagName('p');
        for (let i = 0; i < pb.length; i++) {
            pb[i].innerText = '';
        }
    }
    else {
        let cff = document.getElementById('currentFileformat');
        if (cff) {
            cff.getElementsByTagName('p')[0].innerText = fileformat(obj.result.audioFormat);
        }
    }

    lastState = obj.result;                    
    
    if (settings.mpdConnected === false || uiEnabled === false) {
        getSettings(true);
    }
}

function parseVolume(obj) {
    if (obj.result.volume === -1) {
        domCache.volumePrct.innerText = t('Volumecontrol disabled');
        domCache.volumeControl.classList.add('hide');
    } 
    else {
        domCache.volumeControl.classList.remove('hide');
        domCache.volumePrct.innerText = obj.result.volume + ' %';
        if (obj.result.volume === 0) {
            domCache.volumeMenu.firstChild.innerText = 'volume_off';
        }
        else if (obj.result.volume < 50) {
            domCache.volumeMenu.firstChild.innerText = 'volume_down';
        }
        else {
            domCache.volumeMenu.firstChild.innerText = 'volume_up';
        }
    }
    domCache.volumeBar.value = obj.result.volume;
}

function setBackgroundImage(url) {
    if (url === undefined) {
        clearBackgroundImage();
        return;
    }
    let old = document.querySelectorAll('.albumartbg');
    for (let i = 0; i < old.length; i++) {
        if (old[i].style.zIndex === '-10') {
            old[i].remove();
        }
        else {
            old[i].style.zIndex = '-10';
            old[i].style.opacity = '0';
            //old[i].style.filter = '';
        }
    }
    let div = document.createElement('div');
    div.classList.add('albumartbg');
    div.style.filter = settings.bgCssFilter;
    div.style.backgroundImage = 'url("' + subdir + '/albumart/' + url + '")';
    div.style.opacity = 0;
    let body = document.getElementsByTagName('body')[0];
    body.insertBefore(div, body.firstChild);

    let img = new Image();
    img.onload = function() {
        document.querySelector('.albumartbg').style.opacity = 1;
    };
    img.src = subdir + '/albumart/' + url;
}

function clearBackgroundImage() {
    let old = document.querySelectorAll('.albumartbg');
    for (let i = 0; i < old.length; i++) {
        if (old[i].style.zIndex === '-10') {
            old[i].remove();        
        }
        else {
            old[i].style.zIndex = '-10';
            old[i].style.opacity = '0';
            //old[i].style.filter = '';
        }
    }
}

function setCurrentCover(url) {
    _setCurrentCover(url, domCache.currentCover);
    _setCurrentCover(url, domCache.footerCover);
}

function _setCurrentCover(url, el) {
    if (url === undefined) {
        clearCurrentCover();
        return;
    }
    let old = el.querySelectorAll('.coverbg');
    for (let i = 0; i < old.length; i++) {
        if (old[i].style.zIndex === '2') {
            old[i].remove();        
        }
        else {
            old[i].style.zIndex = '2';
        }
    }

    let div = document.createElement('div');
    div.classList.add('coverbg', 'carousel');
    div.style.backgroundImage = 'url("' + subdir + '/albumart/' + url + '")';
    div.style.opacity = 0;
    el.insertBefore(div, el.firstChild);

    let img = new Image();
    img.onload = function() {
        el.querySelector('.coverbg').style.opacity = 1;
    };
    img.src = subdir + '/albumart/' + url;
}

function clearCurrentCover() {
    _clearCurrentCover(domCache.currentCover);
    _clearCurrentCover(domCache.footerCover);
}

function _clearCurrentCover(el) {
    let old = el.querySelectorAll('.coverbg');
    for (let i = 0; i < old.length; i++) {
        if (old[i].style.zIndex === '2') {
            old[i].remove();        
        }
        else {
            old[i].style.zIndex = '2';
            old[i].style.opacity = '0';
        }
    }
}

function songChange(obj) {
    let curSong = obj.result.Title + ':' + obj.result.Artist + ':' + obj.result.Album + ':' + obj.result.uri + ':' + obj.result.currentSongId;
    if (lastSong === curSong) {
        return;
    }
    let textNotification = '';
    let htmlNotification = '';
    let pageTitle = '';

    mediaSessionSetMetadata(obj.result.Title, obj.result.Artist, obj.result.Album, obj.result.uri);
    
    setCurrentCover(obj.result.uri);
    if (settings.bgCover === true && settings.featCoverimage === true) {
        setBackgroundImage(obj.result.uri);
    }

    if (obj.result.Artist !== undefined && obj.result.Artist.length > 0 && obj.result.Artist !== '-') {
        textNotification += obj.result.Artist;
        htmlNotification += obj.result.Artist;
        pageTitle += obj.result.Artist + ' - ';
        domCache.footerArtist.innerText = obj.result.Artist;
        domCache.footerArtist.setAttribute('data-name', encodeURI(obj.result.Artist));
        domCache.footerArtist.classList.add('clickable');
    }
    else {
        domCache.footerArtist.innerText = '';
        domCache.footerArtist.setAttribute('data-name', '');
        domCache.footerArtist.classList.remove('clickable');
    }

    if (obj.result.Album !== undefined && obj.result.Album.length > 0 && obj.result.Album !== '-') {
        textNotification += ' - ' + obj.result.Album;
        htmlNotification += '<br/>' + obj.result.Album;
        domCache.footerAlbum.innerText = obj.result.Album;
        domCache.footerAlbum.setAttribute('data-name', encodeURI(obj.result.Album));
        domCache.footerAlbum.classList.add('clickable');
    }
    else {
        domCache.footerAlbum.innerText = '';
        domCache.footerAlbum.setAttribute('data-name', '');
        domCache.footerAlbum.classList.remove('clickable');
    }

    if (obj.result.Title !== undefined && obj.result.Title.length > 0) {
        pageTitle += obj.result.Title;
        domCache.currentTitle.innerText = obj.result.Title;
        domCache.currentTitle.setAttribute('data-uri', encodeURI(obj.result.uri));
        domCache.footerTitle.innerText = obj.result.Title;
        domCache.footerTitle.classList.add('clickable');
    }
    else {
        domCache.currentTitle.innerText = '';
        domCache.currentTitle.setAttribute('data-uri', '');
        domCache.footerTitle.innerText = '';
        domCache.footerTitle.setAttribute('data-name', '');
        domCache.footerTitle.classList.remove('clickable');        
    }
    document.title = 'myMPD: ' + pageTitle;
    domCache.footerCover.title = pageTitle;
    
    if (obj.result.uri !== undefined && obj.result.uri !== '' && obj.result.uri.indexOf('://') === -1) {
        domCache.footerTitle.classList.add('clickable');
    }
    else {
        domCache.footerTitle.classList.remove('clickable');
    }

    if (obj.result.uri !== undefined) {
        if (settings.featStickers === true) {
            setVoteSongBtns(obj.result.like, obj.result.uri);
        }
        obj.result['Filetype'] = filetype(obj.result.uri);
    }
    else {
        obj.result['Filetype'] = '';
    }
    
    if (lastState) {
        obj.result['Fileformat'] = fileformat(lastState.audioFormat);
    }
    else {
        obj.result['Fileformat'] = '';
    }

    for (let i = 0; i < settings.colsPlayback.length; i++) {
        let c = document.getElementById('current' + settings.colsPlayback[i]);
        if (c && settings.colsPlayback[i] === 'Lyrics') {
            getLyrics(obj.result.uri, c.getElementsByTagName('p')[0]);
        }
        else if (c) {
            let value = obj.result[settings.colsPlayback[i]];
            if (value === undefined) {
                value = '';
            }
            if (settings.colsPlayback[i] === 'Duration') {
                value = beautifySongDuration(value);
            }
            else if (settings.colsPlayback[i] === 'LastModified') {
                value = localeDate(value);
            }
            c.getElementsByTagName('p')[0].innerText = value;
            c.setAttribute('data-name', encodeURI(value));
        }
    }
    
    document.getElementById('currentBooklet').innerHTML = obj.result.bookletPath === '' || obj.result.bookletPath === undefined|| settings.featBrowse === false ? '' : 
            '<span class="text-light material-icons">description</span>&nbsp;<a class="text-light" target="_blank" href="' + subdir + '/browse/music/' + 
            e(obj.result.bookletPath) + '">' + t('Download booklet') + '</a>';
    
    //Update Artist in queue view for http streams
    let playingTr = document.getElementById('queueTrackId' + obj.result.currentSongId);
    if (playingTr) {
        playingTr.getElementsByTagName('td')[1].innerText = obj.result.Title;
    }

    if (playstate === 'play') {
        showNotification(obj.result.Title, textNotification, htmlNotification, 'success');
    }
    
    lastSong = curSong;
    lastSongObj = obj.result;
}

//eslint-disable-next-line no-unused-vars
function gotoTagList() {
    appGoto(app.current.app, app.current.tab, app.current.view, '0', '-', '-', '-', '');
}

//eslint-disable-next-line no-unused-vars
function volumeStep(dir) {
    let inc = dir === 'up' ? settings.volumeStep : 0 - settings.volumeStep;
    chVolume(inc);
}

function chVolume(increment) {
    let newValue = parseInt(domCache.volumeBar.value) + increment;
    if (newValue < 0)  {
        newValue = 0;
    }
    else if (newValue > 100) {
        newValue = 100;
    }
    domCache.volumeBar.value = newValue;
    sendAPI("MPD_API_PLAYER_VOLUME_SET", {"volume": newValue});
}

//eslint-disable-next-line no-unused-vars
function clickTitle() {
    let uri = decodeURI(domCache.currentTitle.getAttribute('data-uri'));
    if (uri !== '' && uri.indexOf('://') === -1) {
        songDetails(uri);
    }
}

function mediaSessionSetPositionState(duration, position) {
    if (settings.mediaSession === true && 'mediaSession' in navigator && navigator.mediaSession.setPositionState) {
        if (position < duration) {
            //streams have position > duration
            navigator.mediaSession.setPositionState({
                duration: duration,
                position: position
            });
        }
    }
}

function mediaSessionSetState() {
    if (settings.mediaSession === true && 'mediaSession' in navigator) {
        if (playstate === 'play') {
            navigator.mediaSession.playbackState = 'playing';
        }
        else {
            navigator.mediaSession.playbackState = 'paused';
        }
    }
}

function mediaSessionSetMetadata(title, artist, album, url) {
    if (settings.mediaSession === true && 'mediaSession' in navigator) {
        let hostname = window.location.hostname;
        let protocol = window.location.protocol;
        let port = window.location.port;
        let artwork = protocol + '//' + hostname + (port !== '' ? ':' + port : '') + subdir + '/albumart/' + url;

        if (settings.coverimage === true) {
            //eslint-disable-next-line no-undef
            navigator.mediaSession.metadata = new MediaMetadata({
                title: title,
                artist: artist,
                album: album,
                artwork: [{src: artwork}]
            });
        }
        else {
            //eslint-disable-next-line no-undef
            navigator.mediaSession.metadata = new MediaMetadata({
                title: title,
                artist: artist,
                album: album
            });
        }
    }
}
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

function focusTable(rownr, table) {
    if (table === undefined) {
        table = document.getElementById(app.current.app + (app.current.tab !== undefined ? app.current.tab : '') + (app.current.view !== undefined ? app.current.view : '') + 'List');
    }

    if (app.current.app === 'Browse' && app.current.tab === 'Database' && app.current.view === 'List') {
        const tables = document.getElementsByClassName('card-grid');
        if (tables.length === 0 ) {
            return; 
        }
        table = tables[0];
        for (let i = 0; i < tables.length; i++) {
            if (tables[i].classList.contains('selected')) {
                table = tables[i];
                break;
            }
        }
        table.focus();
        return;
    }
    else if (app.current.app === 'Home') {
        const tables = document.getElementsByClassName('home-icons');
        if (tables.length === 0 ) {
            return; 
        }
        table = tables[0];
        for (let i = 0; i < tables.length; i++) {
            if (tables[i].classList.contains('selected')) {
                table = tables[i];
                break;
            }
        }
        table.focus();
        return;
    }

    if (table !== null) {
        let sel = table.getElementsByClassName('selected');
        if (rownr === undefined) {
            if (sel.length === 0) {
                let row = table.getElementsByTagName('tbody')[0].rows[0];
                if (row.classList.contains('not-clickable')) {
                    row = table.getElementsByTagName('tbody')[0].rows[1];
                }
                row.focus();
                row.classList.add('selected');
            }
            else {
                sel[0].focus();
            }
        }
        else {
            if (sel && sel.length > 0) {
                sel[0].classList.remove('selected');
            }
            let rows = table.getElementsByTagName('tbody')[0].rows;
            let rowsLen = rows.length;
            if (rowsLen < rownr) {
                rownr = 0;
            }
            if (rowsLen > rownr) {
                rows[rownr].focus();
                rows[rownr].classList.add('selected');
            }
        }
        //insert goto parent row
        if (table.id === 'BrowseFilesystemList') {
            let tbody = table.getElementsByTagName('tbody')[0];
            if (tbody.rows.length > 0 && tbody.rows[0].getAttribute('data-type') !== 'parentDir' && app.current.search !== '') {
                let nrCells = table.getElementsByTagName('thead')[0].rows[0].cells.length;
                let uri = app.current.search.replace(/\/?([^/]+)$/,'');
                let row = tbody.insertRow(0);
                row.setAttribute('data-type', 'parentDir');
                row.setAttribute('tabindex', 0);
                row.setAttribute('data-uri', encodeURI(uri));
                row.innerHTML = '<td colspan="' + nrCells + '">..</td>';
            }
        }
        scrollFocusIntoView();
    }
}

function scrollFocusIntoView() {
    let el = document.activeElement;
    let posY = el.getBoundingClientRect().top;
    let height = el.offsetHeight;
    let headerHeight = el.parentNode.parentNode.offsetTop;
    if (window.innerHeight > window.innerWidth) {
        headerHeight += domCache.header.offsetHeight;
    }
    let footerHeight = domCache.footer.offsetHeight;
    let parentHeight = window.innerHeight - headerHeight - footerHeight;
    let treshold = height / 2;
    //console.log('posY: ' + posY);
    //console.log('height: ' + height);
    //console.log('treshold: ' + treshold);
    //console.log('parentHeight: ' + parentHeight);
    //console.log('headerHeight:' + headerHeight);
    //console.log('footerHeight:' + footerHeight);
    if (posY <= headerHeight + treshold) {
        //console.log('0, - height');
        window.scrollBy(0, - height);
    }
    else if (posY + height > parentHeight - treshold) {
        //console.log('0, height');
        window.scrollBy(0, height);
    }
}

function navigateTable(table, keyCode) {
    let cur = document.activeElement;
    if (cur) {
        let next = null;
        let handled = false;
        if (keyCode === 'ArrowDown') {
            next = cur.nextElementSibling;
            if (next.classList.contains('not-clickable')) {
                next = next.nextElementSibling;
            }
            handled = true;
        }
        else if (keyCode === 'ArrowUp') {
            next = cur.previousElementSibling;
            if (next.classList.contains('not-clickable')) {
                next = next.previousElementSibling;
            }
            handled = true;
        }
        else if (keyCode === ' ') {
            let popupBtn = cur.lastChild.firstChild;
            if (popupBtn.nodeName === 'A') {
                popupBtn.click();
            }
            handled = true;
        }
        else if (keyCode === 'Enter') {
            cur.firstChild.click();
            handled = true;
        }
        else if (keyCode === 'Escape') {
            cur.blur();
            cur.classList.remove('selected');
            handled = true;
        }
        if (handled === true) {
            event.preventDefault();
            event.stopPropagation();
        }
        if (next) {
            cur.classList.remove('selected');
            next.classList.add('selected');
            next.focus();
            scrollFocusIntoView();
        }
    }
}

function dragAndDropTable(table) {
    let tableBody = document.getElementById(table).getElementsByTagName('tbody')[0];
    tableBody.addEventListener('dragstart', function(event) {
        if (event.target.nodeName === 'TR') {
            event.target.classList.add('opacity05');
            event.dataTransfer.setDragImage(event.target, 0, 0);
            event.dataTransfer.effectAllowed = 'move';
            event.dataTransfer.setData('Text', event.target.getAttribute('id'));
            dragEl = event.target.cloneNode(true);
        }
    }, false);
    tableBody.addEventListener('dragleave', function(event) {
        event.preventDefault();
        if (dragEl.nodeName !== 'TR') {
            return;
        }
        let target = event.target;
        if (event.target.nodeName === 'TD') {
            target = event.target.parentNode;
        }
        if (target.nodeName === 'TR') {
            target.classList.remove('dragover');
        }
    }, false);
    tableBody.addEventListener('dragover', function(event) {
        event.preventDefault();
        if (dragEl.nodeName !== 'TR') {
            return;
        }
        let tr = tableBody.getElementsByClassName('dragover');
        let trLen = tr.length;
        for (let i = 0; i < trLen; i++) {
            tr[i].classList.remove('dragover');
        }
        let target = event.target;
        if (event.target.nodeName === 'TD') {
            target = event.target.parentNode;
        }
        if (target.nodeName === 'TR') {
            target.classList.add('dragover');
        }
        event.dataTransfer.dropEffect = 'move';
    }, false);
    tableBody.addEventListener('dragend', function(event) {
        event.preventDefault();
        if (dragEl.nodeName !== 'TR') {
            return;
        }
        let tr = tableBody.getElementsByClassName('dragover');
        let trLen = tr.length;
        for (let i = 0; i < trLen; i++) {
            tr[i].classList.remove('dragover');
        }
        if (document.getElementById(event.dataTransfer.getData('Text'))) {
            document.getElementById(event.dataTransfer.getData('Text')).classList.remove('opacity05');
        }
    }, false);
    tableBody.addEventListener('drop', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (dragEl.nodeName !== 'TR') {
            return;
        }
        let target = event.target;
        if (event.target.nodeName === 'TD') {
            target = event.target.parentNode;
        }
        let oldSongpos = document.getElementById(event.dataTransfer.getData('Text')).getAttribute('data-songpos');
        let newSongpos = target.getAttribute('data-songpos');
        document.getElementById(event.dataTransfer.getData('Text')).remove();
        dragEl.classList.remove('opacity05');
        tableBody.insertBefore(dragEl, target);
        let tr = tableBody.getElementsByClassName('dragover');
        let trLen = tr.length;
        for (let i = 0; i < trLen; i++) {
            tr[i].classList.remove('dragover');
        }
        document.getElementById(table).classList.add('opacity05');
        if (app.current.app === 'Queue' && app.current.tab === 'Current') {
            sendAPI("MPD_API_QUEUE_MOVE_TRACK", {"from": oldSongpos, "to": newSongpos});
        }
        else if (app.current.app === 'Browse' && app.current.tab === 'Playlists' && app.current.view === 'Detail') {
            playlistMoveTrack(oldSongpos, newSongpos);
        }
    }, false);
}

function dragAndDropTableHeader(table) {
    let tableHeader;
    if (document.getElementById(table + 'List')) {
        tableHeader = document.getElementById(table + 'List').getElementsByTagName('tr')[0];
    }
    else {
        tableHeader = table.getElementsByTagName('tr')[0];
        table = 'BrowseDatabase';
    }

    tableHeader.addEventListener('dragstart', function(event) {
        if (event.target.nodeName === 'TH') {
            event.target.classList.add('opacity05');
            event.dataTransfer.setDragImage(event.target, 0, 0);
            event.dataTransfer.effectAllowed = 'move';
            event.dataTransfer.setData('Text', event.target.getAttribute('data-col'));
            dragEl = event.target.cloneNode(true);
        }
    }, false);
    tableHeader.addEventListener('dragleave', function(event) {
        event.preventDefault();
        if (dragEl.nodeName !== 'TH') {
            return;
        }
        if (event.target.nodeName === 'TH') {
            event.target.classList.remove('dragover-th');
        }
    }, false);
    tableHeader.addEventListener('dragover', function(event) {
        event.preventDefault();
        if (dragEl.nodeName !== 'TH') {
            return;
        }
        let th = tableHeader.getElementsByClassName('dragover-th');
        let thLen = th.length;
        for (let i = 0; i < thLen; i++) {
            th[i].classList.remove('dragover-th');
        }
        if (event.target.nodeName === 'TH') {
            event.target.classList.add('dragover-th');
        }
        event.dataTransfer.dropEffect = 'move';
    }, false);
    tableHeader.addEventListener('dragend', function(event) {
        event.preventDefault();
        if (dragEl.nodeName !== 'TH') {
            return;
        }
        let th = tableHeader.getElementsByClassName('dragover-th');
        let thLen = th.length;
        for (let i = 0; i < thLen; i++) {
            th[i].classList.remove('dragover-th');
        }
        if (this.querySelector('[data-col=' + event.dataTransfer.getData('Text') + ']')) {
            this.querySelector('[data-col=' + event.dataTransfer.getData('Text') + ']').classList.remove('opacity05');
        }
    }, false);
    tableHeader.addEventListener('drop', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (dragEl.nodeName !== 'TH') {
            return;
        }
        this.querySelector('[data-col=' + event.dataTransfer.getData('Text') + ']').remove();
        dragEl.classList.remove('opacity05');
        tableHeader.insertBefore(dragEl, event.target);
        let th = tableHeader.getElementsByClassName('dragover-th');
        let thLen = th.length;
        for (let i = 0; i < thLen; i++) {
            th[i].classList.remove('dragover-th');
        }
        if (document.getElementById(table + 'List')) {
            document.getElementById(table + 'List').classList.add('opacity05');
            saveCols(table);
        }
        else {
            saveCols(table, this.parentNode.parentNode);
        }
    }, false);
}

function setColTags(table) {
    let tags = settings.tags.slice();
    if (settings.featTags === false) {
        tags.push('Title');
    }
    tags.push('Duration');
    if (table === 'QueueCurrent' || table === 'BrowsePlaylistsDetail' || table === 'QueueLastPlayed' || table === 'QueueJukebox') {
        tags.push('Pos');
    }
    if (table === 'BrowseFilesystem') {
        tags.push('Type');
    }
    if (table === 'QueueLastPlayed') {
        tags.push('LastPlayed');
    }
    if (table === 'Playback') {
        tags.push('Filetype');
        tags.push('Fileformat');
        tags.push('LastModified');
        if (settings.featLyrics === true) {
            tags.push('Lyrics');
        }
    }
    
    tags.sort();
    return tags;
}

function setColsChecklist(table) {
    let tagChks = '';
    let tags = setColTags(table);
    for (let i = 0; i < tags.length; i++) {
        if (table === 'Playback' && tags[i] === 'Title') {
            continue;
        }
        tagChks += '<div>' +
            '<button class="btn btn-secondary btn-xs clickable material-icons material-icons-small' +
            (settings['cols' + table].includes(tags[i]) ? ' active' : '') + '" name="' + tags[i] + '">' +
            (settings['cols' + table].includes(tags[i]) ? 'check' : 'radio_button_unchecked') + '</button>' +
            '<label class="form-check-label" for="' + tags[i] + '">&nbsp;&nbsp;' + t(tags[i]) + '</label>' +
            '</div>';
    }
    return tagChks;
}

function setCols(table, className) {
    let colsChkList = document.getElementById(table + 'ColsDropdown');
    if (colsChkList) {
        colsChkList.firstChild.innerHTML = setColsChecklist(table);
    }
    let sort = app.current.sort;
    
    if (table === 'Search' && app.apps.Search.state === '0/any/Title/-/') {
        if (settings.tags.includes('Title')) {
            sort = 'Title';
        }
        else if (settings.featTags === false) {
            sort = 'Filename';
        }
        else {
            sort = '-';
        }
    }
    
    if (table !== 'Playback') {
        let heading = '';
        for (let i = 0; i < settings['cols' + table].length; i++) {
            let h = settings['cols' + table][i];
            heading += '<th draggable="true" data-col="' + h  + '">';
            if (h === 'Track' || h === 'Pos') {
                h = '#';
            }
            heading += t(h);

            if (table === 'Search' && (h === sort || '-' + h === sort) ) {
                let sortdesc = false;
                if (app.current.sort.indexOf('-') === 0) {
                    sortdesc = true;
                }
                heading += '<span class="sort-dir material-icons pull-right">' + (sortdesc === true ? 'arrow_drop_up' : 'arrow_drop_down') + '</span>';
            }
            heading += '</th>';
        }
        if (settings.featTags === true) {
            heading += '<th data-col="Action"><a href="#" class="text-secondary align-middle material-icons material-icons-small">settings</a></th>';
        }
        else {
            heading += '<th></th>';
        }

        if (className === undefined) {
            document.getElementById(table + 'List').getElementsByTagName('tr')[0].innerHTML = heading;
        }
        else {
            let tbls = document.querySelectorAll(className);
            for (let i = 0; i < tbls.length; i++) {
                tbls[i].getElementsByTagName('tr')[0].innerHTML = heading;
            }
        }
    }
}

function saveCols(table, tableEl) {
    let colsDropdown = document.getElementById(table + 'ColsDropdown');
    let header;
    if (tableEl === undefined) {
        header = document.getElementById(table + 'List').getElementsByTagName('tr')[0];
    }
    else if (typeof(tableEl) === 'string') {
        header = document.querySelector(tableEl).getElementsByTagName('tr')[0];
    }
    else {
        header = tableEl.getElementsByTagName('tr')[0];
    }
    if (colsDropdown) {
        let colInputs = colsDropdown.firstChild.getElementsByTagName('button');
        for (let i = 0; i < colInputs.length; i++) {
            if (colInputs[i].getAttribute('name') === null) {
                continue;
            }
            let th = header.querySelector('[data-col=' + colInputs[i].name + ']');
            if (colInputs[i].classList.contains('active') === false) {
                if (th) {
                    th.remove();
                }
            } 
            else if (!th) {
                th = document.createElement('th');
                th.innerText = colInputs[i].name;
                th.setAttribute('data-col', colInputs[i].name);
                header.insertBefore(th, header.lastChild);
            }
        }
    }
    
    let params = {"table": "cols" + table, "cols": []};
    let ths = header.getElementsByTagName('th');
    for (let i = 0; i < ths.length; i++) {
        let name = ths[i].getAttribute('data-col');
        if (name !== 'Action' && name !== null) {
            params.cols.push(name);
        }
    }
    sendAPI("MYMPD_API_COLS_SAVE", params, getSettings);
}

//eslint-disable-next-line no-unused-vars
function saveColsPlayback(table) {
    let colInputs = document.getElementById(table + 'ColsDropdown').firstChild.getElementsByTagName('button');
    let header = document.getElementById('cardPlaybackTags');

    for (let i = 0; i < colInputs.length -1; i++) {
        let th = document.getElementById('current' + colInputs[i].name);
        if (colInputs[i].classList.contains('active') === false) {
            if (th) {
                th.remove();
            }
        } 
        else if (!th) {
            th = document.createElement('div');
            th.innerHTML = '<small>' + t(colInputs[i].name) + '</small><p></p>';
            th.setAttribute('id', 'current' + colInputs[i].name);
            th.setAttribute('data-tag', colInputs[i].name);
            header.appendChild(th);
        }
    }
    
    let params = {"table": "cols" + table, "cols": []};
    let ths = header.getElementsByTagName('div');
    for (let i = 0; i < ths.length; i++) {
        let name = ths[i].getAttribute('data-tag');
        if (name) {
            params.cols.push(name);
        }
    }
    sendAPI("MYMPD_API_COLS_SAVE", params, getSettings);
}

function replaceTblRow(row, el) {
    let menuEl = row.querySelector('[data-popover]');
    let result = false;
    if (menuEl) {
        hideMenu();
    }
    if (row.classList.contains('selected')) {
        el.classList.add('selected');
        el.focus();
        result = true;
    }
    row.replaceWith(el);
    return result;
}
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

//eslint-disable-next-line no-unused-vars
var themes = {
    "theme-autodetect": "Autodetect",
    "theme-default": "Default",
    "theme-dark": "Dark",
    "theme-light": "Light"
};
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

//eslint-disable-next-line no-unused-vars
function deleteTimer(timerid) {
    sendAPI("MYMPD_API_TIMER_RM", {"timerid": timerid}, showListTimer);
}

//eslint-disable-next-line no-unused-vars
function toggleTimer(target, timerid) {
    if (target.classList.contains('active')) {
        target.classList.remove('active');
        sendAPI("MYMPD_API_TIMER_TOGGLE", {"timerid": timerid, "enabled": false}, showListTimer);
    }
    else {
        target.classList.add('active');
        sendAPI("MYMPD_API_TIMER_TOGGLE", {"timerid": timerid, "enabled": true}, showListTimer);
    }
}

//eslint-disable-next-line no-unused-vars
function saveTimer() {
    let formOK = true;
    let nameEl = document.getElementById('inputTimerName');
    if (!validateNotBlank(nameEl)) {
        formOK = false;
    }
    let minOneDay = false;
    let weekdayBtns = ['btnTimerMon', 'btnTimerTue', 'btnTimerWed', 'btnTimerThu', 'btnTimerFri', 'btnTimerSat', 'btnTimerSun'];
    let weekdays = [];
    for (let i = 0; i < weekdayBtns.length; i++) {
        let checked = document.getElementById(weekdayBtns[i]).classList.contains('active') ? true : false;
        weekdays.push(checked);
        if (checked === true) {
            minOneDay = true;
        }
    }
    if (minOneDay === false) {
        formOK = false;
        document.getElementById('invalidTimerWeekdays').style.display = 'block';
    }
    else {
        document.getElementById('invalidTimerWeekdays').style.display = 'none';
    }
    let selectTimerAction = document.getElementById('selectTimerAction');
    let selectTimerPlaylist = document.getElementById('selectTimerPlaylist');
    let selectTimerHour = document.getElementById('selectTimerHour');
    let selectTimerMinute = document.getElementById('selectTimerMinute');
    let jukeboxMode = document.getElementById('btnTimerJukeboxModeGroup').getElementsByClassName('active')[0].getAttribute('data-value');

    if (selectTimerAction.selectedIndex === -1) {
        formOK = false;
        selectTimerAction.classList.add('is-invalid');
    }

    if (jukeboxMode === '0' &&
        selectTimerPlaylist.options[selectTimerPlaylist.selectedIndex].value === 'Database'&&
        selectTimerAction.options[selectTimerAction.selectedIndex].value === 'startplay')
    {
        formOK = false;
        document.getElementById('btnTimerJukeboxModeGroup').classList.add('is-invalid');
    }
    
    if (formOK === true) {
        let args = {};
        let argEls = document.getElementById('timerActionScriptArguments').getElementsByTagName('input');
        for (let i = 0; i < argEls.length; i ++) {
            args[argEls[i].getAttribute('data-name')] = argEls[i].value;
        }
        sendAPI("MYMPD_API_TIMER_SAVE", {
            "timerid": parseInt(document.getElementById('inputTimerId').value),
            "name": nameEl.value,
            "enabled": (document.getElementById('btnTimerEnabled').classList.contains('active') ? true : false),
            "startHour": parseInt(selectTimerHour.options[selectTimerHour.selectedIndex].value),
            "startMinute": parseInt(selectTimerMinute.options[selectTimerMinute.selectedIndex].value),
            "weekdays": weekdays,
            "action": selectTimerAction.options[selectTimerAction.selectedIndex].parentNode.getAttribute('data-value'),
            "subaction": selectTimerAction.options[selectTimerAction.selectedIndex].value,
            "volume": parseInt(document.getElementById('inputTimerVolume').value), 
            "playlist": selectTimerPlaylist.options[selectTimerPlaylist.selectedIndex].value,
            "jukeboxMode": parseInt(jukeboxMode),
            "arguments": args
            }, showListTimer);
    }
}

//eslint-disable-next-line no-unused-vars
function showEditTimer(timerid) {
    document.getElementById('timerActionPlay').classList.add('hide');
    document.getElementById('timerActionScript').classList.add('hide');
    document.getElementById('listTimer').classList.remove('active');
    document.getElementById('editTimer').classList.add('active');
    document.getElementById('listTimerFooter').classList.add('hide');
    document.getElementById('editTimerFooter').classList.remove('hide');
        
    if (timerid !== 0) {
        sendAPI("MYMPD_API_TIMER_GET", {"timerid": timerid}, parseEditTimer);
    }
    else {
        sendAPI("MPD_API_PLAYLIST_LIST_ALL", {"searchstr":""}, function(obj2) { 
            getAllPlaylists(obj2, 'selectTimerPlaylist', 'Database');
        });
        document.getElementById('inputTimerId').value = '0';
        document.getElementById('inputTimerName').value = '';
        toggleBtnChk('btnTimerEnabled', true);
        document.getElementById('selectTimerHour').value = '12';
        document.getElementById('selectTimerMinute').value = '0';
        document.getElementById('selectTimerAction').value = 'startplay';
        document.getElementById('inputTimerVolume').value = '50';
        document.getElementById('selectTimerPlaylist').value = 'Database';
        toggleBtnGroupValue(document.getElementById('btnTimerJukeboxModeGroup'), 1);
        let weekdayBtns = ['btnTimerMon', 'btnTimerTue', 'btnTimerWed', 'btnTimerThu', 'btnTimerFri', 'btnTimerSat', 'btnTimerSun'];
        for (let i = 0; i < weekdayBtns.length; i++) {
            toggleBtnChk(weekdayBtns[i], false);
        }
        document.getElementById('timerActionPlay').classList.remove('hide');
    }
    document.getElementById('inputTimerName').focus();
    removeIsInvalid(document.getElementById('editTimerForm'));    
    document.getElementById('invalidTimerWeekdays').style.display = 'none';
}

function parseEditTimer(obj) {
    let playlistValue = obj.result.playlist;
    sendAPI("MPD_API_PLAYLIST_LIST_ALL", {"searchstr":""}, function(obj2) { 
        getAllPlaylists(obj2, 'selectTimerPlaylist', playlistValue);
    });
    document.getElementById('inputTimerId').value = obj.result.timerid;
    document.getElementById('inputTimerName').value = obj.result.name;
    toggleBtnChk('btnTimerEnabled', obj.result.enabled);
    document.getElementById('selectTimerHour').value = obj.result.startHour;
    document.getElementById('selectTimerMinute').value = obj.result.startMinute;
    document.getElementById('selectTimerAction').value = obj.result.subaction;
    selectTimerActionChange(obj.result.arguments);
    document.getElementById('inputTimerVolume').value = obj.result.volume;
    toggleBtnGroupValue(document.getElementById('btnTimerJukeboxModeGroup'), obj.result.jukeboxMode);
    let weekdayBtns = ['btnTimerMon', 'btnTimerTue', 'btnTimerWed', 'btnTimerThu', 'btnTimerFri', 'btnTimerSat', 'btnTimerSun'];
    for (let i = 0; i < weekdayBtns.length; i++) {
        toggleBtnChk(weekdayBtns[i], obj.result.weekdays[i]);
    }
}

function selectTimerActionChange(values) {
    let el = document.getElementById('selectTimerAction');
    
    if (el.options[el.selectedIndex].value === 'startplay') {
        document.getElementById('timerActionPlay').classList.remove('hide');
        document.getElementById('timerActionScript').classList.add('hide');
    }
    else if (el.options[el.selectedIndex].parentNode.getAttribute('data-value') === 'script') {
        document.getElementById('timerActionScript').classList.remove('hide');
        document.getElementById('timerActionPlay').classList.add('hide');
        showTimerScriptArgs(el.options[el.selectedIndex], values);
    }
    else {
        document.getElementById('timerActionPlay').classList.add('hide');
        document.getElementById('timerActionScript').classList.add('hide');
    }
}

function showTimerScriptArgs(option, values) {
    if (values === undefined) {
        values = {};
    }
    let args = JSON.parse(option.getAttribute('data-arguments'));
    let list = '';
    for (let i = 0; i < args.arguments.length; i++) {
        list += '<div class="form-group row">' +
                  '<label class="col-sm-4 col-form-label" for="timerActionScriptArguments' + i + '">' + e(args.arguments[i]) + '</label>' +
                  '<div class="col-sm-8">' +
                    '<input name="timerActionScriptArguments' + i + '" class="form-control border-secondary" type="text" value="' +
                    (values[args.arguments[i]] ? e(values[args.arguments[i]]) : '') + '"' +
                    'data-name="' + args.arguments[i] + '">' +
                  '</div>' +
                '</div>';
    }
    if (args.arguments.length === 0) {
        list = 'No arguments';
    }
    document.getElementById('timerActionScriptArguments').innerHTML = list;
}

function showListTimer() {
    document.getElementById('listTimer').classList.add('active');
    document.getElementById('editTimer').classList.remove('active');
    document.getElementById('listTimerFooter').classList.remove('hide');
    document.getElementById('editTimerFooter').classList.add('hide');
    sendAPI("MYMPD_API_TIMER_LIST", {}, parseListTimer);
}

function parseListTimer(obj) {
    let tbody = document.getElementById('listTimer').getElementsByTagName('tbody')[0];
    let tr = tbody.getElementsByTagName('tr');
    
    let activeRow = 0;
    let weekdays = ['Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat', 'Sun'];
    for (let i = 0; i < obj.result.returnedEntities; i++) {
        let row = document.createElement('tr');
        row.setAttribute('data-id', obj.result.data[i].timerid);
        let tds = '<td>' + e(obj.result.data[i].name) + '</td>' +
                  '<td><button name="enabled" class="btn btn-secondary btn-xs clickable material-icons material-icons-small' +
                  (obj.result.data[i].enabled === true ? ' active' : '') + '">' +
                  (obj.result.data[i].enabled === true ? 'check' : 'radio_button_unchecked') + '</button></td>' +
                  '<td>' + zeroPad(obj.result.data[i].startHour, 2) + ':' + zeroPad(obj.result.data[i].startMinute,2) + ' ' + t('on') + ' ';
        let days = [];
        for (let j = 0; j < 7; j++) {
            if (obj.result.data[i].weekdays[j] === true) {
                days.push(t(weekdays[j]))
            }
        }
        tds += days.join(', ')  + '</td><td>' + prettyTimerAction(obj.result.data[i].action, obj.result.data[i].subaction) + '</td>' +
               '<td data-col="Action"><a href="#" class="material-icons color-darkgrey">delete</a></td>';
        row.innerHTML = tds;
        if (i < tr.length) {
            activeRow = replaceTblRow(tr[i], row) === true ? i : activeRow;
        }
        else {
            tbody.append(row);
        }
    }
    let trLen = tr.length - 1;
    for (let i = trLen; i >= obj.result.returnedEntities; i --) {
        tr[i].remove();
    }

    if (obj.result.returnedEntities === 0) {
        tbody.innerHTML = '<tr class="not-clickable"><td><span class="material-icons">error_outline</span></td>' +
                          '<td colspan="4">' + t('Empty list') + '</td></tr>';
    }     
}

function prettyTimerAction(action, subaction) {
    if (action === 'player' && subaction === 'startplay') {
        return t('Start playback');
    }
    if (action === 'player' && subaction === 'stopplay') {
        return t('Stop playback');
    }
    if (action === 'syscmd') {
        return t('System command') + ': ' + e(subaction);
    }
    if (action === 'script') {
        return t('Script') + ': ' + e(subaction);
    }
    return e(action) + ': ' + e(subaction);
}
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

//eslint-disable-next-line no-unused-vars
function saveTrigger() {
    let formOK = true;
    
    let nameEl = document.getElementById('inputTriggerName');
    if (!validatePlnameEl(nameEl)) {
        formOK = false;
    }
    
    if (formOK === true) {
        let args = {};
        let argEls = document.getElementById('triggerActionScriptArguments').getElementsByTagName('input');
        for (let i = 0; i < argEls.length; i ++) {
            args[argEls[i].getAttribute('data-name')] = argEls[i].value;
        }

        sendAPI("MPD_API_TRIGGER_SAVE", {
            "id": parseInt(document.getElementById('inputTriggerId').value),
            "name": nameEl.value,
            "event": getSelectValue('selectTriggerEvent'),
            "script": getSelectValue('selectTriggerScript'),
            "arguments": args
            }, showListTrigger, false);
    }
}

//eslint-disable-next-line no-unused-vars
function showEditTrigger(id) {
    document.getElementById('listTrigger').classList.remove('active');
    document.getElementById('newTrigger').classList.add('active');
    document.getElementById('listTriggerFooter').classList.add('hide');
    document.getElementById('newTriggerFooter').classList.remove('hide');
    
    const nameEl = document.getElementById('inputTriggerName');
    nameEl.classList.remove('is-invalid');
    nameEl.value = '';
    nameEl.focus();
    document.getElementById('inputTriggerId').value = '-1';
    document.getElementById('selectTriggerEvent').selectedIndex = 0;
    document.getElementById('selectTriggerScript').selectedIndex = 0;
    if (id > -1) {
        sendAPI("MPD_API_TRIGGER_GET", {"id": id}, parseTriggerEdit, false);
    }
    else {
        selectTriggerActionChange();
    }
}

function parseTriggerEdit(obj) {
    document.getElementById('inputTriggerId').value = obj.result.id;
    document.getElementById('inputTriggerName').value = obj.result.name;
    document.getElementById('selectTriggerEvent').value = obj.result.event;
    document.getElementById('selectTriggerScript').value = obj.result.script;
    selectTriggerActionChange(obj.result.arguments);
}

function selectTriggerActionChange(values) {
    let el = document.getElementById('selectTriggerScript');
    showTriggerScriptArgs(el.options[el.selectedIndex], values);
}

function showTriggerScriptArgs(option, values) {
    if (values === undefined) {
        values = {};
    }
    let args = JSON.parse(option.getAttribute('data-arguments'));
    let list = '';
    for (let i = 0; i < args.arguments.length; i++) {
        list += '<div class="form-group row">' +
                  '<label class="col-sm-4 col-form-label" for="triggerActionScriptArguments' + i + '">' + e(args.arguments[i]) + '</label>' +
                  '<div class="col-sm-8">' +
                    '<input name="triggerActionScriptArguments' + i + '" class="form-control border-secondary" type="text" value="' +
                    (values[args.arguments[i]] ? e(values[args.arguments[i]]) : '') + '"' +
                    'data-name="' + args.arguments[i] + '">' +
                  '</div>' +
                '</div>';
    }
    if (args.arguments.length === 0) {
        list = 'No arguments';
    }
    document.getElementById('triggerActionScriptArguments').innerHTML = list;
}

function showListTrigger() {
    document.getElementById('listTrigger').classList.add('active');
    document.getElementById('newTrigger').classList.remove('active');
    document.getElementById('listTriggerFooter').classList.remove('hide');
    document.getElementById('newTriggerFooter').classList.add('hide');
    sendAPI("MPD_API_TRIGGER_LIST", {}, parseTriggerList, false);
}

function deleteTrigger(id) {
    sendAPI("MPD_API_TRIGGER_DELETE", {"id": id}, function() {
        sendAPI("MPD_API_TRIGGER_LIST", {}, parseTriggerList, false);
    }, true);
}

function parseTriggerList(obj) {
    if (obj.result.data.length > 0) {
        let triggerList = '';
        for (let i = 0; i < obj.result.data.length; i++) {
            triggerList += '<tr data-trigger-id="' + encodeURI(obj.result.data[i].id) + '"><td class="' +
                (obj.result.data[i].name === settings.trigger ? 'font-weight-bold' : '') +
                '">' + e(obj.result.data[i].name) + 
                '</td>' +
                '<td>' + t(obj.result.data[i].eventName) + '</td>' +
                '<td>' + e(obj.result.data[i].script) + '</td>' +
                '<td data-col="Action">' +
                (obj.result.data[i].name === 'default' || obj.result.data[i].name === settings.trigger  ? '' : 
                    '<a href="#" title="' + t('Delete') + '" data-action="delete" class="material-icons color-darkgrey">delete</a>') +
                '</td></tr>';
        }
        document.getElementById('listTriggerList').innerHTML = triggerList;
    }
    else {
        document.getElementById('listTriggerList').innerHTML = '<tr class="not-clickable"><td><span class="material-icons">error_outline</span></td>' +
            '<td colspan="2">' + t('Empty list') + '</td></tr>';
    }
}
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

function removeIsInvalid(el) {
    let els = el.querySelectorAll('.is-invalid');
    for (let i = 0; i < els.length; i++) {
        els[i].classList.remove('is-invalid');
    }
}

function getSelectValue(el) {
    if (typeof el === 'string')	{
        el = document.getElementById(el);
    }
    if (el && el.selectedIndex >= 0) {
        return el.options[el.selectedIndex].value;
    }
    return undefined;
}

function getSelectedOptionAttribute(selectId, attribute) {
    let el = document.getElementById(selectId);
    if (el && el.selectedIndex >= 0) {
        return el.options[el.selectedIndex].getAttribute(attribute);
    }
    return undefined;
}

function alignDropdown(el) {
    const x = getXpos(el.children[0]);
    
    if (x < domCache.body.offsetWidth * 0.66) {
        if (el.id === 'navState') {
            el.classList.remove('dropdown');
            el.classList.add('dropright');
        }
        else {
            el.getElementsByClassName('dropdown-menu')[0].classList.remove('dropdown-menu-right');
        }
    }
    else {
        el.getElementsByClassName('dropdown-menu')[0].classList.add('dropdown-menu-right');
        el.classList.add('dropdown');
        el.classList.remove('dropright');
    }
}

function getXpos(el) {
    var xPos = 0;
    while (el) {
        xPos += (el.offsetLeft - el.scrollLeft + el.clientLeft);
        el = el.offsetParent;
    }
    return xPos;
}

function zeroPad(num, places) {
  var zero = places - num.toString().length + 1;
  return Array(+(zero > 0 && zero)).join("0") + num;
}

function dirname(uri) {
    return uri.replace(/\/[^/]*$/, '');
}

function basename(uri, removeQuery) {
    if (removeQuery === true) {
        return uri.split('/').reverse()[0].split(/[?#]/)[0];
    }
    else {
        return uri.split('/').reverse()[0];
    }
}

function filetype(uri) {
    if (uri === undefined) {
        return '';
    }
    let ext = uri.split('.').pop().toUpperCase();
    switch (ext) {
        case 'MP3':  return ext + ' - MPEG-1 Audio Layer III';
        case 'FLAC': return ext + ' - Free Lossless Audio Codec';
        case 'OGG':  return ext + ' - Ogg Vorbis';
        case 'OPUS': return ext + ' - Opus Audio';
        case 'WAV':  return ext + ' - WAVE Audio File';
        case 'WV':   return ext + ' - WavPack';
        case 'AAC':  return ext + ' - Advancded Audio Coding';
        case 'MPC':  return ext + ' - Musepack';
        case 'MP4':  return ext + ' - MPEG-4';
        case 'APE':  return ext + ' - Monkey Audio ';
        case 'WMA':  return ext + ' - Windows Media Audio';
        default:     return ext;
    }
}

function fileformat(audioformat) {
    return audioformat.bits + t('bits') + ' - ' + audioformat.sampleRate / 1000 + t('kHz');
}

function scrollToPosY(pos) {
    document.body.scrollTop = pos; // For Safari
    document.documentElement.scrollTop = pos; // For Chrome, Firefox, IE and Opera
}

function selectTag(btnsEl, desc, setTo) {
    let btns = document.getElementById(btnsEl);
    let aBtn = btns.querySelector('.active')
    if (aBtn) {
        aBtn.classList.remove('active');
    }
    aBtn = btns.querySelector('[data-tag=' + setTo + ']');
    if (aBtn) {
        aBtn.classList.add('active');
        if (desc !== undefined) {
            document.getElementById(desc).innerText = aBtn.innerText;
            document.getElementById(desc).setAttribute('data-phrase', aBtn.innerText);
        }
    }
}

function addTagList(el, list) {
    let tagList = '';
    if (list === 'searchtags') {
        if (settings.featTags === true) {
            tagList += '<button type="button" class="btn btn-secondary btn-sm btn-block" data-tag="any">' + t('Any Tag') + '</button>';
        }
        tagList += '<button type="button" class="btn btn-secondary btn-sm btn-block" data-tag="filename">' + t('Filename') + '</button>';
    }
    for (let i = 0; i < settings[list].length; i++) {
        tagList += '<button type="button" class="btn btn-secondary btn-sm btn-block" data-tag="' + settings[list][i] + '">' + t(settings[list][i]) + '</button>';
    }
    if (el === 'BrowseNavFilesystemDropdown' || el === 'BrowseNavPlaylistsDropdown') {
        tagList = '<button type="button" class="btn btn-secondary btn-sm btn-block" data-tag="Database">Database</button>';
    }
    if (el === 'BrowseDatabaseByTagDropdown' || el === 'BrowseNavFilesystemDropdown' || el === 'BrowseNavPlaylistsDropdown') {
        if (el === 'BrowseDatabaseByTagDropdown') {
            tagList += '<div class="dropdown-divider"></div>';
        }
        tagList += '<button type="button" class="btn btn-secondary btn-sm btn-block' + (el === 'BrowseNavPlaylistsDropdown' ? ' active' : '') + '" data-tag="Playlists">' + t('Playlists') + '</button>' +
            '<button type="button" class="btn btn-secondary btn-sm btn-block' + (el === 'BrowseNavFilesystemDropdown' ? ' active' : '') + '" data-tag="Filesystem">' + t('Filesystem') + '</button>'
    }
    else if (el === 'databaseSortTagsList') {
        if (settings.tags.includes('Date')) {
            tagList += '<button type="button" class="btn btn-secondary btn-sm btn-block" data-tag="Date">' + t('Date') + '</button>';
        }
        tagList += '<button type="button" class="btn btn-secondary btn-sm btn-block" data-tag="Last-Modified">' + t('Last modified') + '</button>';
    }
    document.getElementById(el).innerHTML = tagList;
}

function addTagListSelect(el, list) {
    let tagList = '';
    if (el === 'saveSmartPlaylistSort' || el === 'selectSmartplsSort') {
        tagList += '<option value="">' + t('Disabled') + '</option>';
        tagList += '<option value="shuffle">' + t('Shuffle') + '</option>';
        tagList += '<optgroup label="' + t('Sort by tag') + '">';
        tagList += '<option value="filename">' + t('Filename') + '</option>';
    }
    else if (el === 'selectJukeboxUniqueTag' && settings.browsetags.includes('Title') === false) {
        //Title tag should be always in the list
        tagList = '<option value="Title">' + t('Song') + '</option>';
    }
    for (let i = 0; i < settings[list].length; i++) {
        tagList += '<option value="' + settings[list][i] + '">' + t(settings[list][i]) + '</option>';
    }
    if (el === 'saveSmartPlaylistSort' || el === 'selectSmartplsSort') {
        tagList += '</optgroup>';
    }
    document.getElementById(el).innerHTML = tagList;
}

//eslint-disable-next-line no-unused-vars
function openModal(modal) {
    window[modal].show();
}

//eslint-disable-next-line no-unused-vars
function openDropdown(dropdown) {
    window[dropdown].toggle();
}

//eslint-disable-next-line no-unused-vars
function focusSearch() {
    if (app.current.app === 'Queue') {
        document.getElementById('searchqueuestr').focus();
    }
    else if (app.current.app === 'Search') {
        domCache.searchstr.focus();
    }
    else {
        appGoto('Search');
    }
}

function btnWaiting(btn, waiting) {
    if (waiting === true) {
        let spinner = document.createElement('span');
        spinner.classList.add('spinner-border', 'spinner-border-sm', 'mr-2');
        btn.insertBefore(spinner, btn.firstChild);
        btn.setAttribute('disabled', 'disabled');
    }
    else {
        btn.removeAttribute('disabled');
        if (btn.firstChild.nodeName === 'SPAN') {
            btn.firstChild.remove();
        }
    }
}

function toggleBtnGroupValue(btngrp, value) {
    let btns = btngrp.getElementsByTagName('button');
    let b = btns[0];
    let valuestr = value;
    if (isNaN(value) === false) {
        valuestr = value.toString();
    }
    for (let i = 0; i < btns.length; i++) {
        if (btns[i].getAttribute('data-value') === valuestr) {
            btns[i].classList.add('active');
            b = btns[i];
        }
        else {
            btns[i].classList.remove('active');
        }
    }
    return b;
}

function toggleBtnGroupValueCollapse(btngrp, collapse, value) {
    let activeBtn = toggleBtnGroupValue(btngrp, value);
    if (activeBtn.getAttribute('data-collapse') === 'show') {
        document.getElementById(collapse).classList.add('show');
    }
    else {
        document.getElementById(collapse).classList.remove('show');
    }
}

function toggleBtnGroup(btn) {
    let b = btn;
    if (typeof btn === 'string') {
        b = document.getElementById(btn);
    }
    let btns = b.parentNode.getElementsByTagName('button');
    for (let i = 0; i < btns.length; i++) {
        if (btns[i] === b) {
            btns[i].classList.add('active');
        }
        else {
            btns[i].classList.remove('active');
        }
    }
    return b;
}

function getBtnGroupValue(btnGroup) {
    let activeBtn = document.getElementById(btnGroup).getElementsByClassName('active');
    if (activeBtn.length === 0) {
        activeBtn = document.getElementById(btnGroup).getElementsByTagName('button');    
    }
    return activeBtn[0].getAttribute('data-value');
}

//eslint-disable-next-line no-unused-vars
function toggleBtnGroupCollapse(btn, collapse) {
    let activeBtn = toggleBtnGroup(btn);
    if (activeBtn.getAttribute('data-collapse') === 'show') {
        if (document.getElementById(collapse).classList.contains('show') === false) {
            window[collapse].show();
        }
    }
    else {
        window[collapse].hide();
    }
}

function toggleBtn(btn, state) {
    let b = btn;
    if (typeof btn === 'string') {
        b = document.getElementById(btn);
    }
    if (!b) {
        return;
    }
    if (state === undefined) {
        //toggle state
        state = b.classList.contains('active') ? false : true;
    }

    if (state === true || state === 1) {
        b.classList.add('active');
    }
    else {
        b.classList.remove('active');
    }
}

function toggleBtnChk(btn, state) {
    let b = btn;
    if (typeof btn === 'string') {
        b = document.getElementById(btn);
    }
    if (!b) {
        return;
    }
    if (state === undefined) {
        //toggle state
        state = b.classList.contains('active') ? false : true;
    }

    if (state === true || state === 1) {
        b.classList.add('active');
        b.innerText = 'check';
        return true;
    }
    else {
        b.classList.remove('active');
        b.innerText = 'radio_button_unchecked';
        return false;
    }
}

function toggleBtnChkCollapse(btn, collapse, state) {
    let checked = toggleBtnChk(btn, state);
    if (checked === true) {
        document.getElementById(collapse).classList.add('show');
    }
    else{
        document.getElementById(collapse).classList.remove('show');
    }
}

function setPagination(total, returned) {
    let cat = app.current.app + (app.current.tab === undefined ? '' : app.current.tab);
    let totalPages = Math.ceil(total / settings.maxElementsPerPage);
    if (totalPages === 0) {
        totalPages = 1;
    }
    let p = [ document.getElementById(cat + 'PaginationTop'), document.getElementById(cat + 'PaginationBottom') ];
    
    for (let i = 0; i < p.length; i++) {
        let prev = p[i].children[0];
        let page = p[i].children[1].children[0];
        let pages = p[i].children[1].children[1];
        let next = p[i].children[2];
    
        page.innerText = (app.current.page / settings.maxElementsPerPage + 1) + ' / ' + totalPages;
        if (totalPages > 1) {
            page.removeAttribute('disabled');
            let pl = '';
            for (let j = 0; j < totalPages; j++) {
                pl += '<button data-page="' + (j * settings.maxElementsPerPage) + '" type="button" class="mr-1 mb-1 btn-sm btn btn-secondary">' +
                      ( j + 1) + '</button>';
            }
            pages.innerHTML = pl;
            page.classList.remove('nodropdown');
        }
        else if (total === -1) {
            page.setAttribute('disabled', 'disabled');
            page.innerText = (app.current.page / settings.maxElementsPerPage + 1);
            page.classList.add('nodropdown');
        }
        else {
            page.setAttribute('disabled', 'disabled');
            page.classList.add('nodropdown');
        }
        
        if (total > app.current.page + settings.maxElementsPerPage || total === -1 && returned >= settings.maxElementsPerPage) {
            next.removeAttribute('disabled');
            p[i].classList.remove('hide');
            document.getElementById(cat + 'ButtonsBottom').classList.remove('hide');
        }
        else {
            next.setAttribute('disabled', 'disabled');
            p[i].classList.add('hide');
            document.getElementById(cat + 'ButtonsBottom').classList.add('hide');
        }
    
        if (app.current.page > 0) {
            prev.removeAttribute('disabled');
            p[i].classList.remove('hide');
            document.getElementById(cat + 'ButtonsBottom').classList.remove('hide');
        }
        else {
            prev.setAttribute('disabled', 'disabled');
        }
    }
}

function genId(x) {
    return 'id' + x.replace(/[^\w-]/g, '');
}

function parseCmd(event, href) {
    if (event !== null) {
        event.preventDefault();
    }
    let cmd = href;
    if (typeof(href) === 'string') {
        cmd = JSON.parse(href);
    }

    if (typeof window[cmd.cmd] === 'function') {
        switch(cmd.cmd) {
            case 'sendAPI':
                sendAPI(cmd.options[0].cmd, {}); 
                break;
            case 'toggleBtn':
            case 'toggleBtnChk':
            case 'toggleBtnGroup':
            case 'toggleBtnGroupCollapse':
            case 'zoomPicture':
            case 'setPlaySettings':
                window[cmd.cmd](event.target, ... cmd.options);
                break;
            case 'toggleBtnChkCollapse':
                window[cmd.cmd](event.target, undefined, ... cmd.options);
                break;
            default:
                window[cmd.cmd](... cmd.options);
        }
    }
    else {
        logError('Can not execute cmd: ' + cmd);
    }
}

function gotoPage(x) {
    switch (x) {
        case 'next':
            app.current.page += settings.maxElementsPerPage;
            break;
        case 'prev':
            app.current.page -= settings.maxElementsPerPage;
            if (app.current.page < 0) {
                app.current.page = 0;
            }
            break;
        default:
            app.current.page = x;
    }
    appGoto(app.current.app, app.current.tab, app.current.view, 
        app.current.page, app.current.filter, app.current.sort, app.current.tag, app.current.search);
}
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

function validateFilenameString(str) {
    if (str === '') {
        return false;
    }
    else if (str.match(/^[\w-.]+$/) !== null) {
        return true;
    }
    else {
        return false;
    }
}

function validateFilename(el) {
    if (validateFilenameString(el.value) === false) {
        el.classList.add('is-invalid');
        return false;
    }
    else {
        el.classList.remove('is-invalid');
        return true;
    }
}

function validateFilenameList(el) {
    el.classList.remove('is-invalid');
    
    let filenames = el.value.split(',');
    for (let i = 0; i < filenames.length; i++) {
        if (validateFilenameString(filenames[i].trim()) === false) {
            el.classList.add('is-invalid');
            return false;
        }
    }
    return true;
}

function validatePath(el) {
    if (el.value === '') {
        el.classList.add('is-invalid');
        return false;
    }
    else if (el.value.match(/^\/[/.\w-]+$/) !== null) {
        el.classList.remove('is-invalid');
        return true;
    }
    else {
        el.classList.add('is-invalid');
        return false;
    }
}

function validatePlnameEl(el) {
    if (validatePlname(el.value) === false) {
        el.classList.add('is-invalid');
        return false;
    }
    else {
        el.classList.remove('is-invalid');
        return true;
    }
}

function validatePlname(x) {
    if (x === '') {
        return false;
    }
    else if (x.match(/\/|\r|\n|"|'/) === null) {
        return true;
    }
    else {
        return false;
    }
}

function validateNotBlank(el) {
    let value = el.value.replace(/\s/g, '');
    if (value === '') {
        el.classList.add('is-invalid');
        return false;
    }
    else {
        el.classList.remove('is-invalid');
        return true;
    }
}

function validateInt(el) {
    let value = el.value.replace(/\d/g, '');
    if (value !== '') {
        el.classList.add('is-invalid');
        return false;
    }
    else {
        el.classList.remove('is-invalid');
        return true;
    }
}

function validateFloat(el) {
    let value = el.value.replace(/[\d-.]/g, '');
    if (value !== '') {
        el.classList.add('is-invalid');
        return false;
    }
    else {
        el.classList.remove('is-invalid');
        return true;
    }
}

function validateStream(el) {
    if (el.value.indexOf('://') > -1) {
        el.classList.remove('is-invalid');
        return true;
    }
    else {
        el.classList.add('is-invalid');
        return false;
    }
}

function validateHost(el) {
    if (el.value.match(/^([\w-.]+)$/) !== null) {
        el.classList.remove('is-invalid');
        return true;
    }
    else {
        el.classList.add('is-invalid');
        return false;
    }
}
