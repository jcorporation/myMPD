"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module selectActions_js */

/**
 * Initializes the select action dropdowns
 * @returns {void}
 */
function initSelectActions() {
    for (const dropdownId of [
        'dropdownQueueCurrentSelection',
        'dropdownQueueLastPlayedSelection',
        'dropdownQueueJukeboxSelection',
        'dropdownBrowseDatabaseAlbumDetailSelection',
        'dropdownBrowsePlaylistDetailSelection',
        'dropdownBrowseRadioWebradiodbSelection',
        'dropdownBrowseRadioRadiobrowserSelection',
        'dropdownSearchSelection'
    ]) {
        const el = document.querySelector('#' + dropdownId + '> div');
        document.getElementById(dropdownId).parentNode.addEventListener('show.bs.dropdown', function() {
            addSelectActionButtons(el, dropdownId);
            showSelectionCount();
        }, false);
        el.addEventListener('click', function(event) {
            if (event.target.nodeName === 'BUTTON') {
                parseCmd(event, getData(event.target, 'href'));
            }
        }, false);
    }
}

/**
 * Adds the select action buttons
 * @param {HTMLElement} el element to append the buttons
 * @param {string} dropdownId dropdown id to append the buttons
 * @returns {void}
 */
function addSelectActionButtons(el, dropdownId) {
    elClear(el);
    const table = document.getElementById(app.id + 'List');
    const type = dropdownId === 'dropdownBrowseRadioWebradiodbSelection' || dropdownId === 'dropdownBrowseRadioRadiobrowserSelection'
            ? 'stream'
            : 'song';

    if (dropdownId === 'dropdownQueueLastPlayedSelection' ||
        dropdownId === 'dropdownQueueJukeboxSelection' ||
        dropdownId === 'dropdownBrowseDatabaseAlbumDetailSelection' ||
        dropdownId === 'dropdownBrowsePlaylistDetailSelection' ||
        dropdownId === 'dropdownBrowseRadioWebradiodbSelection' ||
        dropdownId === 'dropdownBrowseRadioRadiobrowserSelection' ||
        dropdownId === 'dropdownSearchSelection')
    {
        addSelectActionButton(el, {"cmd": "execSelectAction", "options": [type, "appendQueue"]}, 'Append to queue');
        addSelectActionButton(el, {"cmd": "execSelectAction", "options": [type, "appendPlayQueue"]}, 'Append to queue and play');
        if (features.featWhence === true) {
            addSelectActionButton(el, {"cmd": "execSelectAction", "options": [type, "insertAfterCurrentQueue"]}, 'Insert after current playing song');
        }
        addSelectActionButton(el, {"cmd": "execSelectAction", "options": [type, "replaceQueue"]}, 'Replace queue');
        addSelectActionButton(el, {"cmd": "execSelectAction", "options": [type, "replacePlayQueue"]}, 'Replace queue and play');
    }
    if (dropdownId === 'dropdownQueueCurrentSelection') {
        addSelectActionButton(el, {"cmd": "execSelectAction", "options": [type, "playAfterCurrent"]}, 'Play after current playing song');
        addSelectActionButton(el, {"cmd": "execSelectAction", "options": [type, "removeFromQueueIDs"]}, 'Remove');
    }
    if (dropdownId === 'dropdownQueueJukeboxSelection') {
        addSelectActionButton(el, {"cmd": "execSelectAction", "options": [type, "delQueueJukeboxSong"]}, 'Remove');
    }
    if (dropdownId === 'dropdownBrowsePlaylistDetailSelection') {
        const ro = getData(table, 'ro');
        if (ro === false) {
            addSelectActionButton(el, {"cmd": "execSelectAction", "options": [type, "removeFromPlaylistPositions"]}, 'Remove');
        }
    }
    if (features.featPlaylists === true &&
        dropdownId !== 'dropdownBrowsePlaylistListSelection')
    {
        addSelectActionButton(el, {"cmd": "execSelectAction", "options": [type, "showAddToPlaylist"]}, 'Add to playlist');
    }
}

/**
 * Adds a button to the select action dropdown
 * @param {HTMLElement} el element to append the buttons
 * @param {object} cmd button cmd
 * @param {string} text button text
 * @returns {void}
 */
function addSelectActionButton(el, cmd, text) {
    const btn = elCreateTextTn('button', {"class": ["btn", "btn-sm", "btn-secondary"]}, text);
    setData(btn, 'href', cmd);
    el.appendChild(btn);
}

/**
 * Handles the selection actions
 * @param {string} type entity type
 * @param {string} action action to handle
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function execSelectAction(type, action) {
    const table = document.getElementById(app.id + 'List');
    switch(action) {
        case 'appendQueue': {
            const uris = getSelectedRowData(table, 'uri');
            appendQueue(type, uris);
            break;
        }
        case 'appendPlayQueue': {
            const uris = getSelectedRowData(table, 'uri');
            appendPlayQueue(type, uris);
            break;
        }
        case 'playAfterCurrent': {
            const songIds = getSelectedRowData(table, 'songid');
            playAfterCurrent(songIds);
            break;
        }
        case 'insertAfterCurrentQueue': {
            const uris = getSelectedRowData(table, 'uri');
            insertAfterCurrentQueue(type, uris);
            break;
        }
        case 'replaceQueue': {
            const uris = getSelectedRowData(table, 'uri');
            replaceQueue(type, uris);
            break;
        }
        case 'replacePlayQueue': {
            const uris = getSelectedRowData(table, 'uri');
            replacePlayQueue(type, uris);
            break;
        }
        case 'removeFromQueueIDs': {
            const songIds = getSelectedRowData(table, 'songid');
            removeFromQueueIDs(songIds);
            break;
        }
        case 'showAddToPlaylist': {
            const uris = getSelectedRowData(table, 'uri');
            showAddToPlaylist(uris, '');
            break;
        }
        case 'removeFromPlaylistPositions': {
            const pos = getSelectedRowData(table, 'pos');
            const plist = getData(table, 'uri');
            removeFromPlaylistPositions(plist, pos);
            break;
        }
    }
}
