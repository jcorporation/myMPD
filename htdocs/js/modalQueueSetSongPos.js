"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalQueueSetSongPos_js */

/**
 * Shows the set song position modal
 * @param {string} plist the playlist name or the special value "queue"
 * @param {number} oldSongPos song pos in the playlist to move
 * @param {number} songId song id in the queue to move
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showSetSongPos(plist, oldSongPos, songId) {
    const modal = elGetById('modalQueueSetSongPos');
    cleanupModal(modal);
    setData(modal, 'songPosOld', oldSongPos);
    setData(modal, 'songId', songId);
    setData(modal, 'plist', plist);
    elGetById('modalQueueSetSongPosToInput').value = '';
    uiElements.modalQueueSetSongPos.show();
}

/**
 * Sets the song position in the queue / playlist
 * @param {Element} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function setSongPos(target) {
    const modal = elGetById('modalQueueSetSongPos');
    cleanupModal(modal);
    btnWaiting(target, true);
    const plist = getData(modal, 'plist');
    //MPD is zero indexed, display is 1-indexed
    const newSongPos = Number(elGetById('modalQueueSetSongPosToInput').value) - 1;
    if (plist === 'queue') {
        sendAPI("MYMPD_API_QUEUE_MOVE_ID", {
            "songIds": [getData(modal, 'songId')],
            "to": newSongPos
        }, modalClose, true);
    }
    else {
        sendAPI("MYMPD_API_PLAYLIST_CONTENT_MOVE_POSITION", {
            "plist": plist,
            "from": getData(modal, 'songPosOld'),
            "to": newSongPos
        }, modalClose, true);
    }
}
