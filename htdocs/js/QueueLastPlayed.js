"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/**
 * QueueLastPlayed handler
 */
function handleQueueLastPlayed() {
    setFocusId('searchQueueLastPlayedStr');
    sendAPI("MYMPD_API_LAST_PLAYED_LIST", {
        "offset": app.current.offset,
        "limit": app.current.limit,
        "cols": settings.colsQueueLastPlayedFetch,
        "searchstr": app.current.search
    }, parseLastPlayed, true);
    const searchQueueLastPlayedStrEl = document.getElementById('searchQueueLastPlayedStr');
    if (searchQueueLastPlayedStrEl.value === '' &&
        app.current.search !== '')
    {
        searchQueueLastPlayedStrEl.value = app.current.search;
    }
}

/**
 * Initialization function for last played elements
 */
function initQueueLastPlayed() {
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

    document.getElementById('QueueLastPlayedList').addEventListener('click', function(event) {
        if (event.target.nodeName === 'TD') {
            clickSong(getData(event.target.parentNode, 'uri'));
        }
        else if (event.target.nodeName === 'A') {
            //action td
            handleActionTdClick(event);
        }
    }, false);
}

/**
 * Handler for the MYMPD_API_LAST_PLAYED_LIST jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
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
