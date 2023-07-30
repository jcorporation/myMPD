"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalPlaylistRename_js */

/**
 * Shows the rename playlist modal
 * @param {string} from original playlist name
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showRenamePlaylist(from) {
    cleanupModalId('modalPlaylistRename');
    document.getElementById('renamePlaylistFrom').value = from;
    document.getElementById('renamePlaylistTo').value = '';
    uiElements.modalPlaylistRename.show();
}

/**
 * Renames the playlist
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function renamePlaylist() {
    cleanupModalId('modalPlaylistRename');

    const from = document.getElementById('renamePlaylistFrom').value;
    const to = document.getElementById('renamePlaylistTo').value;
    if (to !== from && validatePlist(to) === true) {
        sendAPI("MYMPD_API_PLAYLIST_RENAME", {
            "plist": from,
            "newName": to
        }, renamePlaylistClose, true);
    }
    else {
        document.getElementById('renamePlaylistTo').classList.add('is-invalid');
    }
}

/**
 * Handles the MYMPD_API_PLAYLIST_RENAME jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function renamePlaylistClose(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        uiElements.modalPlaylistRename.hide();
    }
}
