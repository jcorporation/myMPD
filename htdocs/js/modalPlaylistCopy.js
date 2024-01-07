"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalPlaylistCopy_js */

/**
 * Shows the copy playlist modal
 * @param {Array} srcPlists playlist to remove the entries
 * @returns {void}
 */
function showCopyPlaylist(srcPlists) {
    const modal = elGetById('modalPlaylistCopy');
    cleanupModal(modal);
    setData(modal, 'srcPlists', srcPlists);
    filterPlaylistsSelect(1, 'modalPlaylistCopyDstPlistInput', '', '');
    populateEntities('modalPlaylistCopySrcPlists', srcPlists);
    uiElements.modalPlaylistCopy.show();
}

/**
 * Copies the playlist to another playlist
 * @param {Element} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function copyPlaylist(target) {
    const modal = elGetById('modalPlaylistCopy');
    cleanupModal(modal);
    const mode = getRadioBoxValueId('modalPlaylistCopyMode');
    btnWaiting(target, true);
    sendAPI("MYMPD_API_PLAYLIST_COPY", {
        "srcPlists": getData(modal, 'srcPlists'),
        "dstPlist": elGetById('modalPlaylistCopyDstPlistInput').value,
        "mode": Number(mode)
    }, modalClose, true);
}
