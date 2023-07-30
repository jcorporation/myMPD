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
    document.getElementById('modalQueueSetSongPriority').addEventListener('shown.bs.modal', function() {
        const prioEl = document.getElementById('inputSongPriority');
        setFocus(prioEl);
        prioEl.value = '';
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
    cleanupModalId('modalQueueSetSongPriority');
    document.getElementById('inputSongPrioritySondId').value = songId;
    uiElements.modalQueueSetSongPriority.show();
}

/**
 * Sets song priority
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function setSongPriority() {
    cleanupModalId('modalQueueSetSongPriority');

    const songId = Number(document.getElementById('inputSongPrioritySongId').value);
    const priorityEl = document.getElementById('inputSongPriority');
    if (validateIntRangeEl(priorityEl, 0, 255) === true) {
        sendAPI("MYMPD_API_QUEUE_PRIO_SET", {
            "songIds": [songId],
            "priority": Number(priorityEl.value)
        }, setSongPriorityCheckError, true);
    }
}

/**
 * Handles the MYMPD_API_QUEUE_PRIO_SET jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function setSongPriorityCheckError(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        uiElements.modalQueueSetSongPriority.hide();
    }
}
