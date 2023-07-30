"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalQueueSetSongPos_js */

/**
 * Shows the set song position modal
 * @param {string} plist the playlist name or the special value "queue" to move the song
 * @param {number} oldSongPos song pos in the queue to move
 * @param {number} songId song id in the queue to move
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showSetSongPos(plist, oldSongPos, songId) {
    cleanupModalId('modalQueueSetSongPos');
    document.getElementById('inputSongPosNew').value = '';
    document.getElementById('inputSongPosOld').value = oldSongPos;
    document.getElementById('inputSongId').value = songId;
    document.getElementById('inputSongPosPlist').value = plist;
    uiElements.modalQueueSetSongPos.show();
}

/**
 * Sets song position in queue or playlist
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function setSongPos() {
    cleanupModalId('modalQueueSetSongPos');
    const plist = document.getElementById('inputSongPosPlist').value;
    const oldSongPos = Number(document.getElementById('inputSongPosOld').value);
    const songId = Number(document.getElementById('inputSongId').value);
    const newSongPosEl = document.getElementById('inputSongPosNew');
    if (validateIntRangeEl(newSongPosEl, 1, 99999) === true) {
        let newSongPos = Number(newSongPosEl.value);
        if (newSongPos < oldSongPos) {
            newSongPos--;
        }
        if (plist === 'queue') {
            sendAPI("MYMPD_API_QUEUE_MOVE_ID", {
                "songIds": [songId],
                "to": newSongPos
            }, setSongPosCheckError, true);
        }
        else {
            sendAPI("MYMPD_API_PLAYLIST_CONTENT_MOVE_POSITION", {
                "plist": plist,
                "from": oldSongPos,
                "to": newSongPos
            }, setSongPosCheckError, true);
        }
    }
}

/**
 * Handles the MYMPD_API_QUEUE_MOVE_ID and MYMPD_API_PLAYLIST_CONTENT_MOVE_POSITION jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function setSongPosCheckError(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        uiElements.modalQueueSetSongPos.hide();
    }
}
