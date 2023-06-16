"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module QueueCurrent_js */

/**
 * Current queue handler
 * @returns {void}
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
 * @returns {void}
 */
function initQueueCurrent() {
    document.getElementById('QueueCurrentList').addEventListener('click', function(event) {
        //select mode
        if (selectRow(event) === true) {
            return;
        }
        //action td
        if (event.target.nodeName === 'A') {
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
                colName.indexOf('sticker') === 0)
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
        if (checkTargetClick(target) === true) {
            clickQueueSong(getData(target, 'songid'), getData(target, 'uri'), event);
        }
    }, false);

    document.getElementById('dropdownQueueCurrentSelection').parentNode.addEventListener('show.bs.dropdown', function() {
        showSelectionCount();
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
            filterPlaylistsSelect(0, 'selectAddToQueuePlaylist', '', 'Database');
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
        filterPlaylistsSelect(1, 'saveQueueNameSelect', '', '');
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
        //handle Enter key on keydown for IME composing compatibility
        if (event.key !== 'Enter' ||
            features.featAdvqueue === false)
        {
            return;
        }
        clearSearchTimer();
        const value = this.value;
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
    }, false);

    document.getElementById('searchQueueStr').addEventListener('keyup', function(event) {
        if (ignoreKeys(event) === true) {
            return;
        }
        clearSearchTimer();
        const value = this.value;
        searchTimer = setTimeout(function() {
            getQueue(value);
        }, searchTimerTimeout);
    }, false);

    document.getElementById('searchQueueCrumb').addEventListener('click', function(event) {
        if (event.target.nodeName === 'SPAN') {
            //remove search expression
            event.preventDefault();
            event.stopPropagation();
            event.target.parentNode.remove();
            getQueue('');
            document.getElementById('searchQueueStr').updateBtn();
        }
        else if (event.target.nodeName === 'BUTTON') {
            //edit search expression
            event.preventDefault();
            event.stopPropagation();
            const searchQueueStrEl = document.getElementById('searchQueueStr');
            searchQueueStrEl.value = unescapeMPD(getData(event.target, 'filter-value'));
            selectTag('searchQueueTags', 'searchQueueTagsDesc', getData(event.target, 'filter-tag'));
            document.getElementById('searchQueueMatch').value = getData(event.target, 'filter-op');
            event.target.remove();
            app.current.filter = getData(event.target,'filter-tag');
            getQueue(searchQueueStrEl.value);
            if (document.getElementById('searchQueueCrumb').childElementCount === 0) {
                elHideId('searchQueueCrumb');
            }
            searchQueueStrEl.updateBtn();
        }
    }, false);

    document.getElementById('searchQueueMatch').addEventListener('change', function() {
        getQueue(document.getElementById('searchQueueStr').value);
    }, false);
}

/**
 * Wrapper for queue search that respects featAdvqueue
 * @param {string} value search value
 * @returns {void}
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
        if (features.featAdvqueue === false ||
            app.current.sort.tag === 'Priority')
        {
            row.setAttribute('draggable', 'true');
        }
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
            if (currentState.state === 'play') {
                setQueueCounter(row, getCounterText());
            }
        }
    });

    const tfoot = table.querySelector('tfoot');
    if (obj.result.totalTime &&
        obj.result.totalTime > 0 &&
        obj.result.totalEntities <= app.current.limit)
    {
        elReplaceChild(tfoot,
            elCreateNode('tr', {"class": ["not-clickable"]},
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
            elCreateNode('tr', {"class": ["not-clickable"]},
                elCreateNode('td', {"colspan": (colspan + 1)},
                    elCreateTextTnNr('small', {}, 'Num songs', obj.result.totalEntities)
                )
            )
        );
    }
}

/**
 * Handles the selection actions
 * @param {string} action action to handle
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function queueSelectionAction(action) {
    const table = document.getElementById(app.id + 'List');
    switch(action) {
        case 'playAfterCurrent': {
            const songIds = getSelectedRowData(table, 'songid');
            playAfterCurrent(songIds);
            break;
        }
        case 'remove': {
            const songIds = getSelectedRowData(table, 'songid');
            removeFromQueueIDs(songIds);
            break;
        }
        case 'addToPlaylist': {
            const uris = getSelectedRowData(table, 'uri');
            showAddToPlaylist(uris, '');
            break;
        }
    }
}

/**
 * Removes the old playing row and sets the new playing row in the queue view
 * @returns {void}
 */
function queueSetCurrentSong() {
    //remove old playing row
    const old = document.getElementById('queueSongId' + currentState.lastSongId);
    if (old !== null) {
        resetDuration(old);
        resetSongPos(old);
    }
    //set playing row
    setPlayingRow();
}

/**
 * Resets the duration in playing row and footer
 * @param {HTMLElement} [playingRow] current playing row in queue card
 * @returns {void}
 */
function resetDuration(playingRow) {
    //counter in footer
    elClear(domCache.counter);
    //counter in queue
    if (playingRow === undefined) {
        playingRow = document.getElementById('queueSongId' + currentState.currentSongId);
    }
    const durationTd = playingRow.querySelector('[data-col=Duration]');
    if (durationTd) {
        durationTd.textContent = fmtSongDuration(getData(playingRow, 'duration'));
    }
    
    playingRow.classList.remove('queue-playing');
    playingRow.style.removeProperty('background');
}

/**
 * Resets the position in playing row
 * @param {HTMLElement} [playingRow] current playing row in queue card
 * @returns {void}
 */
function resetSongPos(playingRow) {
    if (playingRow === undefined) {
        playingRow = document.getElementById('queueSongId' + currentState.currentSongId);
    }
    const posTd = playingRow.querySelector('[data-col=Pos]');
    if (posTd) {
        posTd.classList.remove('mi');
        posTd.textContent = getData(playingRow, 'songpos') + 1;
    }
}

/**
 * Sets the playing progress in the queue view
 * @param {HTMLElement} playingRow the playing row element
 * @param {string} counterText text to set for the duration
 * @returns {void}
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
 * @returns {void}
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
 * Sets the clickable class for current queue table header,
 * if mpd supports queue sorting (since MPD 0.24)
 * @returns {void}
 */
function setQueueCurrentHeaderClickable() {
    const ths = document.querySelectorAll('#QueueCurrentList > thead > tr > th');
    for (const th of ths) {
        const colName = th.getAttribute('data-col');
        if (colName === null ||
            colName === 'Duration' ||
            colName.indexOf('sticker') === 0)
        {
            continue;
        }
        if (features.featAdvqueue === true) {
            th.classList.add('clickable');
        }
        else {
            th.classList.remove('clickable');
        }
    }
}

/**
 * Appends an element to the queue
 * @param {string} type element type: song, dir, stream, plist, smartpls, webradio, search
 * @param {Array} uris element uris
 * @param {Function} [callback] callback function
 * @returns {void}
 */
function appendQueue(type, uris, callback) {
    _appendQueue(type, uris, false, callback);
}

/**
 * Appends an element to the queue and plays it
 * @param {string} type element type: song, dir, stream, plist, smartpls, webradio, search
 * @param {Array} uris element uris
 * @param {Function} [callback] callback function
 * @returns {void}
 */
function appendPlayQueue(type, uris, callback) {
    _appendQueue(type, uris, true, callback);
}

/**
 * Appends elements to the queue
 * @param {string} type element type: song, dir, stream, plist, smartpls, webradio, search
 * @param {Array} uris element uris
 * @param {boolean} play true = play added entry, false = append only
 * @param {Function} callback callback function
 * @returns {void}
 */
function _appendQueue(type, uris, play, callback) {
    switch(type) {
        case 'song':
        case 'dir':
        case 'stream':
            sendAPI("MYMPD_API_QUEUE_APPEND_URIS", {
                "uris": uris,
                "play": play
            }, callback, true);
            break;
        case 'plist':
        case 'smartpls':
        case 'webradio':
            sendAPI("MYMPD_API_QUEUE_APPEND_PLAYLISTS", {
                "plists": uris,
                "play": play
            }, callback, true);
            break;
        case 'search':
            sendAPI("MYMPD_API_QUEUE_APPEND_SEARCH", {
                "expression": uris[0],
                "play": play
            }, callback, true);
            break;
    }
}

/**
 * Inserts the element after the current playing song
 * @param {string} type element type: song, dir, stream, plist, smartpls, webradio, search
 * @param {Array} uris element uris
 * @param {Function} [callback] callback function
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function insertAfterCurrentQueue(type, uris, callback) {
    insertQueue(type, uris, 0, 1, false, callback);
}

/**
 * Inserts the element after the current playing song
 * @param {string} type element type: song, dir, stream, plist, smartpls, webradio, search
 * @param {Array} uris element uris
 * @param {Function} [callback] callback function
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function insertPlayAfterCurrentQueue(type, uris, callback) {
    insertQueue(type, uris, 0, 1, true, callback);
}

/**
 * Inserts elements into the queue
 * @param {string} type element type: song, dir, stream, plist, smartpls, webradio, search
 * @param {Array} uris element uris
 * @param {number} to position to insert
 * @param {number} whence how t interpret the to parameter: 0 = absolute, 1 = after, 2 = before current song
 * @param {boolean} play true = play added entry, false = insert only
 * @param {Function} callback callback function
 * @returns {void}
 */
function insertQueue(type, uris, to, whence, play, callback) {
    switch(type) {
        case 'song':
        case 'dir':
        case 'stream':
            sendAPI("MYMPD_API_QUEUE_INSERT_URIS", {
                "uris": uris,
                "to": to,
                "whence": whence,
                "play": play
            }, callback, true);
            break;
        case 'plist':
        case 'smartpls':
        case 'webradio':
            sendAPI("MYMPD_API_QUEUE_INSERT_PLAYLISTS", {
                "plists": uris,
                "to": to,
                "whence": whence,
                "play": play
            }, callback, true);
            break;
        case 'search':
            sendAPI("MYMPD_API_QUEUE_INSERT_SEARCH", {
                "expression": uris[0],
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
 * @param {Array} uris element uris
 * @param {Function} [callback] callback function
 * @returns {void}
 */
function replaceQueue(type, uris, callback) {
    _replaceQueue(type, uris, false, callback)
}

/**
 * Replaces the queue with the element and plays it
 * @param {string} type element type: song, dir, stream, plist, smartpls, webradio, search
 * @param {Array} uris element uris
 * @param {Function} [callback] callback function
 * @returns {void}
 */
function replacePlayQueue(type, uris, callback) {
    _replaceQueue(type, uris, true, callback)
}

/**
 * Replaces the queue with the elements
 * @param {string} type element type: song, dir, stream, plist, smartpls, webradio, search
 * @param {Array} uris element uris
 * @param {boolean} play true = play added entry, false = insert only
 * @param {Function} callback callback function
 * @returns {void}
 */
function _replaceQueue(type, uris, play, callback) {
    switch(type) {
        case 'song':
        case 'stream':
        case 'dir':
            sendAPI("MYMPD_API_QUEUE_REPLACE_URIS", {
                "uris": uris,
                "play": play
            }, callback, true);
            break;
        case 'plist':
        case 'smartpls':
        case 'webradio':
            sendAPI("MYMPD_API_QUEUE_REPLACE_PLAYLISTS", {
                "plists": uris,
                "play": play
            }, callback, true);
            break;
        case 'search':
            sendAPI("MYMPD_API_QUEUE_REPLACE_SEARCH", {
                "expression": uris[0],
                "play": play
            }, callback, true);
            break;
    }
}

/**
 * Adds random songs/albums to the queue, one-shot jukebox mode.
 * @returns {void}
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
 * @returns {void}
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
 * @returns {void}
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
 * @returns {void}
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
 * Shows the set song position modal
 * @param {string} plist the playlist name or the special value "queue" to move the song
 * @param {number} oldSongPos song pos in the queue to move
 * @param {number} songId song id in the queue to move
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showSetSongPos(plist, oldSongPos, songId) {
    cleanupModalId('modalSetSongPos');
    document.getElementById('inputSongPosNew').value = '';
    document.getElementById('inputSongPosOld').value = oldSongPos;
    document.getElementById('inputSongId').value = songId;
    document.getElementById('inputSongPosPlist').value = plist;
    uiElements.modalSetSongPos.show();
}

/**
 * Sets song position in queue or playlist
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function setSongPos() {
    cleanupModalId('modalSetSongPos');
    const plist = document.getElementById('inputSongPosPlist').value;
    const oldSongPos = Number(document.getElementById('inputSongPosOld').value);
    const songId = Number(document.getElementById('inputSongId').value);
    const newSongPosEl = document.getElementById('inputSongPosNew');
    if (validateIntRangeEl(newSongPosEl, 1, 99999) === true) {
        let newSongPos = Number(newSongPosEl.value);
        if (newSongPos < oldSongPos) {
            newSongPos--;
        }
        if (plist === 'queue') {
            sendAPI("MYMPD_API_QUEUE_MOVE_ID", {
                "songIds": [songId],
                "to": newSongPos
            }, setSongPosCheckError, true);
        }
        else {
            sendAPI("MYMPD_API_PLAYLIST_CONTENT_MOVE_POSITION", {
                "plist": plist,
                "from": oldSongPos,
                "to": newSongPos
            }, setSongPosCheckError, true);
        }
    }
}

/**
 * Handles the MYMPD_API_QUEUE_MOVE_ID and MYMPD_API_PLAYLIST_CONTENT_MOVE_POSITION jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function setSongPosCheckError(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        uiElements.modalSetSongPos.hide();
    }
}

/**
 * Shows the set song priority modal
 * @param {number} songId the mpd song id
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showSetSongPriority(songId) {
    cleanupModalId('modalSetSongPriority');
    document.getElementById('inputSongPrioritySondId').value = songId;
    uiElements.modalSetSongPriority.show();
}

/**
 * Sets song priority
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function setSongPriority() {
    cleanupModalId('modalSetSongPriority');

    const songId = Number(document.getElementById('inputSongPrioritySongId').value);
    const priorityEl = document.getElementById('inputSongPriority');
    if (validateIntRangeEl(priorityEl, 0, 255) === true) {
        sendAPI("MYMPD_API_QUEUE_PRIO_SET", {
            "songIds": [songId],
            "priority": Number(priorityEl.value)
        }, setSongPriorityCheckError, true);
    }
}

/**
 * Handles the MYMPD_API_QUEUE_PRIO_SET jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
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
 * Removes a song range from the queue
 * @param {number} start start of the range (including)
 * @param {number} [end] end of the range (excluding), -1 for open end
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function removeFromQueueRange(start, end) {
    sendAPI("MYMPD_API_QUEUE_RM_RANGE", {
        "start": start,
        "end": end
    }, null, false);
}

/**
 * Removes song ids from the queue
 * @param {Array} ids MPD queue song ids
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function removeFromQueueIDs(ids) {
    sendAPI("MYMPD_API_QUEUE_RM_IDS", {
        "songIds": ids
    }, null, false);
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
        const playingRow = document.querySelector('.queue-playing');
        if (playingRow !== null) {
            playingRow.scrollIntoView(true);
        }
    }
    else {
        gotoPage(Math.floor(currentState.songPos / app.current.limit) * app.current.limit, undefined);
    }
}

/**
 * Moves a entry in the queue
 * @param {number} from from position
 * @param {number} to to position
 * @returns {void}
 */
function queueMoveSong(from, to) {
    sendAPI("MYMPD_API_QUEUE_MOVE_POSITION", {
        "from": from,
        "to": to
    }, null, false);
}

/**
 * Plays the selected song(s) after the current song.
 * Sets the priority if MPD is in random mode, else moves the song(s) after current playing song.
 * @param {Array} songIds current playing song ids
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function playAfterCurrent(songIds) {
    if (settings.partition.random === false) {
        //not in random mode - move song after current playing song
        sendAPI("MYMPD_API_QUEUE_MOVE_RELATIVE", {
            "songIds": songIds,
            "to": 0,
            "whence": 1
        }, null, false);
    }
    else {
        //in random mode - set song priority
        sendAPI("MYMPD_API_QUEUE_PRIO_SET_HIGHEST", {
            "songIds": songIds
        }, null, false);
    }
}

/**
 * Clears or crops the queue after confirmation
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function clearQueue() {
    showConfirm(tn('Do you really want to clear the queue?'), tn('Yes, clear it'), function() {
        sendAPI("MYMPD_API_QUEUE_CROP_OR_CLEAR", {}, null, false);
    });
}
