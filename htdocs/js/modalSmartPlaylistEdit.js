"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalSmartPlaylistEdit_js */

/**
 * Shows the settings of the smart playlist
 * @param {string} plist smart playlist name
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showSmartPlaylist(plist) {
    sendAPI("MYMPD_API_SMARTPLS_GET", {
        "plist": plist
    }, parseSmartPlaylist, false);
}

/**
 * Parses the MYMPD_API_SMARTPLS_GET jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parseSmartPlaylist(obj) {
    document.getElementById('saveSmartPlaylistName').value = obj.result.plist;
    document.getElementById('saveSmartPlaylistType').value = tn(obj.result.type);
    document.getElementById('saveSmartPlaylistSort').value = obj.result.sort;
    setDataId('saveSmartPlaylistType', 'value', obj.result.type);
    elHideId('saveSmartPlaylistSearch');
    elHideId('saveSmartPlaylistSticker');
    elHideId('saveSmartPlaylistNewest');

    switch(obj.result.type) {
        case 'search':
            elShowId('saveSmartPlaylistSearch');
            document.getElementById('inputSaveSmartPlaylistExpression').value = obj.result.expression;
            break;
        case 'sticker':
            elShowId('saveSmartPlaylistSticker');
            document.getElementById('selectSaveSmartPlaylistSticker').value = obj.result.sticker;
            document.getElementById('inputSaveSmartPlaylistStickerMaxentries').value = obj.result.maxentries;
            document.getElementById('inputSaveSmartPlaylistStickerMinvalue').value = obj.result.minvalue;
            break;
        case 'newest':
            elShowId('saveSmartPlaylistNewest');
            document.getElementById('inputSaveSmartPlaylistNewestTimerange').value = obj.result.timerange / 24 / 60 / 60;
            break;
        default:
            logError('Invalid smart playlist type: ' + obj.result.type);
    }
    cleanupModalId('modalSmartPlaylistEdit');
    uiElements.modalSmartPlaylistEdit.show();
}

/**
 * Saves a smart playlist
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function saveSmartPlaylist() {
    cleanupModalId('modalSmartPlaylistEdit');

    const name = document.getElementById('saveSmartPlaylistName').value;
    const type = getDataId('saveSmartPlaylistType', 'value');
    const sort = getSelectValueId('saveSmartPlaylistSort');
    if (validatePlist(name) === false) {
        document.getElementById('saveSmartPlaylistName').classList.add('is-invalid');
        return;
    }

    switch(type) {
        case 'search':
            sendAPI("MYMPD_API_SMARTPLS_SEARCH_SAVE", {
                "plist": name,
                "expression": document.getElementById('inputSaveSmartPlaylistExpression').value,
                "sort": sort
            }, saveSmartPlaylistClose, true);
            break;
        case 'sticker': {
            const maxentriesEl = document.getElementById('inputSaveSmartPlaylistStickerMaxentries');
            if (!validateIntEl(maxentriesEl)) {
                return;
            }
            const minvalueEl = document.getElementById('inputSaveSmartPlaylistStickerMinvalue');
            if (!validateIntEl(minvalueEl)) {
                return;
            }
            sendAPI("MYMPD_API_SMARTPLS_STICKER_SAVE", {
                "plist": name,
                "sticker": getSelectValueId('selectSaveSmartPlaylistSticker'),
                "maxentries": Number(maxentriesEl.value),
                "minvalue": Number(minvalueEl.value),
                "sort": sort
            }, saveSmartPlaylistClose, true);
            break;
        }
        case 'newest': {
            const timerangeEl = document.getElementById('inputSaveSmartPlaylistNewestTimerange');
            if (!validateIntEl(timerangeEl)) {
                return;
            }
            sendAPI("MYMPD_API_SMARTPLS_NEWEST_SAVE", {
                "plist": name,
                "timerange": Number(timerangeEl.value) * 60 * 60 * 24,
                "sort": sort
            }, saveSmartPlaylistClose, true);
            break;
        }
        default:
            document.getElementById('saveSmartPlaylistType').classList.add('is-invalid');
    }
}

/**
 * Handles the MYMPD_API_SMARTPLS_*_SAVE responses
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function saveSmartPlaylistClose(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        uiElements.modalSmartPlaylistEdit.hide();
        showNotification(tn('Saved smart playlist'), 'playlist', 'info');
    }
}

/**
 * Adds a default smart playlist
 * @param {string} type one of mostPlayed, newest, bestRated
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addSmartpls(type) {
    const obj = {"jsonrpc": "2.0", "id": 0, "result": {"method": "MYMPD_API_SMARTPLS_GET"}};
    switch(type) {
        case 'mostPlayed':
            obj.result.plist = settings.smartplsPrefix + (settings.smartplsPrefix !== '' ? '-' : '') + 'mostPlayed';
            obj.result.type = 'sticker';
            obj.result.sticker = 'playCount';
            obj.result.maxentries = 200;
            obj.result.minvalue = 10;
            break;
        case 'newest':
            obj.result.plist = settings.smartplsPrefix + (settings.smartplsPrefix !== '' ? '-' : '') + 'newestSongs';
            obj.result.type = 'newest';
            //14 days
            obj.result.timerange = 1209600;
            break;
        case 'bestRated':
            obj.result.plist = settings.smartplsPrefix + (settings.smartplsPrefix !== '' ? '-' : '') + 'bestRated';
            obj.result.type = 'sticker';
            obj.result.sticker = 'like';
            obj.result.maxentries = 200;
            obj.result.minvalue = 2;
            break;
        default:
            logError('Invalid smart playlist type: ' + type);
    }
    parseSmartPlaylist(obj);
}
