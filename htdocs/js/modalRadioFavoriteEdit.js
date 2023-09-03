"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
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
    showEditRadioFavorite({
        "Name": "",
        "StreamUri": "",
        "Genre": "",
        "Homepage": "",
        "Country": "",
        "Language": "",
        "Codec": "",
        "Bitrate": "",
        "Description": ""
    });
}

/**
 * Opens the edit modal and populates the values from obj
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function showEditRadioFavorite(obj) {
    cleanupModalId('modalRadioFavoriteEdit');
    elGetById('modalRadioFavoriteEditNameInput').value = obj.Name === undefined ? '' : obj.Name;
    elGetById('modalRadioFavoriteEditStreamUriInput').value = obj.StreamUri === undefined ? '' : obj.StreamUri;
    elGetById('modalRadioFavoriteEditGenreInput').value = obj.Genre === undefined ? '' : obj.Genre;
    elGetById('modalRadioFavoriteEditHomepageInput').value = obj.Homepage === undefined ? '' : obj.Homepage;
    elGetById('modalRadioFavoriteEditCountryInput').value = obj.Country === undefined ? '' : obj.Country;
    elGetById('modalRadioFavoriteEditLanguageInput').value = obj.Language === undefined ? '' : obj.Language;
    elGetById('modalRadioFavoriteEditCodecInput').value = obj.Codec === undefined ? '' : obj.Codec;
    elGetById('modalRadioFavoriteEditBitrateInput').value = obj.Bitrate === undefined ? '' : obj.Bitrate;
    elGetById('modalRadioFavoriteEditDescriptionInput').value = obj.Description === undefined ? '' : obj.Description;

    setDataId('modalRadioFavoriteEdit', "StremUriOld", (obj.StreamUri === undefined ? '' : obj.StreamUri));

    const imageEl = elGetById('modalRadioFavoriteEditImageInput');
    getImageList(imageEl, [], 'thumbs');
    imageEl.value = obj.Image === undefined ? '' : obj.Image;
    setData(imageEl, 'value', obj.Image === undefined ? '' : obj.Image);

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
        "streamUri": elGetById('modalRadioFavoriteEditStreamUriInput').value,
        "streamUriOld": getDataId('modalRadioFavoriteEdit', "StremUriOld"),
        "genre": elGetById('modalRadioFavoriteEditGenreInput').value,
        "image": elGetById('modalRadioFavoriteEditImageInput').value,
        "homepage": elGetById('modalRadioFavoriteEditHomepageInput').value,
        "country": elGetById('modalRadioFavoriteEditCountryInput').value,
        "language": elGetById('modalRadioFavoriteEditLanguageInput').value,
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
            getRadioFavoriteList();
        }
    }
}


/**
 * Wrapper for _checkWebradioDb that fetches the webradioDB if needed
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function checkWebradioDb() {
    elGetById('webradiodbCheckState').textContent = tn('Checking...');
    btnWaitingId('modalRadioFavoriteEditCheckWebradiodbBtn', true);
    if (webradioDb === null) {
        //fetch webradiodb database
        sendAPI("MYMPD_API_CLOUD_WEBRADIODB_COMBINED_GET", {}, function(obj) {
            webradioDb = obj.result.data;
            _checkWebradioDb();
        }, false);
    }
    else {
        _checkWebradioDb();
    }
}

/**
 * Checks the local webradio favorite against the webradioDB entry
 * @returns {void}
 */
function _checkWebradioDb() {
    const streamUri = elGetById('modalRadioFavoriteEditStreamUriInput').value;
    if (streamUri !== '') {
        const webradio = streamUriToName(streamUri) + '.m3u';
        if (webradioDb.webradios[webradio] === undefined) {
            //not a main streamUri - check for alternate streams
            const streamName = streamUriToName(streamUri);
            let alternateStream = undefined;
            for (const key in webradioDb.webradios) {
                if (webradioDb.webradios[key].alternativeStreams[streamName] !== undefined) {
                    alternateStream = webradioDb.webradios[key].alternativeStreams[streamName];
                    break;
                }
            }
            if (alternateStream === undefined) {
                elShowId('modalRadioFavoriteEditAddToWebradiodbBtn');
                elHideId('modalRadioFavoriteEditUpdateWebradiodbBtn');
                elHideId('modalRadioFavoriteEditUpdateFromWebradiodbBtn');
                elGetById('webradiodbCheckState').textContent = tn('Uri not found in WebradioDB');
            }
            else {
                elHideId('modalRadioFavoriteEditAddToWebradiodbBtn');
                elHideId('modalRadioFavoriteEditUpdateWebradiodbBtn');
                elHideId('modalRadioFavoriteEditUpdateFromWebradiodbBtn');
                elGetById('webradiodbCheckState').textContent = tn('Alternative stream uri');
            }
        }
        else {
            elHideId('modalRadioFavoriteEditAddToWebradiodbBtn');
            if (compareWebradioDb() === false) {
                elShowId('modalRadioFavoriteEditUpdateWebradiodbBtn');
                elShowId('modalRadioFavoriteEditUpdateFromWebradiodbBtn');
                elHideId('modalRadioFavoriteEditCheckWebradiodbBtn');
                elGetById('webradiodbCheckState').textContent = tn('Favorite and WebradioDb entry are different');
            }
            else {
                elHideId('modalRadioFavoriteEditUpdateWebradiodbBtn');
                elHideId('modalRadioFavoriteEditUpdateFromWebradiodbBtn');
                elShowId('modalRadioFavoriteEditCheckWebradiodbBtn');
                elGetById('webradiodbCheckState').textContent = tn('Favorite is uptodate');
            }
        }
    }
    else {
        elHideId('modalRadioFavoriteEditAddToWebradiodbBtn');
        elHideId('modalRadioFavoriteEditUpdateWebradiodbBtn');
        elHideId('modalRadioFavoriteEditUpdateFromWebradiodbBtn');
        elShowId('modalRadioFavoriteEditCheckWebradiodbBtn');
        elGetById('webradiodbCheckState').textContent = tn('Empty uri');
    }
    btnWaitingId('modalRadioFavoriteEditCheckWebradiodbBtn', false);
}

/**
 * Compares the local webradio favorite with the entry from webradioDB
 * @returns {boolean} true if entries are equal, else false
 */
function compareWebradioDb() {
    let v1 = '';
    let v2 = '';
    const webradio = streamUriToName(elGetById('modalRadioFavoriteEditStreamUriInput').value) + '.m3u';
    for (const v of ['Name', 'StreamUri', 'Genre', 'Homepage', 'Image', 'Country', 'Language', 'Description', 'Codec', 'Bitrate']) {
        if (v === 'Image') {
            v1 += basename(elGetById('modalRadioFavoriteEdit' + v + 'Input').value, false);
        }
        else {
            v1 += elGetById('modalRadioFavoriteEdit' + v + 'Input').value;
        }
        v2 += webradioDb.webradios[webradio][v];
    }
    return v1 === v2;
}

/**
 * Updates the local webradio favorite from webradioDB
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function updateFromWebradioDb() {
    const webradio = streamUriToName(elGetById('modalRadioFavoriteEditStreamUriInput').value) + '.m3u';
    for (const v of ['Name', 'StreamUri', 'Genre', 'Homepage', 'Image', 'Country', 'Language', 'Description', 'Codec', 'Bitrate']) {
        if (v === 'Image') {
            elGetById('editRadioFavorite' + v).value = webradioDbPicsUri + webradioDb.webradios[webradio][v];
        }
        else {
            elGetById('editRadioFavorite' + v).value = webradioDb.webradios[webradio][v];
        }
    }
    _checkWebradioDb();
}

/**
 * Adds the local webradio favorite to the webradioDB
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addToWebradioDb() {
    const uri = 'https://github.com/jcorporation/webradiodb/issues/new?labels=AddWebradio&template=add-webradio.yml' +
        '&title=' + encodeURIComponent('[Add Webradio]: ' + elGetById('modalRadioFavoriteEditNameInput').value) +
        '&name=' + encodeURIComponent(elGetById('modalRadioFavoriteEditNameInput').value) +
        '&streamuri=' + encodeURIComponent(elGetById('modalRadioFavoriteEditStreamUriInput').value) +
        '&genre=' + encodeURIComponent(elGetById('modalRadioFavoriteEditGenreInput').value) +
        '&homepage=' + encodeURIComponent(elGetById('modalRadioFavoriteEditHomepageInput').value) +
        '&image=' + encodeURIComponent(elGetById('modalRadioFavoriteEditImageInput').value) +
        '&country=' + encodeURIComponent(elGetById('modalRadioFavoriteEditCountryInput').value) +
        '&language=' + encodeURIComponent(elGetById('modalRadioFavoriteEditLanguageInput').value) +
        '&codec=' + encodeURIComponent(elGetById('modalRadioFavoriteEditCodecInput').value) +
        '&bitrate=' + encodeURIComponent(elGetById('modalRadioFavoriteEditBitrateInput').value) +
        '&description=' + encodeURIComponent(elGetById('modalRadioFavoriteEditDescriptionInput').value);
    window.open(uri, '_blank');
}

/**
 * Updates the webradioDB entry from the local webradio favorite
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function updateWebradioDb() {
    const uri = 'https://github.com/jcorporation/webradiodb/issues/new?labels=ModifyWebradio&template=modify-webradio.yml' +
        '&modifyWebradio='  + encodeURIComponent(getDataId('modalRadioFavoriteEdit', "StremUriOld")) +
        '&title=' + encodeURIComponent('[Modify Webradio]: ' + elGetById('modalRadioFavoriteEditNameInput').value) +
        '&name=' + encodeURIComponent(elGetById('modalRadioFavoriteEditNameInput').value) +
        '&streamuri=' + encodeURIComponent(elGetById('modalRadioFavoriteEditStreamUriInput').value) +
        '&genre=' + encodeURIComponent(elGetById('modalRadioFavoriteEditGenreInput').value) +
        '&homepage=' + encodeURIComponent(elGetById('modalRadioFavoriteEditHomepageInput').value) +
        '&image=' + encodeURIComponent(elGetById('modalRadioFavoriteEditImageInput').value) +
        '&country=' + encodeURIComponent(elGetById('modalRadioFavoriteEditCountryInput').value) +
        '&language=' + encodeURIComponent(elGetById('modalRadioFavoriteEditLanguageInput').value) +
        '&codec=' + encodeURIComponent(elGetById('modalRadioFavoriteEditCodecInput').value) +
        '&bitrate=' + encodeURIComponent(elGetById('modalRadioFavoriteEditBitrateInput').value) +
        '&description=' + encodeURIComponent(elGetById('modalRadioFavoriteEditDescriptionInput').value);
    window.open(uri, '_blank');
}
