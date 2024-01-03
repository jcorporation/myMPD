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
    const searchMatchEl = elGetById(app.id + 'SearchMatch');

    if (app.current.sort.tag === '' ||
        app.current.sort.tag === 'Pos')
    {
        app.current.sort.tag = 'Priority';
    }

    sendAPI("MYMPD_API_QUEUE_SEARCH", {
        "offset": app.current.offset,
        "limit": app.current.limit,
        "sort": app.current.sort.tag,
        "sortdesc": app.current.sort.desc,
        "expression": app.current.search,
        "cols": settings.colsQueueCurrentFetch
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
    elGetById('QueueCurrentList').addEventListener('click', function(event) {
        const target = tableClickHandler(event);
        if (target !== null) {
            clickQueueSong(getData(target, 'songid'), getData(target, 'uri'), event);
        }
    }, false);

    initSearchExpression('QueueCurrent');
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
    if (checkResultId(obj, 'QueueCurrentList') === false) {
        return;
    }

    if (obj.result.offset < app.current.offset) {
        gotoPage(obj.result.offset, undefined);
        return;
    }

    const colspan = settings['colsQueueCurrent'].length;
    const smallWidth = uiSmallWidthTagRows();

    const rowTitle = settingsWebuiFields.clickQueueSong.validValues[settings.webuiSettings.clickQueueSong];
    updateTable(obj, 'QueueCurrent', function(row, data) {
        if (features.featAdvqueue === false ||
            app.current.sort.tag === 'Priority')
        {
            row.setAttribute('draggable', 'true');
        }
        row.setAttribute('id', 'queueSongId' + data.id);
        row.setAttribute('title', tn(rowTitle));
        setData(row, 'songid', data.id);
        setData(row, 'pos', data.Pos);
        setData(row, 'duration', data.Duration);
        setData(row, 'uri', data.uri);
        setData(row, 'type', data.Type);
        if (data.Type === 'webradio') {
            setData(row, 'webradioUri', data.webradio.filename);
            setData(row, 'name', data.webradio.Name);
        }
        else {
            setData(row, 'name', data.Title);
            //set AlbumId
            if (data.AlbumId !== undefined) {
                setData(row, 'AlbumId', data.AlbumId);
            }
            //and browse tags
            for (const tag of settings.tagListBrowse) {
                if (albumFilters.includes(tag) &&
                    isEmptyTag(data[tag]) === false)
                {
                    setData(row, tag, data[tag]);
                }
            }
        }
        //set Title to Name + Title for streams
        data.Title = getDisplayTitle(data.Name, data.Title);
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
    if (obj.result.totalEntities > 0) {
        const totalTime = obj.result.totalTime > 0
            ? elCreateText('span', {}, smallSpace + nDash + smallSpace + fmtDuration(obj.result.totalTime))
            : elCreateEmpty('span', {});
        elReplaceChild(tfoot,
            elCreateNode('tr', {"class": ["not-clickable"]},
                elCreateNode('td', {"colspan": (colspan + 1)},
                    elCreateNodes('small', {}, [
                        elCreateTextTnNr('span', {}, 'Num songs', obj.result.totalEntities),
                        totalTime
                    ])
                )
            )
        );
    }
    else {
        elClear(tfoot);
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
    if (playingRow === undefined) {
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
