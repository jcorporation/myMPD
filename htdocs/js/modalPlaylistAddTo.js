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
        if (document.getElementById('addStreamFrm').classList.contains('d-none')) {
            setFocusId('addToPlaylistPlaylist');
        }
        else {
            setFocusId('streamUrl');
            document.getElementById('streamUrl').value = '';
        }
    });

    setDataId('addToPlaylistPlaylist', 'cb-filter', 'filterPlaylistsSelect');
    setDataId('addToPlaylistPlaylist', 'cb-filter-options', [1, 'addToPlaylistPlaylist']);
}

/**
 * Shows the add to playlist modal
 * @param {string} type one off album, disc, search, song, stream
 * @param {Array} entities entities to add
 * @returns {void}
 */
function showAddToPlaylist(type, entities) {
    cleanupModalId('modalPlaylistAddTo');
    setDataId('addToPlaylistEntities', 'type', type);
    setDataId('addToPlaylistEntities', 'entities', entities);
    document.getElementById('addToPlaylistPlaylist').value = '';
    document.getElementById('addToPlaylistPlaylist').filterInput.value = '';
    document.getElementById('addToPlaylistPosAppend').checked = 'checked';
    document.getElementById('streamUrl').value = '';
    if (type === 'stream') {
        //add stream
        toggleAddToPlaylistFrm(document.getElementById('toggleAddToPlaylistQueue'));
        elShowId('addStreamFrm');
        document.getElementById('addToPlaylistCaption').textContent = tn('Add stream');
    }
    else {
        //add to playlist
        toggleAddToPlaylistFrm(document.getElementById('toggleAddToPlaylistPlaylist'));
        elHideId('addStreamFrm');
        document.getElementById('addToPlaylistCaption').textContent = tn('Add to playlist');
    }
    uiElements.modalPlaylistAddTo.show();
    if (features.featPlaylists) {
        filterPlaylistsSelect(1, 'addToPlaylistPlaylist', '', '');
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
    if (target.getAttribute('id') === 'toggleAddToPlaylistPlaylist') {
        //add to playlist
        elShowId('addToPlaylistFrm');
        elShowId('addToPlaylistPosInsertFirstRow');
        elHideId('addToPlaylistPosInsertRow');
        elHideId('addToPlaylistPosAppendPlayRow');
        elHideId('addToPlaylistPosReplacePlayRow');
    }
    else {
        //add to queue
        elHideId('addToPlaylistFrm');
        elHideId('addToPlaylistPosInsertFirstRow');
        elShowId('addToPlaylistPosInsertRow');
        elShowId('addToPlaylistPosAppendPlayRow');
        elShowId('addToPlaylistPosReplacePlayRow');
    }
}

/**
 * Adds the selected elements from the "add to playlist" modal to the playlist or queue
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addToPlaylist() {
    cleanupModalId('modalPlaylistAddTo');
    const type = getDataId('addToPlaylistEntities', 'type');
    const entities = getDataId('addToPlaylistEntities', 'entities');
    const mode = getRadioBoxValueId('addToPlaylistPos');
    if (type === 'stream') {
        const streamUrlEl = document.getElementById('streamUrl');
        if (validateStreamEl(streamUrlEl) === false) {
            return;
        }
        entities[0] = streamUrlEl.value;
    }

    if (document.getElementById('addToPlaylistFrm').classList.contains('d-none') === false) {
        //add to playlist
        const plistEl = document.getElementById('addToPlaylistPlaylist');
        if (validatePlistEl(plistEl) === false) {
            return;
        }
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
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        uiElements.modalPlaylistAddTo.hide();
    }
}
