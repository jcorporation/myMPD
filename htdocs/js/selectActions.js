"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module selectActions_js */

/**
 * Initializes the select action dropdowns
 * @returns {void}
 */
function initSelectActions() {
    for (const dropdownId of [
        'QueueCurrentSelectionDropdown',
        'QueueLastPlayedSelectionDropdown',
        'QueueJukeboxSongSelectionDropdown',
        'QueueJukeboxAlbumSelectionDropdown',
        'BrowseDatabaseAlbumListSelectionDropdown',
        'BrowseDatabaseAlbumDetailSelectionDropdown',
        'BrowseFilesystemSelectionDropdown',
        'BrowsePlaylistListSelectionDropdown',
        'BrowsePlaylistDetailSelectionDropdown',
        'BrowseRadioFavoritesSelectionDropdown',
        'BrowseRadioWebradiodbSelectionDropdown',
        'SearchSelectionDropdown'
    ]) {
        const el = document.querySelector('#' + dropdownId + '> div');
        elGetById(dropdownId).parentNode.addEventListener('show.bs.dropdown', function() {
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
    const parent = elGetById(app.id + 'List');
    const firstSelection = parent.querySelector('.selected');
    const type = firstSelection !== null
        ? getData(firstSelection, 'type')
        : 'song';

    if (dropdownId !== 'QueueCurrentSelectionDropdown') {
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
    if (dropdownId === 'QueueJukeboxSongSelectionDropdown' ||
        dropdownId === 'QueueJukeboxAlbumSelectionDropdown')
    {
        addSelectActionButton(el, {"cmd": "execSelectAction", "options": [type, "delQueueJukeboxEntry"]}, 'Remove');
    }
    if (dropdownId === 'BrowseRadioFavoritesSelectionDropdown') {
        addSelectActionButton(el, {"cmd": "execSelectAction", "options": [type, "delRadioFavorites"]}, 'Delete');
    }
    if (dropdownId === 'BrowsePlaylistDetailSelectionDropdown') {
        const ro = getData(parent, 'ro');
        if (ro === false) {
            addSelectActionButton(el, {"cmd": "execSelectAction", "options": [type, "removeFromPlaylistPositions"]}, 'Remove');
        }
    }
    if (dropdownId === 'BrowsePlaylistListSelectionDropdown') {
        addDivider(el);
        addSelectActionButton(el, {"cmd": "execSelectAction", "options": [type, "showDelPlaylist"]}, 'Delete');
        addSelectActionButton(el, {"cmd": "execSelectAction", "options": [type, "showCopyPlaylist"]}, 'Copy');
    }
    if (features.featPlaylists === true &&
        type !== 'plist' &&
        type !== 'smartpls' &&
        dropdownId !== 'BrowsePlaylistListSelectionDropdown')
    {
        addSelectActionButton(el, {"cmd": "execSelectAction", "options": [type, "showAddToPlaylist"]}, 'Add to playlist');
    }
    if (dropdownId === 'BrowsePlaylistDetailSelectionDropdown' &&
        getData(parent, 'type') !== 'smartpls')
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
    const rows = parent.querySelectorAll('.selected');
    for (const row of rows) {
        data.push(getData(row, attribute));
    }
    return data;
}

/**
 * Returns the attribute for the select action type
 * @param {string} type Action type
 * @param {string} action action to handle
 * @returns {string} Attribute name
 */
function getExecSelectActionAttribute(type, action) {
    if (type === 'album') {
        switch(action) {
            case 'delQueueJukeboxEntry':
                return 'pos';
            default:
                return 'AlbumId';
        }
    }

    switch(action) {
        case 'playAfterCurrent':
        case 'removeFromQueueIDs':
            return'songid';
        case 'showMoveToPlaylist':
        case 'removeFromPlaylistPositions':
        case 'delQueueJukeboxEntry':
            return'pos';
        case'delRadioFavorites':
            return 'name';
        default:
            return 'uri';
    }
}

/**
 * Handles the selection actions
 * @param {string} type entity type
 * @param {string} action action to handle
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function execSelectAction(type, action) {
    const parent = elGetById(app.id + 'List');
    const attribute = getExecSelectActionAttribute(type, action);
    const data = getSelectionData(parent, attribute);
    switch(action) {
        case 'appendQueue':
            appendQueue(type, data);
            break;
        case 'appendPlayQueue':
            appendPlayQueue(type, data);
            break;
        case 'playAfterCurrent':
            playAfterCurrent(data);
            break;
        case 'insertAfterCurrentQueue':
            insertAfterCurrentQueue(type, data);
            break;
        case 'replaceQueue':
            replaceQueue(type, data);
            break;
        case 'replacePlayQueue':
            replacePlayQueue(type, data);
            break;
        case 'removeFromQueueIDs':
            removeFromQueueIDs(data);
            break;
        case 'showAddToPlaylist':
            showAddToPlaylist(type, data);
            break;
        case 'showMoveToPlaylist': {
            const plist = getData(parent, 'uri');
            showMoveToPlaylist(plist, data);
            break;
        }
        case 'removeFromPlaylistPositions': {
            const plist = getData(parent, 'uri');
            removeFromPlaylistPositions(plist, data);
            break;
        }
        case 'showDelPlaylist':
            showDelPlaylist(data);
            break;
        case 'showCopyPlaylist':
            showCopyPlaylist(data);
            break;
        case 'delRadioFavorites':
            deleteRadioFavorites(data);
            break;
        case 'delQueueJukeboxEntry':
            delQueueJukeboxEntries(data);
            break;
        default:
            logError('Invalid select action: ' + action);
    }
}
