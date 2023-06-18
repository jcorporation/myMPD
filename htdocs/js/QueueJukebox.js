"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module QueueJukebox_js */

/**
 * QueueJukebox handler
 * @returns {void}
 */
function handleQueueJukebox() {
    setFocusId('searchQueueJukeboxStr');
    sendAPI("MYMPD_API_JUKEBOX_LIST", {
        "offset": app.current.offset,
        "limit": app.current.limit,
        "cols": settings.colsQueueJukeboxFetch,
        "searchstr": app.current.search
    }, parseJukeboxList, true);
    const searchQueueJukeboxStrEl = document.getElementById('searchQueueJukeboxStr');
    if (searchQueueJukeboxStrEl.value === '' &&
        app.current.search !== '')
    {
        searchQueueJukeboxStrEl.value = app.current.search;
    }
}

/**
 * Initializes the jukebox related elements
 * @returns {void}
 */
function initQueueJukebox() {
    document.getElementById('QueueJukeboxList').addEventListener('click', function(event) {
        //select mode
        if (selectRow(event) === true) {
            return;
        }
        //action td
        if (event.target.nodeName === 'A') {
            handleActionTdClick(event);
            return;
        }
        const target = event.target.closest('TR');
        if (checkTargetClick(target) === true) {
            if (settings.partition.jukeboxMode === 'song') {
                clickSong(getData(target, 'uri'), event);
            }
            else if (settings.partition.jukeboxMode === 'album') {
                clickAlbumPlay(getData(target, 'AlbumArtist'), getData(target, 'Album'));
            }
        }
    }, false);
    document.getElementById('searchQueueJukeboxStr').addEventListener('keyup', function(event) {
        if (ignoreKeys(event) === true) {
            return;
        }
        clearSearchTimer();
        const value = this.value;
        searchTimer = setTimeout(function() {
            appGoto(app.current.card, app.current.tab, app.current.view,
                0, app.current.limit, app.current.filter, app.current.sort, '-', value);
        }, searchTimerTimeout);
    }, false);
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

/**
 * Parses the response from MYMPD_API_JUKEBOX_LIST
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parseJukeboxList(obj) {
    if (checkResultId(obj, 'QueueJukeboxList') === false) {
        if (obj.result !== undefined) {
            if (obj.result.jukeboxMode === 'off') {
                elHideId('QueueJukeboxList');
                elShowId('QueueJukeboxDisabled');
            }
            else {
                elHideId('QueueJukeboxDisabled');
            }
        }
        return;
    }

    elHideId('QueueJukeboxDisabled');
    elShowId('QueueJukeboxList');

    const rowTitle = settings.partition.jukeboxMode === 'song' ?
        webuiSettingsDefault.clickSong.validValues[settings.webuiSettings.clickSong] :
        webuiSettingsDefault.clickQuickPlay.validValues[settings.webuiSettings.clickQuickPlay];
    updateTable(obj, 'QueueJukebox', function(row, data) {
        setData(row, 'uri', data.uri);
        setData(row, 'name', data.Title);
        setData(row, 'type', data.uri === 'Album' ? 'album' : 'song');
        setData(row, 'pos', data.Pos);
        row.setAttribute('title', tn(rowTitle));
        row.setAttribute('tabindex', 0);
    });
}
