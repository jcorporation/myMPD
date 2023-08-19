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
    elGetById('modalSmartPlaylistEditPlistInput').value = obj.result.plist;
    elGetById('modalSmartPlaylistEditTypeInput').value = tn(obj.result.type);
    elGetById('modalSmartPlaylistEditSortInput').value = obj.result.sort;
    setDataId('modalSmartPlaylistEditTypeInput', 'value', obj.result.type);
    elHideId('modalSmartPlaylistEditTypeSearch');
    elHideId('modalSmartPlaylistEditTypeSticker');
    elHideId('modalSmartPlaylistEditTypeNewest');

    switch(obj.result.type) {
        case 'search':
            elShowId('modalSmartPlaylistEditTypeSearch');
            elGetById('modalSmartPlaylistEditExpressionInput').value = obj.result.expression;
            break;
        case 'sticker':
            elShowId('modalSmartPlaylistEditTypeSticker');
            elGetById('modalSmartPlaylistEditStickerInput').value = obj.result.sticker;
            elGetById('modalSmartPlaylistEditMaxentriesInput').value = obj.result.maxentries;
            elGetById('modalSmartPlaylistEditMinvalueInput').value = obj.result.minvalue;
            break;
        case 'newest':
            elShowId('modalSmartPlaylistEditTypeNewest');
            elGetById('modalSmartPlaylistEditTimerangeInput').value = obj.result.timerange / 24 / 60 / 60;
            break;
        default:
            logError('Invalid smart playlist type: ' + obj.result.type);
    }
    cleanupModalId('modalSmartPlaylistEdit');
    uiElements.modalSmartPlaylistEdit.show();
}

/**
 * Saves a smart playlist
 * @param {Element} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function saveSmartPlaylist(target) {
    cleanupModalId('modalSmartPlaylistEdit');
    btnWaiting(target, true);

    const name = elGetById('modalSmartPlaylistEditPlistInput').value;
    const type = getDataId('modalSmartPlaylistEditTypeInput', 'value');
    const sort = getSelectValueId('modalSmartPlaylistEditSortInput');

    switch(type) {
        case 'search':
            sendAPI("MYMPD_API_SMARTPLS_SEARCH_SAVE", {
                "plist": name,
                "expression": elGetById('modalSmartPlaylistEditExpressionInput').value,
                "sort": sort
            }, modalClose, true);
            break;
        case 'sticker': {
            const maxentries = elGetById('modalSmartPlaylistEditMaxentriesInput').value;
            const minvalue = elGetById('modalSmartPlaylistEditMinvalueInput').value;
            sendAPI("MYMPD_API_SMARTPLS_STICKER_SAVE", {
                "plist": name,
                "sticker": getSelectValueId('modalSmartPlaylistEditStickerInput'),
                "maxentries": Number(maxentries),
                "minvalue": Number(minvalue),
                "sort": sort
            }, modalClose, true);
            break;
        }
        case 'newest': {
            const timerange = elGetById('modalSmartPlaylistEditTimerangeInput').value;
            sendAPI("MYMPD_API_SMARTPLS_NEWEST_SAVE", {
                "plist": name,
                "timerange": Number(timerange.value) * 60 * 60 * 24,
                "sort": sort
            }, modalClose, true);
            break;
        }
        default:
            elGetById('modalSmartPlaylistEditTypeInput').classList.add('is-invalid');
            btnWaiting(target, false);
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
