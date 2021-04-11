"use strict";
// SPDX-License-Identifier: GPL-2.0-or-later
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

    document.getElementById('searchqueuetags').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            appGoto(app.current.app, app.current.tab, app.current.view, 
                app.current.offset, app.current.limit, getAttDec(event.target, 'data-tag'), app.current.sort, '-', app.current.search);
        }
    }, false);

    document.getElementById('QueueCurrentList').addEventListener('click', function(event) {
        if (event.target.nodeName === 'TD') {
            clickQueueSong(getAttDec(event.target.parentNode, 'data-trackid'), getAttDec(event.target.parentNode, 'data-uri'));
        }
        else if (event.target.nodeName === 'A') {
            showMenu(event.target, event);
        }
    }, false);
    
    document.getElementById('QueueLastPlayedList').addEventListener('click', function(event) {
        if (event.target.nodeName === 'TD') {
            clickSong(getAttDec(event.target.parentNode, 'data-uri'), getAttDec(event.target.parentNode, 'data-name'));
        }
        else if (event.target.nodeName === 'A') {
            showMenu(event.target, event);
        }
    }, false);

    document.getElementById('selectAddToQueueMode').addEventListener('change', function () {
        const value = getSelectValue(this);
        if (value === '2') {
            disableEl('inputAddToQueueQuantity');
            document.getElementById('inputAddToQueueQuantity').value = '1';
            disableEl('selectAddToQueuePlaylist');
            document.getElementById('selectAddToQueuePlaylist').value = 'Database';
        }
        else if (value === '1') {
            enableEl('inputAddToQueueQuantity');
            enableEl('selectAddToQueuePlaylist');
        }
    });

    document.getElementById('modalAddToQueue').addEventListener('shown.bs.modal', function () {
        removeIsInvalid(document.getElementById('modalAddToQueue'));
        document.getElementById('warnJukeboxPlaylist2').classList.add('hide');
        if (settings.featPlaylists === true) {
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
}

function parseUpdateQueue(obj) {
    //Set playstate
    if (obj.result.state === 1) {
        document.getElementById('btnPlay').innerText = 'play_arrow';
        playstate = 'stop';
        domCache.progressBar.style.transition = 'none';
        domCache.progressBar.style.width = '0';
        setTimeout(function() {
            domCache.progressBar.style.transition = progressBarTransition;
        }, 10);
    }
    else if (obj.result.state === 2) {
        document.getElementById('btnPlay').innerText = settings.advanced.uiFooterPlaybackControls === 'stop' ? 'stop' : 'pause';
        playstate = 'play';
    }
    else {
        document.getElementById('btnPlay').innerText = 'play_arrow';
        playstate = 'pause';
    }

    if (obj.result.queueLength === 0) {
        disableEl('btnPlay');
    }
    else {
        enableEl('btnPlay');
    }

    mediaSessionSetState();
    mediaSessionSetPositionState(obj.result.totalTime, obj.result.elapsedTime);

    const badgeQueueItemsEl = document.getElementById('badgeQueueItems');
    if (badgeQueueItemsEl) {
        badgeQueueItemsEl.innerText = obj.result.queueLength;
    }
    
    if (obj.result.nextSongPos === -1 && settings.jukeboxMode === false) {
        disableEl('btnNext');
    }
    else {
        enableEl('btnNext');
    }
    
    if (obj.result.songPos < 0) {
        disableEl('btnPrev');
    }
    else {
        enableEl('btnPrev');
    }
}

function getQueue() {
    if (app.current.search.length >= 2) {
        sendAPI("MYMPD_API_QUEUE_SEARCH", {"filter": app.current.filter, "offset": app.current.offset, "limit": app.current.limit, "searchstr": app.current.search, "cols": settings.colsQueueCurrent}, parseQueue, false);
    }
    else {
        sendAPI("MYMPD_API_QUEUE_LIST", {"offset": app.current.offset, "limit": app.current.limit, "cols": settings.colsQueueCurrent}, parseQueue, false);
    }
}

function parseQueue(obj) {
    if (obj.result.offset < app.current.offset) {
        gotoPage(obj.result.offset);
        return;
    }

    //goto playing song button
    if (obj.result.totalEntities > app.current.limit && app.current.limit !== 0) {
        document.getElementById('btnQueueGotoPlayingSong').parentNode.classList.remove('hide');
    }
    else {
        document.getElementById('btnQueueGotoPlayingSong').parentNode.classList.add('hide');
    }

    const rowTitle = advancedSettingsDefault.clickQueueSong.validValues[settings.advanced.clickQueueSong];
    updateTable(obj, 'QueueCurrent', function(row, data) {
        data.Pos++;
        row.setAttribute('draggable', 'true');
        row.setAttribute('id','queueTrackId' + data.id);
        row.setAttribute('tabindex', 0);
        row.setAttribute('title', t(rowTitle));
        setAttEnc(row, 'data-trackid', data.id);
        setAttEnc(row, 'data-songpos', data.Pos);
        setAttEnc(row, 'data-duration', data.Duration);
        setAttEnc(row, 'data-uri', data.uri);
        setAttEnc(row, 'data-type', 'song');
    });

    const table = document.getElementById('QueueCurrentList');
    setAttEnc(table, 'data-version', obj.result.queueVersion);
    const colspan = settings['colsQueueCurrent'].length;
    const tfoot = table.getElementsByTagName('tfoot')[0];
    if (obj.result.totalTime && obj.result.totalTime > 0 && obj.result.totalEntities <= app.current.limit ) {
        tfoot.innerHTML = '<tr><td colspan="' + (colspan + 1) + '"><small>' + t('Num songs', obj.result.totalEntities) + '&nbsp;&ndash;&nbsp;' + beautifyDuration(obj.result.totalTime) + '</small></td></tr>';
    }
    else if (obj.result.totalEntities > 0) {
        tfoot.innerHTML = '<tr><td colspan="' + (colspan + 1) + '"><small>' + t('Num songs', obj.result.totalEntities) + '</small></td></tr>';
    }
    else {
        tfoot.innerHTML = '';
    }
}

function parseLastPlayed(obj) {
    const rowTitle = advancedSettingsDefault.clickSong.validValues[settings.advanced.clickSong];
    updateTable(obj, 'QueueLastPlayed', function(row, data) {
        setAttEnc(row, 'data-uri', data.uri);
        setAttEnc(row, 'data-name', data.Title);
        setAttEnc(row, 'data-type', 'song');
        row.setAttribute('tabindex', 0);
        row.setAttribute('title', t(rowTitle));
    });
}

//eslint-disable-next-line no-unused-vars
function queueSelectedItem(append) {
    const item = document.activeElement;
    if (item) {
        if (item.parentNode.parentNode.id === 'QueueCurrentList') {
            return;
        }
        if (append === true) {
            appendQueue(getAttDec(item, 'data-type'), getAttDec(item, 'data-uri'), getAttDec(item, 'data-name'));
        }
        else {
            replaceQueue(getAttDec(item, 'data-type'), getAttDec(item, 'data-uri'), getAttDec(item, 'data-name'));
        }
    }
}

//eslint-disable-next-line no-unused-vars
function dequeueSelectedItem() {
    const item = document.activeElement;
    if (item) {
        if (item.parentNode.parentNode.id !== 'QueueCurrentList') {
            return;
        }
        delQueueSong('single', getAttDec(item, 'data-trackid'));
    }
}

function appendQueue(type, uri, name) {
    switch(type) {
        case 'song':
        case 'dir':
            sendAPI("MYMPD_API_QUEUE_ADD_TRACK", {"uri": uri});
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
            sendAPI("MYMPD_API_QUEUE_ADD_TRACK_AFTER", {"uri": uri, "to": to});
            to++;
            showNotification(t('%{name} added to queue position %{to}', {"name": name, "to": to}), '', 'queue', 'info');
            break;
    }
}

function replaceQueue(type, uri, name) {
    switch(type) {
        case 'song':
        case 'dir':
            sendAPI("MYMPD_API_QUEUE_REPLACE_TRACK", {"uri": uri});
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
            "mode": getSelectValue('selectAddToQueueMode'),
            "playlist": getSelectValue('selectAddToQueuePlaylist'),
            "quantity": document.getElementById('inputAddToQueueQuantity').value
        });
        uiElements.modalAddToQueue.hide();
    }
}

//eslint-disable-next-line no-unused-vars
function saveQueue() {
    const plName = document.getElementById('saveQueueName').value;
    if (validatePlname(plName) === true) {
        sendAPI("MYMPD_API_QUEUE_SAVE", {"plist": plName});
        uiElements.modalSaveQueue.hide();
    }
    else {
        document.getElementById('saveQueueName').classList.add('is-invalid');
    }
}

function delQueueSong(mode, start, end) {
    if (mode === 'range') {
        sendAPI("MYMPD_API_QUEUE_RM_RANGE", {"start": start, "end": end});
    }
    else if (mode === 'single') {
        sendAPI("MYMPD_API_QUEUE_RM_TRACK", { "track": start});
    }
}

//eslint-disable-next-line no-unused-vars
function gotoPlayingSong() {
    if (app.current.limit === 0) {
        return;
    }
    gotoPage(lastState.songPos < app.current.limit ? 0 : Math.floor(lastState.songPos / app.current.limit) * app.current.limit);
}

//eslint-disable-next-line no-unused-vars
function playAfterCurrent(trackid, songpos) {
    if (settings.random === 0) {
        //not in random mode - move song after current playling song
        sendAPI("MYMPD_API_QUEUE_MOVE_TRACK", {
            "from": songpos,
            "to": lastState.songPos !== undefined ? lastState.songPos + 2 : 0
        });
    }
    else {
        //in random mode - set song priority
        sendAPI("MYMPD_API_QUEUE_PRIO_SET_HIGHEST", {"trackid": trackid});
    }
}

//eslint-disable-next-line no-unused-vars
function clearQueue() {
    showConfirm(t('Do you really want to clear the queue?'), "Yes, clear it", function() {
        sendAPI("MYMPD_API_QUEUE_CROP_OR_CLEAR", {});
    });
}
