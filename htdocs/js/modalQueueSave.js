"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalQueueSave_js */

/**
 * Initializes the modalQueueSave
 * @returns {void}
 */
function initModalQueueSave() {
    document.getElementById('modalQueueSave').addEventListener('shown.bs.modal', function() {
        toggleBtnGroupValueId('modalQueueSaveModeInput', 'create');
        toggleSaveQueueMode(document.getElementById('modalQueueSaveModeInput').firstElementChild);
        document.getElementById('modalQueueSavePlistInput').setValue('', '');
        setDataId('modalQueueSavePlistInput', 'value', '');
        document.getElementById('modalQueueSavePlistInput').filterInput.value = '';
        filterPlaylistsSelect(1, 'modalQueueSavePlistInput', '', '');
        cleanupModalId('modalQueueSave');
    });

    setDataId('modalQueueSavePlistInput', 'cb-filter', 'filterPlaylistsSelect');
    setDataId('modalQueueSavePlistInput', 'cb-filter-options', [1, 'modalQueueSavePlistInput']);
}

/**
 * Toggles the queue save mode options
 * @param {EventTarget} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function toggleSaveQueueMode(target) {
    toggleBtnGroup(target);
    const value = getData(target, 'value');
    const input = document.getElementById('modalQueueSavePlistInput');
    input.setValue('', '');
    if (value === 'create') {
        input.removeAttribute('readonly');
        input.setAttribute('placeholder', tn('New playlist name'));
        elHide(input.nextElementSibling);
    }
    else {
        input.setAttribute('readonly', 'readonly');
        input.setAttribute('placeholder', tn('Choose a playlist'));
        elShow(input.nextElementSibling);
    }
}

/**
 * Saves the queue as a playlist
 * @param {Element} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function saveQueue(target) {
    cleanupModalId('modalQueueSave');
    btnWaiting(target, true);
    sendAPI("MYMPD_API_QUEUE_SAVE", {
        "plist": getDataId('modalQueueSavePlistInput', 'value'),
        "mode": getBtnGroupValueId('modalQueueSaveModeInput')
    }, modalClose, true);
}
