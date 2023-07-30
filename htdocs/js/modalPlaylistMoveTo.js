"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalPlaylistMoveTo_js */

/**
 * Shows the move to playlist modal
 * @param {string} srcPlist playlist to remove the entries
 * @param {Array} positions song positions in srcPlist to move
 * @returns {void}
 */
function showMoveToPlaylist(srcPlist, positions) {
    const modal = document.getElementById('modalMoveToPlaylist');
    cleanupModal(modal);
    setData(modal, 'srcPlist', srcPlist);
    setData(modal, 'positions', positions);
    filterPlaylistsSelect(1, 'moveToPlaylistPlaylist', '', '');
    uiElements.modalMoveToPlaylist.show();
}

/**
 * Adds the selected elements from the "move to playlist" modal to the playlist
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function moveToPlaylist() {
    const modal = document.getElementById('modalMoveToPlaylist');
    cleanupModal(modal);
    const srcPlist = getData(modal, 'srcPlist');
    const positions = getData(modal, 'positions');
    const mode = getRadioBoxValueId('moveToPlaylistPos');
    const plistEl = document.getElementById('moveToPlaylistPlaylist');
    if (validatePlistEl(plistEl) === false) {
        return;
    }
    sendAPI("MYMPD_API_PLAYLIST_CONTENT_MOVE_TO_PLAYLIST", {
        "srcPlist": srcPlist,
        "dstPlist": plistEl.value,
        "positions": positions,
        "mode": Number(mode)
    }, moveToPlaylistClose, true);
}

/**
 * Handles the response of "move to playlist" modal
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function moveToPlaylistClose(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        uiElements.modalMoveToPlaylist.hide();
    }
}
