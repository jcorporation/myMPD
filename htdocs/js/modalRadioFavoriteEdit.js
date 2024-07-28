"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalRadioFavoritesEdit_js */

/**
 * Initialization function for modalRadioFavoriteEdit
 * @returns {void}
 */
function initModalRadioFavoriteEdit() {
    setDataId('modalRadioFavoriteEditImageInput', 'cb-filter', 'filterImageSelect');
    setDataId('modalRadioFavoriteEditImageInput', 'cb-filter-options', ['modalRadioFavoriteEditImageInput']);
}

/**
 * Shows the add webradio favorite modal
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function manualAddRadioFavorite() {
    showEditRadioFavorite({"result": {
        "Name": "",
        "StreamUri": "",
        "Genres": [],
        "Homepage": "",
        "Country": "",
        "Region": "",
        "Languages": [],
        "Codec": "",
        "Bitrate": "",
        "Description": ""
    }});
}

/**
 * Gets the WebradioDB entry and shows the "Add to favorite modal"
 * @param {string} uri Webradio Favorite uri
 * @returns {void}
 */
function saveAsRadioFavorite(uri) {
    sendAPI('MYMPD_API_WEBRADIODB_RADIO_GET_BY_URI', {
        "uri": uri
    }, showEditRadioFavorite, false);
}

/**
 * Opens the edit modal and populates the values from obj
 * @param {object} obj Jsonrpc response
 * @returns {void}
 */
function showEditRadioFavorite(obj) {
    cleanupModalId('modalRadioFavoriteEdit');

    elGetById('modalRadioFavoriteEditNameInput').value = obj.result.Name;
    elGetById('modalRadioFavoriteEditStreamUriInput').value = obj.result.StreamUri;
    elGetById('modalRadioFavoriteEditGenresInput').value = joinArray(obj.result.Genres);
    elGetById('modalRadioFavoriteEditHomepageInput').value = obj.result.Homepage;
    elGetById('modalRadioFavoriteEditCountryInput').value = obj.result.Country;
    elGetById('modalRadioFavoriteEditRegionInput').value = obj.result.Region;
    elGetById('modalRadioFavoriteEditLanguagesInput').value = joinArray(obj.result.Languages);
    elGetById('modalRadioFavoriteEditCodecInput').value = obj.result.Codec;
    elGetById('modalRadioFavoriteEditBitrateInput').value = obj.result.Bitrate;
    elGetById('modalRadioFavoriteEditDescriptionInput').value = obj.result.Description;

    setDataId('modalRadioFavoriteEdit', "oldName", obj.result.Name);

    const imageEl = elGetById('modalRadioFavoriteEditImageInput');
    getImageList(imageEl, [], 'thumbs');
    imageEl.value = obj.result.Image === undefined ? '' : obj.result.Image;
    setData(imageEl, 'value', obj.result.Image === undefined ? '' : obj.result.Image);

    elHideId('modalRadioFavoriteEditAddToWebradiodbBtn');
    elHideId('modalRadioFavoriteEditUpdateWebradiodbBtn');
    elHideId('modalRadioFavoriteEditUpdateFromWebradiodbBtn');
    elHideId('modalRadioFavoriteEditCheckWebradiodbBtn');
    checkWebradioDb();

    uiElements.modalRadioFavoriteEdit.show();
}

/**
 * Saves a webradio favorite
 * @param {Element} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function saveRadioFavorite(target) {
    cleanupModalId('modalRadioFavoriteEdit');
    btnWaiting(target, true);
    sendAPI("MYMPD_API_WEBRADIO_FAVORITE_SAVE", {
        "name": elGetById('modalRadioFavoriteEditNameInput').value,
        "oldName": getDataId('modalRadioFavoriteEdit', "oldName"),
        "streamUri": elGetById('modalRadioFavoriteEditStreamUriInput').value,
        "genres": stringToArray(elGetById('modalRadioFavoriteEditGenresInput').value),
        "image": elGetById('modalRadioFavoriteEditImageInput').value,
        "homepage": elGetById('modalRadioFavoriteEditHomepageInput').value,
        "country": elGetById('modalRadioFavoriteEditCountryInput').value,
        "region": elGetById('modalRadioFavoriteEditRegionInput').value,
        "languages": stringToArray(elGetById('modalRadioFavoriteEditLanguagesInput').value),
        "codec": elGetById('modalRadioFavoriteEditCodecInput').value,
        "bitrate": Number(elGetById('modalRadioFavoriteEditBitrateInput').value),
        "description": elGetById('modalRadioFavoriteEditDescriptionInput').value,
    }, saveRadioFavoriteCheckError, true);
}

/**
 * Handler for the MYMPD_API_WEBRADIO_FAVORITE_SAVE jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function saveRadioFavoriteCheckError(obj) {
    if (modalApply(obj) === true) {
        uiElements.modalRadioFavoriteEdit.hide();
        if (app.id === 'BrowseRadioFavorites') {
            handleBrowseRadioFavorites();
        }
    }
}

/**
 * Checks the local webradio favorite against the webradioDB entry
 * @returns {void}
 */
function checkWebradioDb() {
    const streamUri = elGetById('modalRadioFavoriteEditStreamUriInput').value;
    if (streamUri !== '') {
        btnWaitingId('modalRadioFavoriteEditCheckWebradiodbBtn', true);
        sendAPI('MYMPD_API_WEBRADIODB_RADIO_GET_BY_URI', {
            "uri": elGetById('modalRadioFavoriteEditStreamUriInput').value
        }, function(obj) {
            if (obj.result) {
                elHideId('modalRadioFavoriteEditAddToWebradiodbBtn');
                if (obj.result.StreamUri !== streamUri) {
                    elHideId('modalRadioFavoriteEditAddToWebradiodbBtn');
                    elHideId('modalRadioFavoriteEditUpdateWebradiodbBtn');
                    elHideId('modalRadioFavoriteEditUpdateFromWebradiodbBtn');
                    elGetById('webradiodbCheckState').textContent = tn('Alternative stream uri');
                }
                else if (compareWebradioDb(obj.result) === false) {
                    elShowId('modalRadioFavoriteEditUpdateWebradiodbBtn');
                    elShowId('modalRadioFavoriteEditUpdateFromWebradiodbBtn');
                    elHideId('modalRadioFavoriteEditCheckWebradiodbBtn');
                    elGetById('webradiodbCheckState').textContent = tn('Favorite and WebradioDb entry are different');
                }
                else {
                    elHideId('modalRadioFavoriteEditUpdateWebradiodbBtn');
                    elHideId('modalRadioFavoriteEditUpdateFromWebradiodbBtn');
                    elShowId('modalRadioFavoriteEditCheckWebradiodbBtn');
                    elGetById('webradiodbCheckState').textContent = tn('Favorite is up-to-date');
                }
            }
            else {
                elShowId('modalRadioFavoriteEditAddToWebradiodbBtn');
                elHideId('modalRadioFavoriteEditUpdateWebradiodbBtn');
                elHideId('modalRadioFavoriteEditUpdateFromWebradiodbBtn');
                elGetById('webradiodbCheckState').textContent = tn('Uri not found in WebradioDB');
            }
            btnWaitingId('modalRadioFavoriteEditCheckWebradiodbBtn', false);
        }, true);
    }
    else {
        elHideId('modalRadioFavoriteEditAddToWebradiodbBtn');
        elHideId('modalRadioFavoriteEditUpdateWebradiodbBtn');
        elHideId('modalRadioFavoriteEditUpdateFromWebradiodbBtn');
        elShowId('modalRadioFavoriteEditCheckWebradiodbBtn');
        elGetById('webradiodbCheckState').textContent = tn('Empty uri');
    }
}

/**
 * Compares the local webradio favorite with the entry from WebradioDB
 * @param {object} obj jsonrpc response result
 * @returns {boolean} true if entries are equal, else false
 */
function compareWebradioDb(obj) {
    let v1 = '';
    let v2 = '';
    for (const v of webradioFields) {
        if (v === 'Added' || v === 'Last-Modified') {
            continue;
        }
        v1 += elGetById('modalRadioFavoriteEdit' + v + 'Input').value;
        if (typeof obj[v] === 'object') {
            v2 += joinArray(obj[v]);
        }
        else {
            v2 += obj[v];
        }
    }
    return v1 === v2;
}

/**
 * Updates the local webradio favorite from WebradioDB
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function updateFromWebradioDb() {
    sendAPI('MYMPD_API_WEBRADIODB_RADIO_GET_BY_URI', {
        "uri": elGetById('modalRadioFavoriteEditStreamUriInput').value
    }, function(obj) {
        for (const v of webradioFields) {
            if (v === 'Added' || v === 'Last-Modified') {
                continue;
            }
            elGetById('modalRadioFavoriteEdit' + v + 'Input').value = obj.result[v];
        }
        checkWebradioDb();
    }, false);
}

/**
 * Adds the local webradio favorite to the WebradioDB
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addToWebradioDb() {
    const uri = 'https://github.com/jcorporation/webradiodb/issues/new?labels=AddWebradio&template=add-webradio.yml' +
        '&title=' + encodeURIComponent('[Add Webradio]: ' + elGetById('modalRadioFavoriteEditNameInput').value) +
        '&name=' + encodeURIComponent(elGetById('modalRadioFavoriteEditNameInput').value) +
        '&streamuri=' + encodeURIComponent(elGetById('modalRadioFavoriteEditStreamUriInput').value) +
        '&genre=' + encodeURIComponent(elGetById('modalRadioFavoriteEditGenresInput').value) +
        '&homepage=' + encodeURIComponent(elGetById('modalRadioFavoriteEditHomepageInput').value) +
        '&image=' + encodeURIComponent(elGetById('modalRadioFavoriteEditImageInput').value) +
        '&country=' + encodeURIComponent(elGetById('modalRadioFavoriteEditCountryInput').value) +
        '&region=' + encodeURIComponent(elGetById('modalRadioFavoriteEditRegionInput').value) +
        '&language=' + encodeURIComponent(elGetById('modalRadioFavoriteEditLanguagesInput').value) +
        '&codec=' + encodeURIComponent(elGetById('modalRadioFavoriteEditCodecInput').value) +
        '&bitrate=' + encodeURIComponent(elGetById('modalRadioFavoriteEditBitrateInput').value) +
        '&description=' + encodeURIComponent(elGetById('modalRadioFavoriteEditDescriptionInput').value);
    window.open(uri, '_blank');
}

/**
 * Updates the WebradioDB entry from the local webradio favorite
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function updateWebradioDb() {
    const uri = 'https://github.com/jcorporation/webradiodb/issues/new?labels=ModifyWebradio&template=modify-webradio.yml' +
        '&modifyWebradio='  + encodeURIComponent(getDataId('modalRadioFavoriteEdit', "StreamUriOld")) +
        '&title=' + encodeURIComponent('[Modify Webradio]: ' + elGetById('modalRadioFavoriteEditNameInput').value) +
        '&name=' + encodeURIComponent(elGetById('modalRadioFavoriteEditNameInput').value) +
        '&streamuri=' + encodeURIComponent(elGetById('modalRadioFavoriteEditStreamUriInput').value) +
        '&genre=' + encodeURIComponent(elGetById('modalRadioFavoriteEditGenresInput').value) +
        '&homepage=' + encodeURIComponent(elGetById('modalRadioFavoriteEditHomepageInput').value) +
        '&image=' + encodeURIComponent(elGetById('modalRadioFavoriteEditImageInput').value) +
        '&country=' + encodeURIComponent(elGetById('modalRadioFavoriteEditCountryInput').value) +
        '&region=' + encodeURIComponent(elGetById('modalRadioFavoriteEditRegionInput').value) +
        '&language=' + encodeURIComponent(elGetById('modalRadioFavoriteEditLanguagesInput').value) +
        '&codec=' + encodeURIComponent(elGetById('modalRadioFavoriteEditCodecInput').value) +
        '&bitrate=' + encodeURIComponent(elGetById('modalRadioFavoriteEditBitrateInput').value) +
        '&description=' + encodeURIComponent(elGetById('modalRadioFavoriteEditDescriptionInput').value);
    window.open(uri, '_blank');
}
