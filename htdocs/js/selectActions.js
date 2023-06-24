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
        'dropdownBrowseFilesystemSelection',
        'dropdownBrowsePlaylistListSelection',
        'dropdownBrowsePlaylistDetailSelection',
        'dropdownBrowseRadioWebradiodbSelection',
        'dropdownBrowseRadioRadiobrowserSelection',
        'dropdownSearchSelection',
        'dropdownBrowseDatabaseAlbumListSelection'
    ]) {
        const el = document.querySelector('#' + dropdownId + '> div');
        if (dropdownId === 'dropdownBrowseDatabaseAlbumListSelection' ||
            dropdownId === 'dropdownBrowseRadioFavoritesListSelection')
        {
            document.getElementById(dropdownId).parentNode.addEventListener('show.bs.dropdown', function() {
                addSelectActionButtons(el, dropdownId);
                showGridSelectionCount();
            }, false);
        }
        else {
            document.getElementById(dropdownId).parentNode.addEventListener('show.bs.dropdown', function() {
                addSelectActionButtons(el, dropdownId);
                showTableSelectionCount();
            }, false);
        }
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
    const firstSelection = table.querySelector('.active');
    const type = firstSelection !== null
        ? getData(firstSelection, 'type')
        : 'song';

    if (dropdownId !== 'dropdownQueueCurrentSelection') {
        addSelectActionButton(el, {"cmd": "execSelectAction", "options": [type, "appendQueue"]}, 'Append to queue');
        addSelectActionButton(el, {"cmd": "execSelectAction", "options": [type, "appendPlayQueue"]}, 'Append to queue and play');
        if (features.featWhence === true) {
            addSelectActionButton(el, {"cmd": "execSelectAction", "options": [type, "insertAfterCurrentQueue"]}, 'Insert after current playing song');
        }
        addSelectActionButton(el, {"cmd": "execSelectAction", "options": [type, "replaceQueue"]}, 'Replace queue');
        addSelectActionButton(el, {"cmd": "execSelectAction", "options": [type, "replacePlayQueue"]}, 'Replace queue and play');
    }
    else {
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
    if (dropdownId === 'dropdownBrowsePlaylistListSelection') {
        addDivider(el);
        addSelectActionButton(el, {"cmd": "execSelectAction", "options": [type, "showDelPlaylist"]}, 'Delete');
        addSelectActionButton(el, {"cmd": "execSelectAction", "options": [type, "showCopyPlaylist"]}, 'Copy');
    }
    if (features.featPlaylists === true &&
        type !== 'plist' &&
        type !== 'smartpls' &&
        dropdownId !== 'dropdownBrowsePlaylistListSelection')
    {
        addSelectActionButton(el, {"cmd": "execSelectAction", "options": [type, "showAddToPlaylist"]}, 'Add to playlist');
    }
    if (dropdownId === 'dropdownBrowsePlaylistDetailSelection' &&
        getData(table, 'type') !== 'smartpls')
    {
        addSelectActionButton(el, {"cmd": "execSelectAction", "options": [type, "showMoveToPlaylist"]}, 'Move to playlist');
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
 * Returns an array of all selected rows/grids attribute
 * @param {HTMLElement} parent table or grid element
 * @param {string} attribute attribute name
 * @returns {Array} list of attribute values of selected rows
 */
function getSelectionData(parent, attribute) {
    const data = [];
    const rows = parent.querySelectorAll('.active');
    for (const row of rows) {
        data.push(getData(row, attribute));
    }
    return data;
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
    const attribute = type === 'album'
        ? 'AlbumId'
        : action === 'playAfterCurrent' || action === 'removeFromQueueIDs'
            ? 'songid' 
            : action === 'showMoveToPlaylist' || action === 'removeFromPlaylistPositions'
                ? 'songpos'
                : 'uri';
    switch(action) {
        case 'appendQueue': {
            const uris = getSelectionData(table, attribute);
            appendQueue(type, uris);
            break;
        }
        case 'appendPlayQueue': {
            const uris = getSelectionData(table, attribute);
            appendPlayQueue(type, uris);
            break;
        }
        case 'playAfterCurrent': {
            const songIds = getSelectionData(table, attribute);
            playAfterCurrent(songIds);
            break;
        }
        case 'insertAfterCurrentQueue': {
            const uris = getSelectionData(table, attribute);
            insertAfterCurrentQueue(type, uris);
            break;
        }
        case 'replaceQueue': {
            const uris = getSelectionData(table, attribute);
            replaceQueue(type, uris);
            break;
        }
        case 'replacePlayQueue': {
            const uris = getSelectionData(table, attribute);
            replacePlayQueue(type, uris);
            break;
        }
        case 'removeFromQueueIDs': {
            const songIds = getSelectionData(table, attribute);
            removeFromQueueIDs(songIds);
            break;
        }
        case 'showAddToPlaylist': {
            const uris = getSelectionData(table, attribute);
            showAddToPlaylist(uris, '');
            break;
        }
        case 'showMoveToPlaylist': {
            const positions = getSelectionData(table, attribute);
            const plist = getData(table, 'uri');
            showMoveToPlaylist(plist, positions);
            break;
        }
        case 'removeFromPlaylistPositions': {
            const positions = getSelectionData(table, attribute);
            const plist = getData(table, 'uri');
            removeFromPlaylistPositions(plist, positions);
            break;
        }
        case 'showDelPlaylist': {
            const uris = getSelectionData(table, attribute);
            showDelPlaylist(uris);
            break;
        }
        case 'showCopyPlaylist': {
            const plists = getSelectionData(table, attribute);
            showCopyPlaylist(plists);
            break;
        }
    }
}
