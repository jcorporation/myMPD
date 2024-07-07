"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalRadioWebradiodbDetail_js */

/**
 * Shows the add to webradio favorites modal
 * @param {Event} event triggering event
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showAddToWebradioFavorites(event) {
    event.preventDefault();
    showEditRadioFavorite(getDataId('modalWebradiodbDetailTitle', 'webradio'));
}

/**
 * Fetches the details of a WebradioDB entry
 * @param {string} uri webradio uri
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showWebradiodbDetails(uri) {
    sendAPI("MYMPD_API_WEBRADIODB_RADIO_GET_BY_URI", {
        "uri": uri
    }, parseWebradiodbDetail, true);
}

/**
 * Parses the details of a WebradioDB entry and shows the modal
 * @param {object} obj Jsonrpc response
 * @returns {void}
 */
function parseWebradiodbDetail(obj) {
    elShowId('modalWebradiodbDetailAddToFavoriteBtn');
    const table = elGetById('modalWebradiodbDetailList');
    const tbody = table.querySelector('tbody');
    elClear(tbody);

    if (obj.error) {
        tbody.appendChild(obj.error, 1, 'table');
    }
    if (obj.result.Image !== '') {
        elGetById('modalWebradiodbDetailImage').style.backgroundImage = getCssImageUri(obj.result.Image);
    }
    else {
        elGetById('modalWebradiodbDetailImage').style.backgroundImage = 'url("' + subdir + '/assets/coverimage-notavailable")';
    }
    elGetById('modalWebradiodbDetailTitle').textContent = obj.result.Name;
    setDataId('modalWebradiodbDetailTitle', 'webradio', obj.result);
    for (const field of webradioFields) {
        const value = printValue(field, obj.result[field]);
        tbody.appendChild(
            elCreateNodes('tr', {}, [
                elCreateTextTn('th', {}, field),
                elCreateNode('td', {}, value)
            ])
        );
    }
    const alternateStreams = Object.keys(obj.result.alternativeStreams);
    if (alternateStreams.length > 0) {
        const td = elCreateEmpty('td', {});
        for (const name of alternateStreams) {
            const p = elCreateTextTn('p', {"class": ["pb-0"]}, 'Webradioformat',
                {"codec": obj.result.alternativeStreams[name].Codec, "bitrate": obj.result.alternativeStreams[name].Bitrate});
            const btn = elCreateText('button', {"class": ["btn", "btn-sm", "btn-secondary", "mi", "mi-sm", "ms-2"]}, 'favorite');
            p.appendChild(btn);
            td.appendChild(p);
            btn.addEventListener('click', function(event) {
                event.preventDefault();
                saveAsRadioFavorite(obj.result.alternativeStreams[name].StreamUri);
            }, false);
        }
        tbody.appendChild(
            elCreateNodes('tr', {}, [
                elCreateTextTn('th', {}, 'Alternative streams'),
                td
            ])
        );
    }
    uiElements.modalWebradiodbDetail.show();
}
