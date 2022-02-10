"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function initQueue() {
    document.getElementById('searchqueuestr').addEventListener('keyup', function(event) {
        clearSearchTimer();
        if (event.key === 'Escape') {
            this.blur();
        }
        else {
            const value = this.value;
            searchTimer = setTimeout(function() {
                appGoto(app.current.card, app.current.tab, app.current.view,
                    0, app.current.limit, app.current.filter , app.current.sort, '-', value);
            }, searchTimerTimeout);
        }
    }, false);

    document.getElementById('searchQueueLastPlayedStr').addEventListener('keyup', function(event) {
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

    document.getElementById('searchqueuetags').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            appGoto(app.current.card, app.current.tab, app.current.view,
                app.current.offset, app.current.limit, getData(event.target, 'tag'), app.current.sort, '-', app.current.search);
        }
    }, false);

    document.getElementById('QueueCurrentList').addEventListener('click', function(event) {
        if (event.target.nodeName === 'TH') {
            return;
        }
        if (event.target.nodeName === 'A') {
            showPopover(event);
            return;
        }
        const target = getParent(event.target, 'TR');
        if (target !== null) {
            clickQueueSong(getData(target, 'trackid'), getData(target, 'uri'));
        }
    }, false);

    document.getElementById('QueueLastPlayedList').addEventListener('click', function(event) {
        if (event.target.nodeName === 'TD') {
            clickSong(getData(event.target.parentNode, 'uri'));
        }
        else if (event.target.nodeName === 'A') {
            showPopover(event);
        }
    }, false);

    document.getElementById('selectAddToQueueMode').addEventListener('change', function() {
        const value = Number(getSelectValue(this));
        if (value === 2) {
            //album mode
            elDisableId('inputAddToQueueQuantity');
            document.getElementById('inputAddToQueueQuantity').value = '1';
            elDisableId('selectAddToQueuePlaylist');
            document.getElementById('selectAddToQueuePlaylist').value = 'Database';
        }
        else if (value === 1) {
            //song mode
            elEnableId('inputAddToQueueQuantity');
            elEnableId('selectAddToQueuePlaylist');
        }
    });

    document.getElementById('modalAddToQueue').addEventListener('shown.bs.modal', function() {
        cleanupModalId('modalAddToQueue');
        document.getElementById('selectAddToQueuePlaylist').value = tn('Database');
        setDataId('selectAddToQueuePlaylist', 'value', 'Database');
        document.getElementById('selectAddToQueuePlaylist').filterInput.value = '';
        if (features.featPlaylists === true) {
            filterPlaylistsSelect(0, 'selectAddToQueuePlaylist', '');
        }
    });

    setDataId('selectAddToQueuePlaylist', 'cb-filter', 'filterPlaylistsSelect');
    setDataId('selectAddToQueuePlaylist', 'cb-filter-options', [0, 'selectAddToQueuePlaylist']);

    document.getElementById('modalSaveQueue').addEventListener('shown.bs.modal', function() {
        const plName = document.getElementById('saveQueueName');
        plName.focus();
        plName.value = '';
        cleanupModalId('modalSaveQueue');
    });

    document.getElementById('modalSetSongPriority').addEventListener('shown.bs.modal', function() {
        const prioEl = document.getElementById('inputSongPriority');
        prioEl.focus();
        prioEl.value = '';
        cleanupModalId('modalSetSongPriority');
    });
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
    //goto playing song button
    if (obj.result &&
        obj.result.totalEntities > 1)
    {
        elEnableId('btnQueueGotoPlayingSong');
    }
    else {
        elDisableId('btnQueueGotoPlayingSong');
    }

    const table = document.getElementById('QueueCurrentList');
    if (checkResultId(obj, 'QueueCurrentList') === false) {
        return;
    }

    if (obj.result.offset < app.current.offset) {
        gotoPage(obj.result.offset);
        return;
    }

    const colspan = settings['colsQueueCurrent'].length;
    const smallWidth = window.innerWidth < 576 ? true : false;

    const rowTitle = webuiSettingsDefault.clickQueueSong.validValues[settings.webuiSettings.clickQueueSong];
    updateTable(obj, 'QueueCurrent', function(row, data) {
        row.setAttribute('draggable', 'true');
        row.setAttribute('id', 'queueTrackId' + data.id);
        row.setAttribute('title', tn(rowTitle));
        setData(row, 'trackid', data.id);
        setData(row, 'songpos', data.Pos);
        setData(row, 'duration', data.Duration);
        setData(row, 'uri', data.uri);
        setData(row, 'type', data.type);
        setData(row, 'name', data.Title);
        if (data.type === 'webradio') {
            setData(row, 'webradioUri', data.webradio.filename);
        }
        //set artist and album data
        if (data.Album !== undefined) {
            setData(row, 'Album', data.Album);
        }
        if (data[tagAlbumArtist] !== undefined) {
            setData(row, 'AlbumArtist', data[tagAlbumArtist]);
        }
        //and other browse tags
        for (const tag of settings.tagListBrowse) {
            if (albumFilters.includes(tag) &&
                data[tag] !== undefined &&
                checkTagValue(data[tag], '-') === false)
            {
                setData(row, tag, data[tag]);
            }
        }

    }, function(row, data) {
        tableRow(row, data, app.id, colspan, smallWidth);
        if (currentState.currentSongId === data.id) {
            setPlayingRow(row);
            setQueueCounter(row, getCounterText());
        }
    });

    const tfoot = table.getElementsByTagName('tfoot')[0];
    if (obj.result.totalTime &&
        obj.result.totalTime > 0 &&
        obj.result.totalEntities <= app.current.limit)
    {
        elReplaceChild(tfoot, elCreateNode('tr', {},
            elCreateNode('td', {"colspan": (colspan + 1)},
                elCreateText('small', {}, tn('Num songs', obj.result.totalEntities) +
                    smallSpace + nDash + smallSpace + beautifyDuration(obj.result.totalTime))))
        );
    }
    else if (obj.result.totalEntities > 0) {
        elReplaceChild(tfoot, elCreateNode('tr', {},
            elCreateNode('td', {"colspan": (colspan + 1)},
                elCreateText('small', {}, tn('Num songs', obj.result.totalEntities))))
        );
    }
}

function queueSetCurrentSong() {
    //remove old playing row
    const old = document.getElementById('queueTrackId' + currentState.lastSongId);
    if (old !== null &&
        old.classList.contains('queue-playing'))
    {
        const durationTd = old.querySelector('[data-col=Duration]');
        if (durationTd) {
            durationTd.textContent = beautifySongDuration(getData(old, 'duration'));
        }
        const posTd = old.querySelector('[data-col=Pos]');
        if (posTd) {
            posTd.classList.remove('mi');
            posTd.textContent = getData(old, 'songpos') + 1;
        }
        old.classList.remove('queue-playing');
        old.style = '';
    }
    //set playing row
    const playingRow = document.getElementById('queueTrackId' + currentState.currentSongId);
    setPlayingRow(playingRow);
}

function setQueueCounter(playingRow, counterText) {
    //calc percent with two decimals after comma
    const progressPrct = currentState.state === 'stop' || currentState.totalTime === 0 ?
            100 : Math.ceil((100 / currentState.totalTime) * currentState.elapsedTime * 100) / 100;
    playingRow.style.background = 'linear-gradient(90deg, var(--mympd-highlightcolor) 0%, var(--mympd-highlightcolor) ' +
        progressPrct + '%, transparent ' + progressPrct + '%, transparent 100%)';
    //counter in queue card
    const durationTd = playingRow.querySelector('[data-col=Duration]');
    if (durationTd) {
        durationTd.textContent = counterText;
    }
}

function setPlayingRow(playingRow) {
    if (playingRow !== null &&
        playingRow.classList.contains('queue-playing') === false)
    {
        const posTd = playingRow.querySelector('[data-col=Pos]');
        if (posTd !== null) {
            posTd.classList.add('mi');
            posTd.textContent = 'play_arrow';
        }
        playingRow.classList.add('queue-playing');
    }
}

function parseLastPlayed(obj) {
    if (checkResultId(obj, 'QueueLastPlayedList') === false) {
        return;
    }

    const rowTitle = webuiSettingsDefault.clickSong.validValues[settings.webuiSettings.clickSong];
    updateTable(obj, 'QueueLastPlayed', function(row, data) {
        setData(row, 'uri', data.uri);
        setData(row, 'name', data.Title);
        setData(row, 'type', 'song');
        row.setAttribute('title', tn(rowTitle));
    });
}

function appendQueue(type, uri, callback) {
    _appendQueue(type, uri, false, callback);
}

function appendPlayQueue(type, uri, callback) {
    _appendQueue(type, uri, true, callback);
}

function _appendQueue(type, uri, play, callback) {
    switch(type) {
        case 'song':
        case 'dir':
        case 'stream':
            sendAPI("MYMPD_API_QUEUE_APPEND_URI", {
                "uri": uri,
                "play": play
            }, callback, true);
            break;
        case 'plist':
        case 'webradio':
            sendAPI("MYMPD_API_QUEUE_APPEND_PLAYLIST", {
                "plist": uri,
                "play": play
            }, callback, true);
            break;
        case 'search':
            sendAPI("MYMPD_API_QUEUE_APPEND_SEARCH", {
                "expression": uri,
                "play": play
            }, callback, true);
            break;
    }
}

//eslint-disable-next-line no-unused-vars
function insertAfterCurrentQueue(type, uri, callback) {
    insertQueue(type, uri, 0, 1, false, callback);
}

//eslint-disable-next-line no-unused-vars
function insertPlayAfterCurrentQueue(type, uri, callback) {
    insertQueue(type, uri, 0, 1, true, callback);
}

function insertQueue(type, uri, to, whence, play, callback) {
    switch(type) {
        case 'song':
        case 'dir':
        case 'stream':
            sendAPI("MYMPD_API_QUEUE_INSERT_URI", {
                "uri": uri,
                "to": to,
                "whence": whence,
                "play": play
            }, callback, true);
            break;
        case 'plist':
        case 'webradio':
            sendAPI("MYMPD_API_QUEUE_INSERT_PLAYLIST", {
                "plist": uri,
                "to": to,
                "whence": whence,
                "play": play
            }, callback, true);
            break;
        case 'search':
            sendAPI("MYMPD_API_QUEUE_INSERT_SEARCH", {
                "expression": uri,
                "to": to,
                "whence": whence,
                "play": play
            }, callback, true);
            break;
    }
}

function replaceQueue(type, uri, callback) {
    _replaceQueue(type, uri, false, callback)
}

function replacePlayQueue(type, uri, callback) {
    _replaceQueue(type, uri, true, callback)
}

function _replaceQueue(type, uri, play, callback) {
    switch(type) {
        case 'song':
        case 'stream':
        case 'dir':
            sendAPI("MYMPD_API_QUEUE_REPLACE_URI", {
                "uri": uri,
                "play": play
            }, callback, true);
            break;
        case 'plist':
        case 'webradio':
            sendAPI("MYMPD_API_QUEUE_REPLACE_PLAYLIST", {
                "plist": uri,
                "play": play
            }, callback, true);
            break;
        case 'search':
            sendAPI("MYMPD_API_QUEUE_REPLACE_SEARCH", {
                "expression": uri,
                "play": play
            }, callback, true);
            break;
    }
}

//eslint-disable-next-line no-unused-vars
function addToQueue() {
    cleanupModalId('modalAddToQueue');
    let formOK = true;
    const inputAddToQueueQuantityEl = document.getElementById('inputAddToQueueQuantity');
    if (!validateInt(inputAddToQueueQuantityEl)) {
        formOK = false;
    }
    const selectAddToQueuePlaylistValue = getDataId('selectAddToQueuePlaylist', 'value');
    if (formOK === true) {
        sendAPI("MYMPD_API_QUEUE_ADD_RANDOM", {
            "mode": Number(getSelectValueId('selectAddToQueueMode')),
            "plist": selectAddToQueuePlaylistValue,
            "quantity": Number(document.getElementById('inputAddToQueueQuantity').value)
        });
        uiElements.modalAddToQueue.hide();
    }
}

//eslint-disable-next-line no-unused-vars
function saveQueue() {
    cleanupModalId('modalSaveQueue');
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
        uiElements.modalSaveQueue.hide();
    }
}

//eslint-disable-next-line no-unused-vars
function showSetSongPriority(trackId) {
    cleanupModalId('modalSetSongPriority');
    document.getElementById('inputSongPriorityTrackId').value = trackId;
    uiElements.modalSetSongPriority.show();
}

//eslint-disable-next-line no-unused-vars
function setSongPriority() {
    cleanupModalId('modalSetSongPriority');

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
        uiElements.modalSetSongPriority.hide();
    }
}

//eslint-disable-next-line no-unused-vars
function delQueueSong(mode, start, end) {
    if (mode === 'range') {
        sendAPI("MYMPD_API_QUEUE_RM_RANGE", {
            "start": start,
            "end": end
        });
    }
    else if (mode === 'single') {
        sendAPI("MYMPD_API_QUEUE_RM_SONG", {
            "songId": start
        });
    }
}

//eslint-disable-next-line no-unused-vars
function gotoPlayingSong() {
    if (currentState.songPos === -1) {
        elDisableId('btnQueueGotoPlayingSong');
        return;
    }
    if (currentState.songPos >= app.current.offset &&
        currentState.songPos < app.current.offset + app.current.limit)
    {
        //playing song is in this page
        document.getElementsByClassName('queue-playing')[0].scrollIntoView(true);
    }
    else {
        gotoPage(Math.floor(currentState.songPos / app.current.limit) * app.current.limit);
    }
}

//eslint-disable-next-line no-unused-vars
function playAfterCurrent(songId, songPos) {
    if (settings.random === false) {
        //not in random mode - move song after current playling song
        sendAPI("MYMPD_API_QUEUE_MOVE_SONG", {
            "from": songPos,
            "to": currentState.songPos !== undefined ? currentState.songPos + 1 : 0
        });
    }
    else {
        //in random mode - set song priority
        sendAPI("MYMPD_API_QUEUE_PRIO_SET_HIGHEST", {
            "songId": songId
        });
    }
}

//eslint-disable-next-line no-unused-vars
function clearQueue() {
    showConfirm(tn('Do you really want to clear the queue?'), tn('Yes, clear it'), function() {
        sendAPI("MYMPD_API_QUEUE_CROP_OR_CLEAR", {});
    });
}
