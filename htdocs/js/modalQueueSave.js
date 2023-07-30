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
        const plName = document.getElementById('saveQueueName');
        setFocus(plName);
        plName.value = '';
        toggleBtnGroupValueId('btnQueueSaveMode', 'create');
        toggleSaveQueueMode(document.getElementById('btnQueueSaveMode').firstElementChild);
        document.getElementById('saveQueueNameSelect').value = '';
        document.getElementById('saveQueueNameSelect').filterInput.value = '';
        filterPlaylistsSelect(1, 'saveQueueNameSelect', '', '');
        cleanupModalId('modalQueueSave');
    });

    setDataId('saveQueueNameSelect', 'cb-filter', 'filterPlaylistsSelect');
    setDataId('saveQueueNameSelect', 'cb-filter-options', [1, 'saveQueueNameSelect']);
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
    if (value === 'create') {
        elShowId('rowSaveQueueName');
        elHideId('rowSaveQueueNameSelect');
    }
    else {
        elHideId('rowSaveQueueName');
        elShowId('rowSaveQueueNameSelect');
    }
}

/**
 * Saves the queue as a playlist
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function saveQueue() {
    cleanupModalId('modalQueueSave');
    const plNameEl = document.getElementById('saveQueueName');
    let name = plNameEl.value;
    let saveMode = 'create';
    let formOK = true;
    if (features.featAdvqueue === true) {
        //support queue save modes (since MPD 0.24)
        saveMode = getBtnGroupValueId('btnQueueSaveMode');
        if (saveMode !== 'create') {
            //append or replace existing playlist
            name = getDataId('saveQueueNameSelect', 'value');
        }
        else {
            formOK = validatePlistEl(plNameEl);
        }
    }
    else {
        formOK = validatePlistEl(plNameEl);
    }
    if (formOK === true) {
        sendAPI("MYMPD_API_QUEUE_SAVE", {
            "plist": name,
            "mode": saveMode
        }, saveQueueCheckError, true);
    }
}

/**
 * Handler for the MYMPD_API_QUEUE_SAVE jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function saveQueueCheckError(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        uiElements.modalQueueSave.hide();
    }
}
