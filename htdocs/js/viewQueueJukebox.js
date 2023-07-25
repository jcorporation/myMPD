"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module QueueJukebox_js */

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
    setFocusId('search' + view + 'Str');
    getJukeboxList(view);
    const searchQueueJukeboxStrEl = document.getElementById('search' + view + 'Str');
    if (searchQueueJukeboxStrEl.value === '' &&
        app.current.search !== '')
    {
        searchQueueJukeboxStrEl.value = app.current.search;
    }
}

/**
 * Initializes the jukebox related elements
 * @param {string} view jukebox view to display (song or album)
 * @returns {void}
 */
function initQueueJukebox(view) {
    document.getElementById(view + 'List').addEventListener('click', function(event) {
        //select mode
        if (selectRow(event) === true) {
            return;
        }
        //action td
        if (event.target.nodeName === 'A') {
            handleActionTdClick(event);
            return;
        }
        //table body
        const target = event.target.closest('TR');
        if (target === null) {
            return;
        }
        if (target.parentNode.nodeName === 'TBODY' &&
            checkTargetClick(target) === true)
        {
            if (settings.partition.jukeboxMode === 'song') {
                clickSong(getData(target, 'uri'), event);
            }
            else if (settings.partition.jukeboxMode === 'album') {
                clickQuickPlay(target);
            }
        }
    }, false);
    document.getElementById('search' + view + 'Str').addEventListener('keyup', function(event) {
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
 * Gets and parses the jukebox list
 * @param {string} view jukebox view to display (song or album)
 * @returns {void}
 */
function getJukeboxList(view) {
    sendAPI("MYMPD_API_JUKEBOX_LIST", {
        "offset": app.current.offset,
        "limit": app.current.limit,
        "cols": settings['cols' + view + 'Fetch'],
        "searchstr": app.current.search
    }, parseJukeboxList, true);
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
    const view = settings.partition.jukeboxMode === 'album'
        ? 'QueueJukeboxAlbum'
        : 'QueueJukeboxSong';
    if (checkResultId(obj, view + 'List') === false) {
        if (obj.result !== undefined) {
            if (obj.result.jukeboxMode === 'off') {
                elHideId(view + 'List');
                elShowId(view + 'Disabled');
            }
            else {
                elShowId(view + 'List');
                elHideId(view + 'Disabled');
            }
        }
        return;
    }

    elHideId(view + 'Disabled');
    elShowId(view + 'List');

    const rowTitle = settings.partition.jukeboxMode === 'song' ?
        webuiSettingsDefault.clickSong.validValues[settings.webuiSettings.clickSong] :
        webuiSettingsDefault.clickQuickPlay.validValues[settings.webuiSettings.clickQuickPlay];
    updateTable(obj, view, function(row, data) {
        setData(row, 'uri', data.uri);
        setData(row, 'name', data.Title);
        setData(row, 'type', data.Type);
        setData(row, 'pos', data.Pos);
        row.setAttribute('title', tn(rowTitle));
        row.setAttribute('tabindex', 0);
    });
}
