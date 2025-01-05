"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module viewQueueLastPlayed_js */

/**
 * QueueLastPlayed handler
 * @returns {void}
 */
function handleQueueLastPlayed() {
    handleSearchExpression('QueueLastPlayed');
    sendAPI("MYMPD_API_LAST_PLAYED_LIST", {
        "offset": app.current.offset,
        "limit": app.current.limit,
        "fields": settings.viewQueueLastPlayedFetch.fields,
        "expression": app.current.search
    }, parseLastPlayed, true);
}

/**
 * Initialization function for last played elements
 * @returns {void}
 */
function initViewQueueLastPlayed() {
    initSearchExpression('QueueLastPlayed');

    setView('QueueLastPlayed');
}

/**
 * Click event handler for last played
 * @param {MouseEvent} event click event
 * @param {HTMLElement} target calculated target
 * @returns {void}
 */
function viewQueueLastPlayedListClickHandler(event, target) {
    clickSong(getData(target, 'uri'), event);
}

/**
 * Handler for the MYMPD_API_LAST_PLAYED_LIST jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parseLastPlayed(obj) {
    const table = elGetById('QueueLastPlayedList');
    if (checkResult(obj, table, undefined) === false) {
        return;
    }

    if (settings['view' + app.id].mode === 'table') {
        const tfoot = table.querySelector('tfoot');
        elClear(tfoot);
        updateTable(obj, app.id, function(row, data) {
            parseLastPlayedUpdate(row, data);
        });
        addTblFooter(tfoot,
            elCreateTextTnNr('span', {}, 'Num songs', obj.result.totalEntities)
        );
        return;
    }
    if (settings['view' + app.id].mode === 'grid') {
        updateGrid(obj, app.id, function(card, data) {
            parseLastPlayedUpdate(card, data);
        });
        return;
    }
    updateList(obj, app.id, function(card, data) {
        parseLastPlayedUpdate(card, data);
    });
}

/**
 * Callback function for row or card
 * @param {HTMLElement} card Row or card
 * @param {object} data Data object
 * @returns {void}
 */
function parseLastPlayedUpdate(card, data) {
    const rowTitle = settingsWebuiFields.clickSong.validValues[settings.webuiSettings.clickSong];
    card.setAttribute('title', tn(rowTitle));
    setData(card, 'uri', data.uri);
    setData(card, 'name', data.Title);
    setData(card, 'type', 'song');
}
