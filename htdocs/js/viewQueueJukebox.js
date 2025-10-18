"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module viewQueueJukebox_js */

/**
 * QueueJukeboxSong handler
 * @returns {void}
 */
function handleQueueJukeboxSong() {
    handleQueueJukebox('QueueJukeboxSong');
}

/**
 * QueueJukeboxAlbum handler
 * @returns {void}
 */
function handleQueueJukeboxAlbum() {
    handleQueueJukebox('QueueJukeboxAlbum');
}

/**
 * QueueJukeboxAlbum handler
 * @param {string} view jukebox view to display (song or album)
 * @returns {void}
 */
function handleQueueJukebox(view) {
    handleSearchExpression(view);
    getJukeboxList(view);
}

/**
 * Initializes the jukebox related elements
 * @param {string} view jukebox view to display (song or album)
 * @returns {void}
 */
function initViewQueueJukebox(view) {
    initSearchExpression(view);

    setView('QueueJukeboxAlbum');
    setView('QueueJukeboxSong');
}

/**
 * Click event handler for jukebox list
 * @param {MouseEvent} event click event
 * @param {HTMLElement} target calculated target
 * @returns {void}
 */
function viewQueueJukeboxListClickHandler(event, target) {
    if (settings.partition.jukeboxMode === 'song') {
        clickSong(getData(target, 'uri'), event);
    }
    else if (settings.partition.jukeboxMode === 'album') {
        clickQuickPlay(target);
    }
}

/**
 * Gets and parses the jukebox list
 * @param {string} view jukebox view to display (song or album)
 * @returns {void}
 */
function getJukeboxList(view) {
    if (settings.partition.jukeboxMode === 'off') {
        elHideId(view + 'List');
        elShowId(view + 'Disabled');
    }
    else {
        elShowId(view + 'List');
        elHideId(view + 'Disabled');
    }
    sendAPI("MYMPD_API_JUKEBOX_LIST", {
        "offset": app.current.offset,
        "limit": app.current.limit,
        "fields": settings['view' + view + 'Fetch'].fields,
        "expression": app.current.search
    }, parseJukeboxList, true);
}

/**
 * Parses the response from MYMPD_API_JUKEBOX_LIST
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parseJukeboxList(obj) {
    const view = settings.partition.jukeboxMode === 'album'
        ? 'QueueJukeboxAlbum'
        : 'QueueJukeboxSong';
    const table = elGetById(view + 'List');
    if (checkResult(obj, table, undefined) === false) {
        return;
    }

    if (settings['view' + app.id].mode === 'table') {
        const tfoot = table.querySelector('tfoot');
        elClear(tfoot);
        updateTable(obj, view, function(row, data) {
            parseJukeboxListUpdate(row, data);
        });
        if (obj.result.totalEntities > 0) {
            addTblFooter(tfoot,
                elCreateTextTnNr('span', {}, 'Num entries', obj.result.totalEntities)
            );
        }
        return;
    }
    if (settings['view' + app.id].mode === 'grid') {
        updateGrid(obj, app.id, function(card, data) {
            parseJukeboxListUpdate(card, data);
        });
        return;
    }
    updateList(obj, app.id, function(card, data) {
        parseJukeboxListUpdate(card, data);
    });
}

/**
 * Callback function for row or card
 * @param {HTMLElement} card Row or card
 * @param {object} data Data object
 * @returns {void}
 */
function parseJukeboxListUpdate(card, data) {
    const rowTitle = settings.partition.jukeboxMode === 'song' ?
        settingsWebuiFields.clickSong.validValues[settings.webuiSettings.clickSong] :
        settingsWebuiFields.clickQuickPlay.validValues[settings.webuiSettings.clickQuickPlay];
    setData(card, 'uri', data.uri);
    setData(card, 'name', data.Title);
    setData(card, 'type', data.Type);
    setData(card, 'pos', data.Pos);
    card.setAttribute('title', tn(rowTitle));
}

/**
 * Goto handler that respects the jukebox mode
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function gotoJukebox() {
    const view = settings.partition.jukeboxMode === 'album'
        ? 'Album'
        : 'Song';
    appGoto('Queue', 'Jukebox', view);
}

/**
 * Clears the jukebox queue
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function clearJukeboxQueue() {
    sendAPI("MYMPD_API_JUKEBOX_CLEAR", {}, null, false);
}

/**
 * Refills the jukebox queue
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function refillJukeboxQueue() {
    sendAPI("MYMPD_API_JUKEBOX_REFILL", {}, null, false);
}

/**
 * Removes a song / album from the jukebox queue
 * @param {Array} pos position
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function delQueueJukeboxEntries(pos) {
    sendAPI("MYMPD_API_JUKEBOX_RM", {
        "positions": pos
    }, null, false);
}
