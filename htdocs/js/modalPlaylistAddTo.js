"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalPlaylistAddTo_js */

/**
 * Initializes the playlist elements
 * @returns {void}
 */
function initModalPlaylistAddTo() {
    setDataId('modalPlaylistAddToPlistInput', 'cb-filter', 'filterPlaylistsSelect');
    setDataId('modalPlaylistAddToPlistInput', 'cb-filter-options', [1, 'modalPlaylistAddToPlistInput']);
}

/**
 * Shows the add to playlist modal
 * @param {string} type one off album, disc, search, song, stream, searchdir
 * @param {Array} entities entities to add
 * @returns {void}
 */
function showAddToPlaylist(type, entities) {
    cleanupModalId('modalPlaylistAddTo');
    setDataId('modalPlaylistAddToUrisInput', 'type', type);
    setDataId('modalPlaylistAddToUrisInput', 'entities', entities);
    elGetById('modalPlaylistAddToPlistInput').value = '';
    elGetById('modalPlaylistAddToPlistInput').filterInput.value = '';
    elGetById('modalPlaylistAddToPosAppend').checked = 'checked';
    elGetById('modalPlaylistAddToUrisInput').value = '';
    if (entities.length === 0) {
        // manual add a stream
        toggleAddToPlaylistFrm(elGetById('modalPlaylistAddToQueueBtn'));
        elShowId('modalPlaylistAddToAddStreamFrm');
        elHideId('modalPlaylistAddToSrcRow');
        elGetById('addToPlaylistCaption').textContent = tn('Add stream');
    }
    else {
        // add to playlist
        toggleAddToPlaylistFrm(elGetById('modalPlaylistAddToPlaylistBtn'));
        elHideId('modalPlaylistAddToAddStreamFrm');
        elShowId('modalPlaylistAddToSrcRow');
        const names = [];
        if (type === 'searchdir') {
            names.push(tn('Path') + ': ' + entities[0]);
            if (entities[1] !== '') {
                names.push(tn('Search') + ': ' + entities[1]);
            }
        }
        else if (type === 'search') {
            names.push(tn('Search') + ': ' + entities[0]);
        }
        else {
            for (const entity of entities) {
                names.push(basename(entity, true));
            }
        }
        populateEntities('modalPlaylistAddToSrc', names);
        elGetById('addToPlaylistCaption').textContent = tn('Add to playlist');
    }
    if (features.featPlaylists) {
        filterPlaylistsSelect(1, 'modalPlaylistAddToPlistInput', '', '');
    }
    uiElements.modalPlaylistAddTo.show();
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
 * @param {Element} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addToPlaylist(target) {
    cleanupModalId('modalPlaylistAddTo');
    const type = getDataId('modalPlaylistAddToUrisInput', 'type');
    const entities = getDataId('modalPlaylistAddToUrisInput', 'entities');
    const mode = getRadioBoxValueId('modalPlaylistAddToPos');
    if (type === 'stream') {
        const streamUrlEl = elGetById('modalPlaylistAddToUrisInput');
        if (validateStreamEl(streamUrlEl) === false) {
            return;
        }
        entities[0] = streamUrlEl.value;
    }
    btnWaiting(target, true);
    if (elGetById('modalPlaylistAddToPlaylistFrm').classList.contains('d-none') === false) {
        // add to playlist
        const plistEl = elGetById('modalPlaylistAddToPlistInput');
        switch(mode) {
            case 'append':
                appendPlaylist(type, entities, plistEl.value, modalClose);
                break;
            case 'insertFirst':
                insertPlaylist(type, entities, plistEl.value, 0, modalClose);
                break;
            case 'replace':
                replacePlaylist(type, entities, plistEl.value, modalClose);
                break;
            default:
                logError('Invalid mode: ' + mode);
        }
    }
    else {
        // add to queue
        switch(mode) {
            case 'append':
                appendQueue(type, entities, modalClose);
                break;
            case 'appendPlay':
                appendPlayQueue(type, entities, modalClose);
                break;
            case 'insertAfterCurrent':
                insertAfterCurrentQueue(type, entities, modalClose);
                break;
            case 'insertPlayAfterCurrent':
                insertPlayAfterCurrentQueue(type, entities, modalClose);
                break;
            case 'replace':
                replaceQueue(type, entities, modalClose);
                break;
            case 'replacePlay':
                replacePlayQueue(type, entities, modalClose);
                break;
            default:
                logError('Invalid mode: ' + mode);
        }
    }
}
