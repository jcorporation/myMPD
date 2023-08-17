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
    document.getElementById('modalPlaylistRenamePlistInput').value = from;
    document.getElementById('modalPlaylistRenameNewNameInput').value = '';
    uiElements.modalPlaylistRename.show();
}

/**
 * Renames the playlist
 * @param {Element} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function renamePlaylist(target) {
    cleanupModalId('modalPlaylistRename');
    btnWaiting(target, true);
    sendAPI("MYMPD_API_PLAYLIST_RENAME", {
        "plist": document.getElementById('modalPlaylistRenamePlistInput').value,
        "newName": document.getElementById('modalPlaylistRenameNewNameInput').value
    }, modalClose, true);
}
