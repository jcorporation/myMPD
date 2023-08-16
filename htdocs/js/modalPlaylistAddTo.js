"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalPlaylistAddTo_js */

/**
 * Initializes the playlist elements
 * @returns {void}
 */
function initModalPlaylistAddTo() {
    document.getElementById('modalPlaylistAddTo').addEventListener('shown.bs.modal', function () {
        if (document.getElementById('modalPlaylistAddToAddStreamFrm').classList.contains('d-none')) {
            setFocusId('modalPlaylistAddToPlistInput');
        }
        else {
            setFocusId('modalPlaylistAddToUrisInput');
            document.getElementById('modalPlaylistAddToUrisInput').value = '';
        }
    });

    setDataId('modalPlaylistAddToPlistInput', 'cb-filter', 'filterPlaylistsSelect');
    setDataId('modalPlaylistAddToPlistInput', 'cb-filter-options', [1, 'modalPlaylistAddToPlistInput']);
}

/**
 * Shows the add to playlist modal
 * @param {string} type one off album, disc, search, song, stream
 * @param {Array} entities entities to add
 * @returns {void}
 */
function showAddToPlaylist(type, entities) {
    cleanupModalId('modalPlaylistAddTo');
    setDataId('modalPlaylistAddToUrisInput', 'type', type);
    setDataId('modalPlaylistAddToUrisInput', 'entities', entities);
    document.getElementById('modalPlaylistAddToPlistInput').value = '';
    document.getElementById('modalPlaylistAddToPlistInput').filterInput.value = '';
    document.getElementById('modalPlaylistAddToPosAppend').checked = 'checked';
    document.getElementById('modalPlaylistAddToUrisInput').value = '';
    if (type === 'stream') {
        //add stream
        toggleAddToPlaylistFrm(document.getElementById('modalPlaylistAddToQueueBtn'));
        elShowId('modalPlaylistAddToAddStreamFrm');
        elHideId('modalPlaylistAddToSrcRow');
        document.getElementById('addToPlaylistCaption').textContent = tn('Add stream');
    }
    else {
        //add to playlist
        toggleAddToPlaylistFrm(document.getElementById('modalPlaylistAddToPlaylistBtn'));
        elHideId('modalPlaylistAddToAddStreamFrm');
        const names = [];
        for (const entity of entities) {
            names.push(basename(entity, true));
        }
        document.getElementById('modalPlaylistAddToSrc').value = arrayToLines(names);
        elShowId('modalPlaylistAddToSrcRow');
        document.getElementById('addToPlaylistCaption').textContent = tn('Add to playlist');
    }
    uiElements.modalPlaylistAddTo.show();
    if (features.featPlaylists) {
        filterPlaylistsSelect(1, 'modalPlaylistAddToPlistInput', '', '');
    }
}

/**
 * Toggles the view in the add to playlist modal
 * @param {EventTarget} target event target
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function toggleAddToPlaylistFrm(target) {
    toggleBtnGroup(target);
    if (target.getAttribute('id') === 'modalPlaylistAddToPlaylistBtn') {
        //add to playlist
        elShowId('modalPlaylistAddToPlaylistFrm');
        elShowId('modalPlaylistAddToPosInsertFirstRow');
        elHideId('modalPlaylistAddToPosInsertRow');
        elHideId('modalPlaylistAddToPosAppendPlayRow');
        elHideId('modalPlaylistAddToPosReplacePlayRow');
    }
    else {
        //add to queue
        elHideId('modalPlaylistAddToPlaylistFrm');
        elHideId('modalPlaylistAddToPosInsertFirstRow');
        elShowId('modalPlaylistAddToPosInsertRow');
        elShowId('modalPlaylistAddToPosAppendPlayRow');
        elShowId('modalPlaylistAddToPosReplacePlayRow');
    }
}

/**
 * Adds the selected elements from the "add to playlist" modal to the playlist or queue
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addToPlaylist() {
    cleanupModalId('modalPlaylistAddTo');
    const type = getDataId('modalPlaylistAddToUrisInput', 'type');
    const entities = getDataId('modalPlaylistAddToUrisInput', 'entities');
    const mode = getRadioBoxValueId('modalPlaylistAddToPos');
    if (type === 'stream') {
        const streamUrlEl = document.getElementById('modalPlaylistAddToUrisInput');
        if (validateStreamEl(streamUrlEl) === false) {
            return;
        }
        entities[0] = streamUrlEl.value;
    }
    btnWaitingId('modalPlaylistAddToUriAddBtn', true);
    if (document.getElementById('modalPlaylistAddToPlaylistFrm').classList.contains('d-none') === false) {
        //add to playlist
        const plistEl = document.getElementById('modalPlaylistAddToPlistInput');
        switch(mode) {
            case 'append':
                appendPlaylist(type, entities, plistEl.value, addToPlaylistClose);
                break;
            case 'insertFirst':
                insertPlaylist(type, entities, plistEl.value, 0, addToPlaylistClose);
                break;
            case 'replace':
                replacePlaylist(type, entities, plistEl.value, addToPlaylistClose);
                break;
            default:
                logError('Invalid mode: ' + mode);
        }
    }
    else {
        //add to queue
        switch(mode) {
            case 'append':
                appendQueue(type, entities, addToPlaylistClose);
                break;
            case 'appendPlay':
                appendPlayQueue(type, entities, addToPlaylistClose);
                break;
            case 'insertAfterCurrent':
                insertAfterCurrentQueue(type, entities, addToPlaylistClose);
                break;
            case 'insertPlayAfterCurrent':
                insertPlayAfterCurrentQueue(type, entities, addToPlaylistClose);
                break;
            case 'replace':
                replaceQueue(type, entities, addToPlaylistClose);
                break;
            case 'replacePlay':
                replacePlayQueue(type, entities, addToPlaylistClose);
                break;
            default:
                logError('Invalid mode: ' + mode);
        }
    }
}

/**
 * Handles the response of "add to playlist" modal
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function addToPlaylistClose(obj) {
    btnWaitingId('modalPlaylistAddToUriAddBtn', false);
    if (obj.error) {
        if (highlightInvalidInput('modalPlaylistAddTo', obj) === false) {
            showModalAlert(obj);
        }
    }
    else {
        uiElements.modalPlaylistAddTo.hide();
    }
}
