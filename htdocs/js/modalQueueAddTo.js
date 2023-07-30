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
    document.getElementById('selectAddToQueueMode').addEventListener('change', function() {
        const value = Number(getSelectValue(this));
        if (value === 2) {
            //album mode
            elDisableId('inputAddToQueueQuantity');
            document.getElementById('inputAddToQueueQuantity').value = '1';
            elDisableId('selectAddToQueuePlaylist');
            document.getElementById('selectAddToQueuePlaylist').value = 'Database';
        }
        else if (value === 1) {
            //song mode
            elEnableId('inputAddToQueueQuantity');
            elEnableId('selectAddToQueuePlaylist');
        }
    });

    document.getElementById('modalQueueAddTo').addEventListener('shown.bs.modal', function() {
        cleanupModalId('modalQueueAddTo');
        document.getElementById('selectAddToQueuePlaylist').value = tn('Database');
        setDataId('selectAddToQueuePlaylist', 'value', 'Database');
        document.getElementById('selectAddToQueuePlaylist').filterInput.value = '';
        if (features.featPlaylists === true) {
            filterPlaylistsSelect(0, 'selectAddToQueuePlaylist', '', 'Database');
        }
    });

    setDataId('selectAddToQueuePlaylist', 'cb-filter', 'filterPlaylistsSelect');
    setDataId('selectAddToQueuePlaylist', 'cb-filter-options', [0, 'selectAddToQueuePlaylist']);
}

/**
 * Adds random songs/albums to the queue, one-shot jukebox mode.
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addRandomToQueue() {
    cleanupModalId('modalQueueAddTo');
    let formOK = true;
    const inputAddToQueueQuantityEl = document.getElementById('inputAddToQueueQuantity');
    if (!validateIntEl(inputAddToQueueQuantityEl)) {
        formOK = false;
    }
    const selectAddToQueuePlaylistValue = getDataId('selectAddToQueuePlaylist', 'value');
    if (formOK === true) {
        sendAPI("MYMPD_API_QUEUE_ADD_RANDOM", {
            "mode": Number(getSelectValueId('selectAddToQueueMode')),
            "plist": selectAddToQueuePlaylistValue,
            "quantity": Number(document.getElementById('inputAddToQueueQuantity').value)
        }, addRandomToQueueCheckError, true);
    }
}

/**
 * Handles the MYMPD_API_QUEUE_ADD_RANDOM jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function addRandomToQueueCheckError(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        uiElements.modalQueueAddTo.hide();
    }
}
