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
}

/**
 * Click event handler for last played
 * @param {MouseEvent} event click event
 * @returns {void}
 */
function viewQueueLastPlayedListClickHandler(event) {
    const target = tableClickHandler(event);
    if (target !== null) {
        clickSong(getData(target, 'uri'), event);
    }
}

/**
 * Handler for the MYMPD_API_LAST_PLAYED_LIST jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parseLastPlayed(obj) {
    if (checkResultId(obj, 'QueueLastPlayedList', undefined) === false) {
        return;
    }

    const rowTitle = settingsWebuiFields.clickSong.validValues[settings.webuiSettings.clickSong];
    updateTable(obj, 'QueueLastPlayed', function(row, data) {
        setData(row, 'uri', data.uri);
        setData(row, 'name', data.Title);
        setData(row, 'type', 'song');
        row.setAttribute('title', tn(rowTitle));
    });
}
