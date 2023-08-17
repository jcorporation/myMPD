"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalQueueAddTo_js */

/**
 * Initializes the modalQueueAddTo
 * @returns {void}
 */
function initModalQueueAddTo() {
    document.getElementById('modalQueueAddToModeInput').addEventListener('change', function() {
        const value = Number(getSelectValue(this));
        if (value === 2) {
            //album mode
            elDisableId('modalQueueAddToQuantityInput');
            document.getElementById('modalQueueAddToQuantityInput').value = '1';
            elDisableId('modalQueueAddToPlaylistInput');
            document.getElementById('modalQueueAddToPlaylistInput').value = 'Database';
        }
        else if (value === 1) {
            //song mode
            elEnableId('modalQueueAddToQuantityInput');
            elEnableId('modalQueueAddToPlaylistInput');
        }
    });

    document.getElementById('modalQueueAddTo').addEventListener('shown.bs.modal', function() {
        cleanupModalId('modalQueueAddTo');
        document.getElementById('modalQueueAddToPlaylistInput').value = tn('Database');
        setDataId('modalQueueAddToPlaylistInput', 'value', 'Database');
        document.getElementById('modalQueueAddToPlaylistInput').filterInput.value = '';
        if (features.featPlaylists === true) {
            filterPlaylistsSelect(0, 'modalQueueAddToPlaylistInput', '', 'Database');
        }
    });

    setDataId('modalQueueAddToPlaylistInput', 'cb-filter', 'filterPlaylistsSelect');
    setDataId('modalQueueAddToPlaylistInput', 'cb-filter-options', [0, 'modalQueueAddToPlaylistInput']);
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
        "quantity": Number(document.getElementById('modalQueueAddToQuantityInput').value)
    }, modalClose, true);
}
