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
    const modal = document.getElementById('modalPlaylistMoveTo');
    cleanupModal(modal);
    setData(modal, 'srcPlist', srcPlist);
    setData(modal, 'positions', positions);
    filterPlaylistsSelect(1, 'modalPlaylistMoveToDstPlist', '', '');
    populateEntities('modalPlaylistMoveToPositions', positions);
    uiElements.modalPlaylistMoveTo.show();
}

/**
 * Adds the selected elements from the "move to playlist" modal to the playlist
 * @param {Element} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function moveToPlaylist(target) {
    const modal = document.getElementById('modalPlaylistMoveTo');
    cleanupModal(modal);
    const srcPlist = getData(modal, 'srcPlist');
    const positions = getData(modal, 'positions');
    const mode = getRadioBoxValueId('modalPlaylistMoveToPos');
    btnWaiting(target, true);
    sendAPI("MYMPD_API_PLAYLIST_CONTENT_MOVE_TO_PLAYLIST", {
        "srcPlist": srcPlist,
        "dstPlist": document.getElementById('modalPlaylistMoveToDstPlist').value,
        "positions": positions,
        "mode": Number(mode)
    }, modalClose, true);
}
