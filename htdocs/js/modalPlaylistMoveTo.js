"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalPlaylistMoveTo_js */

/**
 * Shows the move to playlist modal
 * @param {string} srcPlist playlist to remove the entries
 * @param {Array} positions song positions in srcPlist to move
 * @returns {void}
 */
function showMoveToPlaylist(srcPlist, positions) {
    const modal = elGetById('modalPlaylistMoveTo');
    cleanupModal(modal);
    setData(modal, 'srcPlist', srcPlist);
    setData(modal, 'positions', positions);
    filterPlaylistsSelect(1, 'modalPlaylistMoveToDstPlistInput', '', '');
    const positionsHuman = [];
    for (const pos of positions) {
        positionsHuman.push(pos + 1);
    }
    populateEntities('modalPlaylistMoveToPositions', positionsHuman);
    uiElements.modalPlaylistMoveTo.show();
}

/**
 * Adds the selected elements from the "move to playlist" modal to the playlist
 * @param {Element} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function moveToPlaylist(target) {
    const modal = elGetById('modalPlaylistMoveTo');
    cleanupModal(modal);
    const mode = getRadioBoxValueId('modalPlaylistMoveToPos');
    btnWaiting(target, true);
    sendAPI("MYMPD_API_PLAYLIST_CONTENT_MOVE_TO_PLAYLIST", {
        "srcPlist": getData(modal, 'srcPlist'),
        "dstPlist": elGetById('modalPlaylistMoveToDstPlistInput').value,
        "positions": getData(modal, 'positions'),
        "mode": Number(mode)
    }, modalClose, true);
}
