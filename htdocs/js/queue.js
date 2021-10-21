"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function initQueue() {
    document.getElementById('searchqueuestr').addEventListener('keyup', function(event) {
        if (event.key === 'Escape') {
            this.blur();
        }
        else {
            appGoto(app.current.app, app.current.tab, app.current.view, '0', app.current.limit, app.current.filter , app.current.sort, '-', this.value);
        }
    }, false);

    document.getElementById('searchQueueLastPlayedStr').addEventListener('keyup', function(event) {
        if (event.key === 'Escape') {
            this.blur();
        }
        else {
            appGoto(app.current.app, app.current.tab, app.current.view, 
                '0', app.current.limit, app.current.filter, app.current.sort, '-', this.value);
        }
    }, false);

    document.getElementById('searchqueuetags').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            appGoto(app.current.app, app.current.tab, app.current.view, 
                app.current.offset, app.current.limit, getCustomDomProperty(event.target, 'data-tag'), app.current.sort, '-', app.current.search);
        }
    }, false);

    document.getElementById('QueueCurrentList').addEventListener('click', function(event) {
        if (event.target.nodeName === 'TD') {
            clickQueueSong(getCustomDomProperty(event.target.parentNode, 'data-trackid'), getCustomDomProperty(event.target.parentNode, 'data-uri'));
        }
        else if (event.target.nodeName === 'A') {
            showPopover(event);
        }
    }, false);
    
    document.getElementById('QueueLastPlayedList').addEventListener('click', function(event) {
        if (event.target.nodeName === 'TD') {
            clickSong(getCustomDomProperty(event.target.parentNode, 'data-uri'), getCustomDomProperty(event.target.parentNode, 'data-name'));
        }
        else if (event.target.nodeName === 'A') {
            showPopover(event);
        }
    }, false);

    document.getElementById('selectAddToQueueMode').addEventListener('change', function () {
        const value = getSelectValue(this);
        if (value === '2') {
            elDisableId('inputAddToQueueQuantity');
            document.getElementById('inputAddToQueueQuantity').value = '1';
            elDisableId('selectAddToQueuePlaylist');
            document.getElementById('selectAddToQueuePlaylist').value = 'Database';
        }
        else if (value === '1') {
            elEnableId('inputAddToQueueQuantity');
            elEnableId('selectAddToQueuePlaylist');
        }
    });

    document.getElementById('modalAddToQueue').addEventListener('shown.bs.modal', function () {
        removeIsInvalid(document.getElementById('modalAddToQueue'));
        elHideId('warnJukeboxPlaylist2');
        if (features.featPlaylists === true) {
            sendAPI("MYMPD_API_PLAYLIST_LIST", {"searchstr": "", "offset": 0, "limit": 0}, function(obj) { 
                getAllPlaylists(obj, 'selectAddToQueuePlaylist');
            });
        }
    });

    document.getElementById('modalSaveQueue').addEventListener('shown.bs.modal', function () {
        const plName = document.getElementById('saveQueueName');
        plName.focus();
        plName.value = '';
        removeIsInvalid(document.getElementById('modalSaveQueue'));
    });

    document.getElementById('modalSetSongPriority').addEventListener('shown.bs.modal', function () {
        const prioEl = document.getElementById('inputSongPriority');
        prioEl.focus();
        prioEl.value = '';
        removeIsInvalid(document.getElementById('modalSetSongPriority'));
    });
}

function parseUpdateQueue(obj) {
    //Set playstate
    if (obj.result.state === 1) {
        document.getElementById('btnPlay').textContent = 'play_arrow';
        playstate = 'stop';
        domCache.progressBar.style.transition = 'none';
        domCache.progressBar.style.width = '0';
        setTimeout(function() {
            domCache.progressBar.style.transition = progressBarTransition;
        }, 10);
    }
    else if (obj.result.state === 2) {
        document.getElementById('btnPlay').textContent = settings.webuiSettings.uiFooterPlaybackControls === 'stop' ? 'stop' : 'pause';
        playstate = 'play';
    }
    else {
        document.getElementById('btnPlay').textContent = 'play_arrow';
        playstate = 'pause';
    }

    if (obj.result.queueLength === 0) {
        elDisableId('btnPlay');
    }
    else {
        elEnableId('btnPlay');
    }

    mediaSessionSetState();
    mediaSessionSetPositionState(obj.result.totalTime, obj.result.elapsedTime);

    const badgeQueueItemsEl = document.getElementById('badgeQueueItems');
    if (badgeQueueItemsEl) {
        badgeQueueItemsEl.textContent = obj.result.queueLength;
    }
    
    if (obj.result.nextSongPos === -1 && settings.jukeboxMode === false) {
        elDisableId('btnNext');
    }
    else {
        elEnableId('btnNext');
    }
    
    if (obj.result.songPos < 0) {
        elDisableId('btnPrev');
    }
    else {
        elEnableId('btnPrev');
    }
}

function getQueue() {
    if (app.current.search.length >= 2) {
        sendAPI("MYMPD_API_QUEUE_SEARCH", {
            "filter": app.current.filter,
            "offset": app.current.offset,
            "limit": app.current.limit,
            "searchstr": app.current.search,
            "cols": settings.colsQueueCurrentFetch
        }, parseQueue, true);
    }
    else {
        sendAPI("MYMPD_API_QUEUE_LIST", {
            "offset": app.current.offset,
            "limit": app.current.limit,
            "cols": settings.colsQueueCurrentFetch
        }, parseQueue, true);
    }
}

function parseQueue(obj) {
    if (checkResult(obj, 'QueueCurrent', null) === false) {
        return;
    }

    if (obj.result.offset < app.current.offset) {
        gotoPage(obj.result.offset);
        return;
    }

    //goto playing song button
    if (obj.result.totalEntities > 1) {
        elShowId('btnQueueGotoPlayingSong');
    }
    else {
        elHideId('btnQueueGotoPlayingSong');
    }

    const colspan = settings['colsQueueCurrent'].length;
    const smallWidth = window.innerWidth < 576 ? true : false;

    const rowTitle = webuiSettingsDefault.clickQueueSong.validValues[settings.webuiSettings.clickQueueSong];
    updateTable(obj, 'QueueCurrent', function(row, data) {
        data.Pos++;
        row.setAttribute('draggable', 'true');
        row.setAttribute('id','queueTrackId' + data.id);
        row.setAttribute('tabindex', 0);
        row.setAttribute('title', t(rowTitle));
        setCustomDomProperty(row, 'data-trackid', data.id);
        setCustomDomProperty(row, 'data-songpos', data.Pos);
        setCustomDomProperty(row, 'data-duration', data.Duration);
        setCustomDomProperty(row, 'data-uri', data.uri);
        setCustomDomProperty(row, 'data-type', 'song');
        if (data.Album !== undefined) {
            setCustomDomProperty(row, 'data-album', data.Album);
        }
        if (data[tagAlbumArtist] !== undefined) {
            setCustomDomProperty(row, 'data-albumartist', data[tagAlbumArtist]);
        }
    }, function(row, data) {
        tableRow(row, data, app.id, colspan, smallWidth);

        if (lastState && lastState.currentSongId === data.id) {
            setPlayingRow(row, lastState.elapsedTime, data.Duration);
        }
    });

    const table = document.getElementById('QueueCurrentList');
    setCustomDomProperty(table, 'data-version', obj.result.queueVersion);
    const tfoot = table.getElementsByTagName('tfoot')[0];
    if (obj.result.totalTime && obj.result.totalTime > 0 && obj.result.totalEntities <= app.current.limit ) {
        elReplaceChild(tfoot, elCreateNode('tr', {}, 
            elCreateNode('td', {"colspan": (colspan + 1)}, 
                elCreateText('small', {}, tn('Num songs', obj.result.totalEntities) + ' - ' + beautifyDuration(obj.result.totalTime))))
        );
    }
    else if (obj.result.totalEntities > 0) {
        elReplaceChild(tfoot, elCreateNode('tr', {}, 
            elCreateNode('td', {"colspan": (colspan + 1)}, 
                elCreateText('small', {}, tn('Num songs', obj.result.totalEntities))))
        );
    }
    else {
        elClear(tfoot);
    }
}

function queueSetCurrentSong(currentSongId, elapsedTime, totalTime) {
    if (lastState) {
        if (lastState.currentSongId !== currentSongId) {
            const tr = document.getElementById('queueTrackId' + lastState.currentSongId);
            if (tr) {
                const durationTd = tr.querySelector('[data-col=Duration]');
                if (durationTd) {
                    durationTd.textContent = beautifySongDuration(getCustomDomProperty(tr, 'data-duration'));
                }
                const posTd = tr.querySelector('[data-col=Pos]');
                if (posTd) {
                    posTd.classList.remove('mi');
                    posTd.textContent = getCustomDomProperty(tr, 'data-songpos');
                }
                tr.classList.remove('queue-playing');
                tr.style = '';
            }
        }
    }
    const tr = document.getElementById('queueTrackId' + currentSongId);
    if (tr) {
        setPlayingRow(tr, elapsedTime, totalTime);
    }
}

function setPlayingRow(row, elapsedTime, totalTime) {
    const durationTd = row.querySelector('[data-col=Duration]');
    if (durationTd) {
        durationTd.textContent = beautifySongDuration(elapsedTime) + smallSpace + "/" + smallSpace + beautifySongDuration(totalTime);
    }
    const posTd = row.querySelector('[data-col=Pos]');
    if (posTd) {
        if (!posTd.classList.contains('mi')) {
            posTd.classList.add('mi');
            posTd.textContent = 'play_arrow';
        }
    }
    row.classList.add('queue-playing');
    
    let progressPrct = totalTime > 0 ? (100 / totalTime) * elapsedTime : 100;
    if (playstate === 'stop') {
        progressPrct = 100;
    }
    row.style.background = 'linear-gradient(90deg, var(--mympd-highlightcolor) 0%, var(--mympd-highlightcolor) ' +
        progressPrct + '%, transparent ' + progressPrct +'%)';
}

function parseLastPlayed(obj) {
    if (checkResult(obj, 'QueueLastPlayed', null) === false) {
        return;
    }

    const rowTitle = webuiSettingsDefault.clickSong.validValues[settings.webuiSettings.clickSong];
    updateTable(obj, 'QueueLastPlayed', function(row, data) {
        setCustomDomProperty(row, 'data-uri', data.uri);
        setCustomDomProperty(row, 'data-name', data.Title);
        setCustomDomProperty(row, 'data-type', 'song');
        row.setAttribute('tabindex', 0);
        row.setAttribute('title', t(rowTitle));
    });
}

function appendQueue(type, uri, name) {
    switch(type) {
        case 'song':
        case 'dir':
            sendAPI("MYMPD_API_QUEUE_ADD_URI", {"uri": uri});
            showNotification(t('%{name} added to queue', {"name": name}), '', 'queue', 'info');
            break;
        case 'plist':
            sendAPI("MYMPD_API_QUEUE_ADD_PLAYLIST", {"plist": uri});
            showNotification(t('%{name} added to queue', {"name": name}), '', 'queue', 'info');
            break;
    }
}

//eslint-disable-next-line no-unused-vars
function appendAfterQueue(type, uri, to, name) {
    switch(type) {
        case 'song':
            sendAPI("MYMPD_API_QUEUE_ADD_URI_AFTER", {"uri": uri, "to": to});
            to++;
            showNotification(t('%{name} added to queue position %{to}', {"name": name, "to": to}), '', 'queue', 'info');
            break;
    }
}

function replaceQueue(type, uri, name) {
    switch(type) {
        case 'song':
        case 'dir':
            sendAPI("MYMPD_API_QUEUE_REPLACE_URI", {"uri": uri});
            showNotification(t('Queue replaced with %{name}', {"name": name}), '', 'queue', 'info');
            break;
        case 'plist':
            sendAPI("MYMPD_API_QUEUE_REPLACE_PLAYLIST", {"plist": uri});
            showNotification(t('Queue replaced with %{name}', {"name": name}), '', 'queue', 'info');
            break;
    }
}

//eslint-disable-next-line no-unused-vars
function addToQueue() {
    let formOK = true;
    const inputAddToQueueQuantityEl = document.getElementById('inputAddToQueueQuantity');
    if (!validateInt(inputAddToQueueQuantityEl)) {
        formOK = false;
    }
    if (formOK === true) {
        sendAPI("MYMPD_API_QUEUE_ADD_RANDOM", {
            "mode": Number(getSelectValueId('selectAddToQueueMode')),
            "plist": getSelectValueId('selectAddToQueuePlaylist'),
            "quantity": Number(document.getElementById('inputAddToQueueQuantity').value)
        });
        uiElements.modalAddToQueue.hide();
    }
}

//eslint-disable-next-line no-unused-vars
function saveQueue() {
    const plNameEl = document.getElementById('saveQueueName');
    if (validatePlnameEl(plNameEl) === true) {
        sendAPI("MYMPD_API_QUEUE_SAVE", {
            "plist": plNameEl.value
        }, saveQueueCheckError, true);
    }
}

function saveQueueCheckError(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        hideModalAlert();
        uiElements.modalSaveQueue.hide();
    }
}

//eslint-disable-next-line no-unused-vars
function showSetSongPriority(trackId) {
    document.getElementById('inputSongPriorityTrackId').value = trackId;
    uiElements.modalSetSongPriority.show();
}

//eslint-disable-next-line no-unused-vars
function setSongPriority() {
    const trackId = Number(document.getElementById('inputSongPriorityTrackId').value);
    const priorityEl = document.getElementById('inputSongPriority');
    if (validateIntRange(priorityEl, 0, 255) === true) {
        sendAPI("MYMPD_API_QUEUE_PRIO_SET", {
            "songId": trackId,
            "priority": Number(priorityEl.value)
        }, setSongPriorityCheckError, true);
    }
}

function setSongPriorityCheckError(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        hideModalAlert();
        uiElements.modalSetSongPriority.hide();
    }
}

//eslint-disable-next-line no-unused-vars
function delQueueSong(mode, start, end) {
    if (mode === 'range') {
        sendAPI("MYMPD_API_QUEUE_RM_RANGE", {"start": start, "end": end});
    }
    else if (mode === 'single') {
        sendAPI("MYMPD_API_QUEUE_RM_SONG", { "songId": start});
    }
}

//eslint-disable-next-line no-unused-vars
function gotoPlayingSong() {
    if (lastState.songPos >= app.current.offset && lastState.songPos < app.current.offset + app.current.limit) {
        //playing song is in this page
        document.getElementsByClassName('queue-playing')[0].scrollIntoView(true);
    }
    else {
        gotoPage(Math.floor(lastState.songPos / app.current.limit) * app.current.limit);
    }
}

//eslint-disable-next-line no-unused-vars
function playAfterCurrent(songId, songPos) {
    if (settings.random === 0) {
        //not in random mode - move song after current playling song
        sendAPI("MYMPD_API_QUEUE_MOVE_SONG", {
            "from": songPos,
            "to": lastState.songPos !== undefined ? lastState.songPos + 2 : 0
        });
    }
    else {
        //in random mode - set song priority
        sendAPI("MYMPD_API_QUEUE_PRIO_SET_HIGHEST", {"songId": songId});
    }
}

//eslint-disable-next-line no-unused-vars
function clearQueue() {
    showConfirm(tn('Do you really want to clear the queue?'), tn('Yes, clear it'), function() {
        sendAPI("MYMPD_API_QUEUE_CROP_OR_CLEAR", {});
    });
}
