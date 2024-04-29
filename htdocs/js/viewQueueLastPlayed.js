"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
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

    const rowTitle = settingsWebuiFields.clickSong.validValues[settings.webuiSettings.clickSong];
    if (settings['view' + app.id].mode === 'table') {
        const tfoot = table.querySelector('tfoot');
        elClear(tfoot);
        updateTable(obj, app.id, function(row, data) {
            setData(row, 'uri', data.uri);
            setData(row, 'name', data.Title);
            setData(row, 'type', 'song');
            row.setAttribute('title', tn(rowTitle));
        });
        addTblFooter(tfoot,
            elCreateTextTnNr('span', {}, 'Num songs', obj.result.totalEntities)
        );
        return;
    }
    updateGrid(obj, app.id, function(card, data) {
        card.setAttribute('title', tn(rowTitle));
        setData(card, 'uri', data.uri);
        setData(card, 'name', data.Title);
        setData(card, 'type', 'song');
    });
}
