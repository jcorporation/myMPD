"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalQueueSetSongPriority_js */

/**
 * Initializes the modalQueueSetSongPriority
 * @returns {void}
 */
function initModalSetSongPriority() {
    document.getElementById('modalQueueSetSongPriority').addEventListener('show.bs.modal', function() {
        document.getElementById('modalQueueSetSongPriorityPriorityInput').value = '';
        cleanupModalId('modalQueueSetSongPriority');
    });
}

/**
 * Shows the set song priority modal
 * @param {number} songId the mpd song id
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showSetSongPriority(songId) {
    const modal = document.getElementById('modalQueueSetSongPriority');
    cleanupModal(modal);
    setData(modal, 'songId', songId);
    uiElements.modalQueueSetSongPriority.show();
}

/**
 * Sets song priority
 * @param {Element} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function setSongPriority(target) {
    const modal = document.getElementById('modalQueueSetSongPriority');
    cleanupModal(modal);
    btnWaiting(target, true);

    sendAPI("MYMPD_API_QUEUE_PRIO_SET", {
        "songIds": [getData(modal, 'songId')],
        "priority": Number(document.getElementById('modalQueueSetSongPriorityPriorityInput').value)
    }, modalClose, true);
}
