"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
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
    document.getElementById(view + 'List').addEventListener('click', function(event) {
        const target = tableClickHandler(event);
        if (target !== null) {
            if (settings.partition.jukeboxMode === 'song') {
                clickSong(getData(target, 'uri'), event);
            }
            else if (settings.partition.jukeboxMode === 'album') {
                clickQuickPlay(target);
            }
        }
    }, false);

    initSearchExpression(view);
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
        "expression": app.current.search
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

    elShowId(view + 'List');
    elHideId(view + 'Disabled');

    const rowTitle = settings.partition.jukeboxMode === 'song' ?
        settingsWebuiFields.clickSong.validValues[settings.webuiSettings.clickSong] :
        settingsWebuiFields.clickQuickPlay.validValues[settings.webuiSettings.clickQuickPlay];
    updateTable(obj, view, function(row, data) {
        setData(row, 'uri', data.uri);
        setData(row, 'name', data.Title);
        setData(row, 'type', data.Type);
        setData(row, 'pos', data.Pos);
        row.setAttribute('title', tn(rowTitle));
        row.setAttribute('tabindex', 0);
    });
}
