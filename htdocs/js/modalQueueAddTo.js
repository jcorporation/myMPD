"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalQueueAddTo_js */

/**
 * Initializes the modalQueueAddTo
 * @returns {void}
 */
function initModalQueueAddTo() {
    elGetById('modalQueueAddToModeInput').addEventListener('change', function() {
        const value = Number(getSelectValue(this));
        if (value === 2) {
            //album mode
            elDisableId('modalQueueAddToQuantityInput');
            elGetById('modalQueueAddToQuantityInput').value = '1';
            elDisableId('modalQueueAddToPlaylistInput');
            elGetById('modalQueueAddToPlaylistInput').value = 'Database';
        }
        else if (value === 1) {
            //song mode
            elEnableId('modalQueueAddToQuantityInput');
            elEnableId('modalQueueAddToPlaylistInput');
        }
    });

    setDataId('modalQueueAddToPlaylistInput', 'cb-filter', 'filterPlaylistsSelect');
    setDataId('modalQueueAddToPlaylistInput', 'cb-filter-options', [0, 'modalQueueAddToPlaylistInput']);
}

/**
 * Shows the add random to queue modal
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showAddRandomToQueue() {
    cleanupModalId('modalQueueAddTo');
    elGetById('modalQueueAddToPlaylistInput').value = tn('Database');
    setDataId('modalQueueAddToPlaylistInput', 'value', 'Database');
    elGetById('modalQueueAddToPlaylistInput').filterInput.value = '';
    if (features.featPlaylists === true) {
        filterPlaylistsSelect(0, 'modalQueueAddToPlaylistInput', '', 'Database');
    }
    uiElements.modalQueueAddTo.show();
}

/**
 * Adds random songs/albums to the queue, one-shot jukebox mode.
 * @param {Element} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addRandomToQueue(target) {
    cleanupModalId('modalQueueAddTo');
    btnWaiting(target, true);
    sendAPI("MYMPD_API_QUEUE_ADD_RANDOM", {
        "mode": Number(getSelectValueId('modalQueueAddToModeInput')),
        "plist": getDataId('modalQueueAddToPlaylistInput', 'value'),
        "quantity": Number(elGetById('modalQueueAddToQuantityInput').value),
        "play": false
    }, modalClose, true);
}
