"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module viewQueueCurrent_js */

/**
 * Current queue handler
 * @returns {void}
 */
function handleQueueCurrent() {
    handleSearchExpression('QueueCurrent');
    toggleBtnChkId('QueueCurrentSortDesc', app.current.sort.desc);
    selectTag('QueueCurrentSortTagsList', undefined, app.current.sort.tag);
    const searchMatchEl = elGetById(app.id + 'SearchMatch');

    sendAPI("MYMPD_API_QUEUE_SEARCH", {
        "offset": app.current.offset,
        "limit": app.current.limit,
        "sort": app.current.sort.tag,
        "sortdesc": app.current.sort.desc,
        "expression": app.current.search,
        "fields": settings.viewQueueCurrentFetch.fields
    }, parseQueue, true);

    if (app.current.filter === 'prio') {
        elShowId('QueueCurrentSearchPriorityMatch');
        searchMatchEl.value = '>=';
    }
    else {
        if (getSelectValue(searchMatchEl) === '>=') {
            searchMatchEl.value = 'contains';
        }
        elHideId('QueueCurrentSearchPriorityMatch');
    }
}

/**
 * Initializes the current queue elements
 * @returns {void}
 */
function initViewQueueCurrent() {
    initSortBtns('QueueCurrent');
    initSearchExpression('QueueCurrent');

    setView('QueueCurrent');
}

/**
 * Click event handler for current queue list
 * @param {MouseEvent} event click event
 * @param {HTMLElement} target calculated target
 * @returns {void}
 */
function viewQueueCurrentListClickHandler(event, target) {
    clickQueueSong(getData(target, 'songid'), getData(target, 'uri'), event);
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
        elEnableId('QueueCurrentGotoPlayingSongBtn');
    }
    else {
        elDisableId('QueueCurrentGotoPlayingSongBtn');
    }

    const table = elGetById('QueueCurrentList');
    if (checkResult(obj, table, undefined) === false) {
        return;
    }

    if (obj.result.offset < app.current.offset) {
        gotoPage(obj.result.offset, undefined);
        return;
    }

    if (settings['view' + app.id].mode === 'table') {
        const colspan = settings['viewQueueCurrent'].fields.length;
        const smallWidth = uiSmallWidthTagRows();
        const tfoot = table.querySelector('tfoot');
        elClear(tfoot);
        const actionTd = elCreateEmpty('td', {"data-col": "Action"});
        addActionLinks(actionTd);

        updateTable(obj, app.id, function(row, data) {
            parseQueueUpdate(row, data);
        }, function(row, data) {
            tableRow(row, data, app.id, colspan, smallWidth, actionTd);
            if (currentState.currentSongId === data.id) {
                setPlayingRow(row);
                if (currentState.state === 'play') {
                    setQueueCounter(row, getCounterText());
                }
            }
        });

        if (obj.result.totalEntities > 0) {
            const totalTime = obj.result.totalTime > 0
                ? elCreateText('span', {}, smallSpace + nDash + smallSpace + fmtDuration(obj.result.totalTime))
                : elCreateEmpty('span', {});
            addTblFooter(tfoot,
                elCreateNodes('small', {}, [
                    elCreateTextTnNr('span', {}, 'Num songs', obj.result.totalEntities),
                    totalTime
                ])
            );
        }
        return;
    }
    if (settings['view' + app.id].mode === 'grid') {
        updateGrid(obj, app.id, function(card, data) {
            parseQueueUpdate(card, data);
        }, function(card, data) {
            createGridBody(card, data, app.id);
            parseQueueBody(card, data);
        });
        return;
    }
    updateList(obj, app.id, function(card, data) {
        parseQueueUpdate(card, data);
    }, function(card, data) {
        createListBody(card, data, app.id);
        parseQueueBody(card, data);
    });
}

/**
 * Callback function for row or card body
 * @param {HTMLElement} card Row or card
 * @param {object} data Data object
 * @returns {void}
 */
function parseQueueBody(card, data) {
    if (currentState.currentSongId === data.id) {
        setPlayingRow();
        if (currentState.state === 'play') {
            setQueueCounter(card, getCounterText());
        }
    }
}

/**
 * Callback function for row or card
 * @param {HTMLElement} card Row or card
 * @param {object} data Data object
 * @returns {void}
 */
function parseQueueUpdate(card, data) {
    const rowTitle = settingsWebuiFields.clickQueueSong.validValues[settings.webuiSettings.clickQueueSong];
    card.setAttribute('id', 'queueSongId' + data.id);
    card.setAttribute('title', tn(rowTitle));
    setData(card, 'songid', data.id);
    setData(card, 'pos', data.Pos);
    setData(card, 'duration', data.Duration);
    setData(card, 'uri', data.uri);
    setData(card, 'type', data.Type);
    if (data.Type === 'webradio') {
        setData(card, 'webradioType', data.webradio.Type);
        setData(card, 'name', data.webradio.Name);
    }
    else {
        setData(card, 'name', data.Title);
        //set AlbumId
        if (data.AlbumId !== undefined) {
            setData(card, 'AlbumId', data.AlbumId);
        }
        //and browse tags
        for (const tag of settings.tagListBrowse) {
            if (albumFilters.includes(tag) &&
                isEmptyTag(data[tag]) === false)
            {
                setData(card, tag, data[tag]);
            }
        }
    }
    //set Title to Name + Title for streams
    data.Title = getDisplayTitle(data.Name, data.Title);

    if (features.featAdvqueue === false ||
        app.current.sort.tag === 'Priority')
    {
        // Enable drag and drop only if list is sorted by priority
        // This is the default sorting for simple queue mode
        card.setAttribute('draggable', 'true');
        card.setAttribute('tabindex', '0');
    }
}

/**
 * Removes the old playing row and sets the new playing row in the queue view
 * @returns {void}
 */
function queueSetCurrentSong() {
    //remove old playing row
    const old = elGetById('queueSongId' + currentState.lastSongId);
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
        playingRow = elGetById('queueSongId' + currentState.currentSongId);
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
    if (playingRow === undefined ||
        playingRow.getAttribute('id') === null)
    {
        playingRow = elGetById('queueSongId' + currentState.currentSongId);
    }
    const posTd = playingRow.querySelector('[data-col=Pos]');
    if (posTd) {
        posTd.classList.remove('mi');
        posTd.textContent = getData(playingRow, 'pos') + 1;
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
        const progressPrct = currentState.state === 'stop' || currentState.totalTime === 0
            ? 100
            : Math.ceil((100 / currentState.totalTime) * currentState.elapsedTime * 100) / 100;
        let targetRow = playingRow;
        if (targetRow.getAttribute('id') === null) {
            targetRow = elGetById('queueSongId' + currentState.currentSongId);
        }
        if (targetRow !== null) {
            targetRow.style.background = 'linear-gradient(90deg, var(--mympd-highlightcolor) 0%, var(--mympd-highlightcolor) ' +
                progressPrct + '%, transparent ' + progressPrct + '%, transparent 100%)';
        }
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
    if (playingRow === undefined ||
        playingRow.getAttribute('id') === null)
    {
        playingRow = elGetById('queueSongId' + currentState.currentSongId);
    }
    if (playingRow !== null) {
        const posTd = playingRow.querySelector('[data-col=Pos]');
        if (posTd !== null) {
            posTd.classList.add('mi');
            posTd.textContent = currentState.state === 'play'
                ? 'play_arrow'
                : currentState.state === 'pause'
                    ? 'pause'
                    : 'stop';
        }
        playingRow.classList.add('queue-playing');
    }
}

/**
 * Scrolls to the current song
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function gotoPlayingSong() {
    if (currentState.songPos === -1) {
        elDisableId('QueueCurrentGotoPlayingSongBtn');
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
