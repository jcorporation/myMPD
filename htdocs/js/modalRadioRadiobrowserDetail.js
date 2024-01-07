"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalRadioRadiobrowserDetail_js */

/**
 * Initialization function for the radiobrowser details modal
 * @param {Event} event triggering event
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showAddToWebradioFavorites(event) {
    event.preventDefault();
    showEditRadioFavorite(getDataId('RadiobrowserDetailsTitle', 'webradio'));
}

/**
 * Shows the details of a radiobrowser station
 * @param {string} uuid station uuid
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showRadiobrowserDetails(uuid) {
    sendAPI("MYMPD_API_CLOUD_RADIOBROWSER_STATION_DETAIL", {
        "uuid": uuid
    }, parseRadiobrowserDetails, true);
    uiElements.modalRadiobrowserDetails.show();
    elReplaceChildId('modalRadiobrowserDetailsList',
        elCreateNode('tr', {}, 
            elCreateTextTn('td', {"colspan": 2}, 'Loading...')
        )
    );
    countClickRadiobrowser(uuid);
}

/**
 * Parses the MYMPD_API_CLOUD_RADIOBROWSER_STATION_DETAIL jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parseRadiobrowserDetails(obj) {
    const tbody = elGetById('modalRadiobrowserDetailsList');
    if (checkResult(obj, tbody) === false) {
        return;
    }
    elClearId('modalRadiobrowserDetailsList');
    const result = obj.result.data[0];
    if (result.favicon !== '') {
        elGetById('modalRadiobrowserDetailsImage').style.backgroundImage = getCssImageUri(result.favicon);
    }
    else {
        elGetById('modalRadiobrowserDetailsImage').style.backgroundImage =
            'url("' + subdir + '/assets/coverimage-notavailable")';
    }
    elGetById('RadiobrowserDetailsTitle').textContent = result.name;
    //map fields to webradiodb fields
    setDataId('RadiobrowserDetailsTitle', 'webradio', {
        "Name": result.name,
        "StreamUri": result.url_resolved,
        "Genre": result.tags,
        "Homepage": result.homepage,
        "Country": result.country,
        "Language": result.language,
        "Codec": result.codec,
        "Bitrate": result.bitrate,
        "Description": "",
        "Image": result.favicon
    });
    //friendly names for fields
    const showFields = {
        'url_resolved': 'StreamUri',
        'homepage': 'Homepage',
        'tags': 'Tags',
        'country': 'Country',
        'language': 'Language',
        'codec': 'Codec',
        'bitrate': 'Bitrate',
        'votes': 'Votes',
        'lastchangetime': 'Last change time',
        'lastcheckok': 'State',
        'clickcount': 'Click count'
    };
    for (const field in showFields) {
        const value = printValue(field, result[field]);
        tbody.appendChild(
            elCreateNodes('tr', {}, [
                elCreateTextTn('th', {}, showFields[field]),
                elCreateNode('td', {}, value)
            ])
        );
    }
}
