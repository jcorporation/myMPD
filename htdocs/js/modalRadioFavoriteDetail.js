"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalRadioWebradiodbDetail_js */

/**
 * Shows the details of a webradio favorites entry
 * @param {string} uri webradio uri
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showRadioFavoriteDetails(uri) {
    cleanupModalId('modalWebradiodbDetailList');
    elHideId('modalWebradiodbDetailAddToFavoriteBtn');
    sendAPI('MYMPD_API_WEBRADIO_FAVORITE_GET', {'filename': uri}, parseShowRadioFavoriteDetails, true);
}

/**
 * Parses the details of a webradio favorites entry
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parseShowRadioFavoriteDetails(obj) {
    const table = elGetById('modalWebradiodbDetailList');
    const tbody = table.querySelector('tbody');
    elClear(tbody);
    if (checkResult(obj, table, 'table') === false) {
        return;
    }

    if (obj.result.Image !== '') {
        elGetById('modalWebradiodbDetailImage').style.backgroundImage = getCssImageUri(obj.result.Image);
    }
    else {
        elGetById('modalWebradiodbDetailImage').style.backgroundImage = 'url("' + subdir + '/assets/coverimage-notavailable")';
    }
    elGetById('modalWebradiodbDetailTitle').textContent = obj.result.Name;
    setDataId('modalWebradiodbDetailTitle', 'webradio', obj.result);
    const showFields = [
        'StreamUri',
        'Homepage',
        'Genre',
        'Country',
        'State',
        'Language',
        'Codec',
        'Bitrate',
        'Description'
    ];
    for (const field of showFields) {
        const value = printValue(field, obj.result[field]);
        tbody.appendChild(
            elCreateNodes('tr', {}, [
                elCreateTextTn('th', {}, field),
                elCreateNode('td', {}, value)
            ])
        );
    }
    uiElements.modalWebradiodbDetail.show();
}
