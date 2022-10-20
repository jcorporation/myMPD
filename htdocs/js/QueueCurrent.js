"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module QueueCurrent_js */

/**
 * Current queue handler
 */
function handleQueueCurrent() {
    setFocusId('searchQueueStr');
    if (features.featAdvqueue === true) {
        createSearchCrumbs(app.current.search, document.getElementById('searchQueueStr'), document.getElementById('searchQueueCrumb'));
    }
    else if (document.getElementById('searchQueueStr').value === '' &&
        app.current.search !== '')
    {
        document.getElementById('searchQueueStr').value = app.current.search;
    }
    if (app.current.search === '') {
        document.getElementById('searchQueueStr').value = '';
    }
    if (features.featAdvqueue === true) {
        if (app.current.sort.tag === '-' ||
            app.current.sort.tag === 'Pos')
        {
            app.current.sort.tag = 'Priority';
        }
        sendAPI("MYMPD_API_QUEUE_SEARCH_ADV", {
            "offset": app.current.offset,
            "limit": app.current.limit,
            "sort": app.current.sort.tag,
            "sortdesc": app.current.sort.desc,
            "expression": app.current.search,
            "cols": settings.colsQueueCurrentFetch
        }, parseQueue, true);
        if (app.current.filter === 'prio') {
            elShowId('priorityMatch');
            document.getElementById('searchQueueMatch').value = '>=';
        }
        else {
            if (getSelectValueId('searchQueueMatch') === '>=') {
                document.getElementById('searchQueueMatch').value = 'contains';
            }
            elHideId('priorityMatch');
        }
    }
    else if (document.getElementById('searchQueueStr').value.length >= 2 ||
             document.getElementById('searchQueueCrumb').children.length > 0)
    {
        sendAPI("MYMPD_API_QUEUE_SEARCH", {
            "offset": app.current.offset,
            "limit": app.current.limit,
            "filter": app.current.filter,
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
    selectTag('searchQueueTags', 'searchQueueTagsDesc', app.current.filter);
}

/**
 * Initializes the current queue elements
 */
function initQueueCurrent() {
    document.getElementById('QueueCurrentList').addEventListener('click', function(event) {
        //popover
        if (event.target.nodeName === 'A') {
            //action td
            handleActionTdClick(event);
            return;
        }
        //table header
        if (event.target.nodeName === 'TH') {
            if (features.featAdvqueue === false) {
                return;
            }
            const colName = event.target.getAttribute('data-col');
            if (colName === null ||
                colName === 'Duration' ||
                colName.indexOf('sticker') === 0 ||
                features.featAdvqueue === false)
            {
                return;
            }
            toggleSort(event.target, colName);
            appGoto(app.current.card, app.current.tab, app.current.view,
                app.current.offset, app.current.limit, app.current.filter, app.current.sort, '-', app.current.search);
            return;
        }
        //table body
        const target = getParent(event.target, 'TR');
        if (target !== null) {
            clickQueueSong(getData(target, 'songid'), getData(target, 'uri'));
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
        setFocus(plName);
        plName.value = '';
        toggleBtnGroupValueId('btnQueueSaveMode', 'create');
        toggleSaveQueueMode(document.getElementById('btnQueueSaveMode').firstElementChild);
        document.getElementById('saveQueueNameSelect').value = '';
        document.getElementById('saveQueueNameSelect').filterInput.value = '';
        filterPlaylistsSelect(1, 'saveQueueNameSelect', '');
        cleanupModalId('modalSaveQueue');
    });

    setDataId('saveQueueNameSelect', 'cb-filter', 'filterPlaylistsSelect');
    setDataId('saveQueueNameSelect', 'cb-filter-options', [1, 'saveQueueNameSelect']);

    document.getElementById('modalSetSongPriority').addEventListener('shown.bs.modal', function() {
        const prioEl = document.getElementById('inputSongPriority');
        setFocus(prioEl);
        prioEl.value = '';
        cleanupModalId('modalSetSongPriority');
    });

    document.getElementById('searchQueueTags').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            app.current.filter = getData(event.target, 'tag');
            getQueue(document.getElementById('searchQueueStr').value);
        }
    }, false);

    document.getElementById('searchQueueStr').addEventListener('keyup', function(event) {
        clearSearchTimer();
        const value = this.value;
        if (event.key === 'Escape') {
            this.blur();
        }
        else if (event.key === 'Enter' &&
            features.featAdvqueue === true)
        {
            if (value !== '') {
                const op = getSelectValueId('searchQueueMatch');
                document.getElementById('searchQueueCrumb').appendChild(createSearchCrumb(app.current.filter, op, value));
                elShowId('searchQueueCrumb');
                this.value = '';
            }
            else {
                searchTimer = setTimeout(function() {
                    getQueue(value);
                }, searchTimerTimeout);
            }
        }
        else {
            searchTimer = setTimeout(function() {
                getQueue(value);
            }, searchTimerTimeout);
        }
    }, false);

    document.getElementById('searchQueueCrumb').addEventListener('click', function(event) {
        if (event.target.nodeName === 'SPAN') {
            //remove search expression
            event.preventDefault();
            event.stopPropagation();
            event.target.parentNode.remove();
            getQueue('');
        }
        else if (event.target.nodeName === 'BUTTON') {
            //edit search expression
            event.preventDefault();
            event.stopPropagation();
            document.getElementById('searchQueueStr').value = unescapeMPD(getData(event.target, 'filter-value'));
            selectTag('searchQueueTags', 'searchQueueTagsDesc', getData(event.target, 'filter-tag'));
            document.getElementById('searchQueueMatch').value = getData(event.target, 'filter-op');
            event.target.remove();
            app.current.filter = getData(event.target,'filter-tag');
            getQueue(document.getElementById('searchQueueStr').value);
            if (document.getElementById('searchQueueCrumb').childElementCount === 0) {
                elHideId('searchQueueCrumb');
            }
        }
    }, false);

    document.getElementById('searchQueueMatch').addEventListener('change', function() {
        getQueue(document.getElementById('searchQueueStr').value);
    }, false);
}

/**
 * Wrapper for queue search that respects featAdvqueue
 * @param {*} value search value
 */
function getQueue(value) {
    if (features.featAdvqueue) {
        const expression = createSearchExpression(document.getElementById('searchQueueCrumb'), app.current.filter, getSelectValueId('searchQueueMatch'), value);
        appGoto('Queue', 'Current', undefined, 0, app.current.limit, app.current.filter, app.current.sort, '-', expression, 0);
    }
    else {
        appGoto('Queue', 'Current', undefined, 0, app.current.limit, app.current.filter, app.current.sort, '-', value, 0);
    }
}

/**
 * Parses the queue list and search responses
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
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
        gotoPage(obj.result.offset, undefined);
        return;
    }

    const colspan = settings['colsQueueCurrent'].length;
    const smallWidth = uiSmallWidthTagRows();

    const rowTitle = webuiSettingsDefault.clickQueueSong.validValues[settings.webuiSettings.clickQueueSong];
    updateTable(obj, 'QueueCurrent', function(row, data) {
        row.setAttribute('draggable', 'true');
        row.setAttribute('id', 'queueSongId' + data.id);
        row.setAttribute('title', tn(rowTitle));
        setData(row, 'songid', data.id);
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

    const tfoot = table.querySelector('tfoot');
    if (obj.result.totalTime &&
        obj.result.totalTime > 0 &&
        obj.result.totalEntities <= app.current.limit)
    {
        elReplaceChild(tfoot,
            elCreateNode('tr', {},
                elCreateNode('td', {"colspan": (colspan + 1)},
                    elCreateNodes('small', {}, [
                        elCreateTextTnNr('span', {}, 'Num songs', obj.result.totalEntities),
                        elCreateText('span', {}, smallSpace + nDash + smallSpace + fmtDuration(obj.result.totalTime))
                    ])
                )
            )
        );
    }
    else if (obj.result.totalEntities > 0) {
        elReplaceChild(tfoot,
            elCreateNode('tr', {},
                elCreateNode('td', {"colspan": (colspan + 1)},
                    elCreateTextTnNr('small', {}, 'Num songs', obj.result.totalEntities)
                )
            )
        );
    }
}

/**
 * Removes the old playing row and sets the new playing row in the queue view
 */
function queueSetCurrentSong() {
    //remove old playing row
    const old = document.getElementById('queueSongId' + currentState.lastSongId);
    if (old !== null &&
        old.classList.contains('queue-playing'))
    {
        const durationTd = old.querySelector('[data-col=Duration]');
        if (durationTd) {
            durationTd.textContent = fmtSongDuration(getData(old, 'duration'));
        }
        const posTd = old.querySelector('[data-col=Pos]');
        if (posTd) {
            posTd.classList.remove('mi');
            posTd.textContent = getData(old, 'songpos') + 1;
        }
        old.classList.remove('queue-playing');
    }
    //set playing row
    setPlayingRow();
}

/**
 * Sets the playing progress in the queue view
 * @param {HTMLElement} playingRow the playing row element
 * @param {string} counterText text to set for the duration
 */
function setQueueCounter(playingRow, counterText) {
    if (userAgentData.isSafari === false) {
        //safari does not support gradient backgrounds at row level
        //calc percent with two decimals after comma
        const progressPrct = currentState.state === 'stop' || currentState.totalTime === 0 ?
                100 : Math.ceil((100 / currentState.totalTime) * currentState.elapsedTime * 100) / 100;
        playingRow.style.background = 'linear-gradient(90deg, var(--mympd-highlightcolor) 0%, var(--mympd-highlightcolor) ' +
            progressPrct + '%, transparent ' + progressPrct + '%, transparent 100%)';
    }
    //counter in queue card
    const durationTd = playingRow.querySelector('[data-col=Duration]');
    if (durationTd) {
        durationTd.textContent = counterText;
    }
}

/**
 * Sets the playing song in the current queue view
 * @param {HTMLElement} [playingRow] playing row element
 */
function setPlayingRow(playingRow) {
    if (playingRow === undefined) {
        playingRow = document.getElementById('queueSongId' + currentState.currentSongId);
    }
    if (playingRow !== null) {
        const posTd = playingRow.querySelector('[data-col=Pos]');
        if (posTd !== null) {
            posTd.classList.add('mi');
            posTd.textContent = currentState.state === 'play' ? 'play_arrow' :
                currentState.state === 'pause' ? 'pause' : 'stop';
        }
        playingRow.classList.add('queue-playing');
    }
}

/**
 * Appends an element to the queue
 * @param {string} type element type: song, dir, stream, plist, smartpls, webradio, search
 * @param {string} uri element uri
 * @param {Function} [callback] callback function
 */
function appendQueue(type, uri, callback) {
    _appendQueue(type, uri, false, callback);
}

/**
 * Appends an element to the queue and plays it
 * @param {string} type element type: song, dir, stream, plist, smartpls, webradio, search
 * @param {string} uri element uri
 * @param {Function} [callback] callback function
 */
function appendPlayQueue(type, uri, callback) {
    _appendQueue(type, uri, true, callback);
}

/**
 * Appends an element to the queue
 * @param {string} type element type: song, dir, stream, plist, smartpls, webradio, search
 * @param {string} uri element uri
 * @param {boolean} play true = play added entry, false = append only
 * @param {Function} callback callback function
 */
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
        case 'smartpls':
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

/**
 * Inserts the element after the current playing song
 * @param {string} type element type: song, dir, stream, plist, smartpls, webradio, search
 * @param {string} uri element uri
 * @param {Function} [callback] callback function
 */
//eslint-disable-next-line no-unused-vars
function insertAfterCurrentQueue(type, uri, callback) {
    insertQueue(type, uri, 0, 1, false, callback);
}

/**
 * Inserts the element after the current playing song
 * @param {string} type element type: song, dir, stream, plist, smartpls, webradio, search
 * @param {string} uri element uri
 * @param {Function} [callback] callback function
 */
//eslint-disable-next-line no-unused-vars
function insertPlayAfterCurrentQueue(type, uri, callback) {
    insertQueue(type, uri, 0, 1, true, callback);
}

/**
 * Inserts the element in the queue
 * @param {string} type element type: song, dir, stream, plist, smartpls, webradio, search
 * @param {string} uri element uri
 * @param {number} to position to insert
 * @param {number} whence how t interpret the to parameter: 0 = absolute, 1 = after, 2 = before current song
 * @param {boolean} play true = play added entry, false = insert only
 * @param {Function} callback callback function
 */
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
        case 'smartpls':
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

/**
 * Replaces the queue with the element
 * @param {string} type element type: song, dir, stream, plist, smartpls, webradio, search
 * @param {string} uri element uri
 * @param {Function} [callback] callback function
 */
function replaceQueue(type, uri, callback) {
    _replaceQueue(type, uri, false, callback)
}

/**
 * Replaces the queue with the element and plays it
 * @param {string} type element type: song, dir, stream, plist, smartpls, webradio, search
 * @param {string} uri element uri
 * @param {Function} [callback] callback function
 */
function replacePlayQueue(type, uri, callback) {
    _replaceQueue(type, uri, true, callback)
}

/**
 * Replaces the queue with the element
 * @param {string} type element type: song, dir, stream, plist, smartpls, webradio, search
 * @param {string} uri element uri
 * @param {boolean} play true = play added entry, false = insert only
 * @param {Function} callback callback function
 */
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
        case 'smartpls':
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

/**
 * Adds random songs/albums to the queue, one-shot jukebox mode.
 */
//eslint-disable-next-line no-unused-vars
function addRandomToQueue() {
    cleanupModalId('modalAddToQueue');
    let formOK = true;
    const inputAddToQueueQuantityEl = document.getElementById('inputAddToQueueQuantity');
    if (!validateIntEl(inputAddToQueueQuantityEl)) {
        formOK = false;
    }
    const selectAddToQueuePlaylistValue = getDataId('selectAddToQueuePlaylist', 'value');
    if (formOK === true) {
        sendAPI("MYMPD_API_QUEUE_ADD_RANDOM", {
            "mode": Number(getSelectValueId('selectAddToQueueMode')),
            "plist": selectAddToQueuePlaylistValue,
            "quantity": Number(document.getElementById('inputAddToQueueQuantity').value)
        }, null, false);
        uiElements.modalAddToQueue.hide();
    }
}

/**
 * Toggles the queue save mode options
 * @param {EventTarget} target triggering element
 */
//eslint-disable-next-line no-unused-vars
function toggleSaveQueueMode(target) {
    toggleBtnGroup(target);
    const value = getData(target, 'value');
    if (value === 'create') {
        elShowId('rowSaveQueueName');
        elHideId('rowSaveQueueNameSelect');
    }
    else {
        elHideId('rowSaveQueueName');
        elShowId('rowSaveQueueNameSelect');
    }
}

/**
 * Saves the queue as a playlist
 */
//eslint-disable-next-line no-unused-vars
function saveQueue() {
    cleanupModalId('modalSaveQueue');
    const plNameEl = document.getElementById('saveQueueName');
    let name = plNameEl.value;
    let saveMode = 'create';
    let formOK = true;
    if (features.featAdvqueue === true) {
        //support queue save modes (since MPD 0.24)
        saveMode = getBtnGroupValueId('btnQueueSaveMode');
        if (saveMode !== 'create') {
            //append or replace existing playlist
            name = getDataId('saveQueueNameSelect', 'value');
        }
        else {
            formOK = validatePlistEl(plNameEl);
        }
    }
    else {
        formOK = validatePlistEl(plNameEl);
    }
    if (formOK === true) {
        sendAPI("MYMPD_API_QUEUE_SAVE", {
            "plist": name,
            "mode": saveMode
        }, saveQueueCheckError, true);
    }
}

/**
 * Handler for the MYMPD_API_QUEUE_SAVE jsonrpc response
 * @param {object} obj jsonrpc response
 */
function saveQueueCheckError(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        uiElements.modalSaveQueue.hide();
    }
}

/**
 * Shows the set song priority modal
 * @param {number} songId the mpd song id
 */
//eslint-disable-next-line no-unused-vars
function showSetSongPriority(songId) {
    cleanupModalId('modalSetSongPriority');
    document.getElementById('inputSongPrioritySondId').value = songId;
    uiElements.modalSetSongPriority.show();
}

/**
 * Sets song priority
 */
//eslint-disable-next-line no-unused-vars
function setSongPriority() {
    cleanupModalId('modalSetSongPriority');

    const songId = Number(document.getElementById('inputSongPrioritySongId').value);
    const priorityEl = document.getElementById('inputSongPriority');
    if (validateIntRangeEl(priorityEl, 0, 255) === true) {
        sendAPI("MYMPD_API_QUEUE_PRIO_SET", {
            "songId": songId,
            "priority": Number(priorityEl.value)
        }, setSongPriorityCheckError, true);
    }
}

/**
 * Handles the MYMPD_API_QUEUE_PRIO_SET jsonrpc response
 * @param {object} obj jsonrpc response
 */
function setSongPriorityCheckError(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        uiElements.modalSetSongPriority.hide();
    }
}

/**
 * Removes song(s) from the queue
 * @param {string} mode range or single
 * @param {number} start start of the range (including) or song id to remove
 * @param {number} [end] end of the range (excluding), -1 for open end
 */
//eslint-disable-next-line no-unused-vars
function removeFromQueue(mode, start, end) {
    if (mode === 'range') {
        sendAPI("MYMPD_API_QUEUE_RM_RANGE", {
            "start": start,
            "end": end
        }, null, false);
    }
    else if (mode === 'single') {
        sendAPI("MYMPD_API_QUEUE_RM_SONG", {
            "songId": start
        }, null, false);
    }
}

/**
 * Scrolls to the current song
 * @returns {void}
 */
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
        document.querySelector('queue-playing').scrollIntoView(true);
    }
    else {
        gotoPage(Math.floor(currentState.songPos / app.current.limit) * app.current.limit, undefined);
    }
}

/**
 * Plays the selected song after the current song.
 * Uses the priority if in random mode else moves the song after current playing song.
 * @param {number} songId current playing song id (for priority mode)
 * @param {number} songPos current playing song position (for move mode)
 */
//eslint-disable-next-line no-unused-vars
function playAfterCurrent(songId, songPos) {
    if (settings.partition.random === false) {
        //not in random mode - move song after current playling song
        sendAPI("MYMPD_API_QUEUE_MOVE_SONG", {
            "from": songPos,
            "to": currentState.songPos !== undefined ? currentState.songPos + 1 : 0
        }, null, false);
    }
    else {
        //in random mode - set song priority
        sendAPI("MYMPD_API_QUEUE_PRIO_SET_HIGHEST", {
            "songId": songId
        }, null, false);
    }
}

/**
 * Clears or crops the queue after confirmation
 */
//eslint-disable-next-line no-unused-vars
function clearQueue() {
    showConfirm(tn('Do you really want to clear the queue?'), tn('Yes, clear it'), function() {
        sendAPI("MYMPD_API_QUEUE_CROP_OR_CLEAR", {}, null, false);
    });
}
