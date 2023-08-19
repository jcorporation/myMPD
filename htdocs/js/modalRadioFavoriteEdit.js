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
    setDataId('editRadioFavoriteImage', 'cb-filter', 'filterImageSelect');
    setDataId('editRadioFavoriteImage', 'cb-filter-options', ['editRadioFavoriteImage']);
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
    elGetById('editRadioFavoriteName').value = obj.Name === undefined ? '' : obj.Name;
    elGetById('editRadioFavoriteStreamUri').value = obj.StreamUri === undefined ? '' : obj.StreamUri;
    elGetById('editRadioFavoriteStreamUriOld').value = obj.StreamUri === undefined ? '' : obj.StreamUri;
    elGetById('editRadioFavoriteGenre').value = obj.Genre === undefined ? '' : obj.Genre;
    elGetById('editRadioFavoriteHomepage').value = obj.Homepage === undefined ? '' : obj.Homepage;
    elGetById('editRadioFavoriteCountry').value = obj.Country === undefined ? '' : obj.Country;
    elGetById('editRadioFavoriteLanguage').value = obj.Language === undefined ? '' : obj.Language;
    elGetById('editRadioFavoriteCodec').value = obj.Codec === undefined ? '' : obj.Codec;
    elGetById('editRadioFavoriteBitrate').value = obj.Bitrate === undefined ? '' : obj.Bitrate;
    elGetById('editRadioFavoriteDescription').value = obj.Description === undefined ? '' : obj.Description;

    const imageEl = elGetById('editRadioFavoriteImage');
    getImageList(imageEl, [], 'thumbs');
    imageEl.value = obj.Image === undefined ? '' : obj.Image;
    setData(imageEl, 'value', obj.Image === undefined ? '' : obj.Image);

    elHideId('btnAddToWebradiodb');
    elHideId('btnUpdateWebradiodb');
    elHideId('btnUpdateFromWebradiodb');
    elHideId('btnCheckWebradiodb');
    checkWebradioDb();

    uiElements.modalRadioFavoriteEdit.show();
}

/**
 * Saves a webradio favorite
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function saveRadioFavorite() {
    cleanupModalId('modalRadioFavoriteEdit');
    sendAPI("MYMPD_API_WEBRADIO_FAVORITE_SAVE", {
        "name": elGetById('editRadioFavoriteName').value,
        "streamUri": elGetById('editRadioFavoriteStreamUri').value,
        "streamUriOld": elGetById('editRadioFavoriteStreamUriOld').value,
        "genre": elGetById('editRadioFavoriteGenre').value,
        "image": elGetById('editRadioFavoriteImage').value,
        "homepage": elGetById('editRadioFavoriteHomepage').value,
        "country": elGetById('editRadioFavoriteCountry').value,
        "language": elGetById('editRadioFavoriteLanguage').value,
        "codec": elGetById('editRadioFavoriteCodec').value,
        "bitrate": Number(elGetById('editRadioFavoriteBitrate').value),
        "description": elGetById('editRadioFavoriteDescription').value,
    }, saveRadioFavoriteClose, true);
}

/**
 * Wrapper for _checkWebradioDb that fetches the webradioDB if needed
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function checkWebradioDb() {
    elGetById('webradiodbCheckState').textContent = tn('Checking...');
    btnWaitingId('btnCheckWebradiodb', true);
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
    const streamUri = elGetById('editRadioFavoriteStreamUri').value;
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
                elShowId('btnAddToWebradiodb');
                elHideId('btnUpdateWebradiodb');
                elHideId('btnUpdateFromWebradiodb');
                elGetById('webradiodbCheckState').textContent = tn('Uri not found in WebradioDB');
            }
            else {
                elHideId('btnAddToWebradiodb');
                elHideId('btnUpdateWebradiodb');
                elHideId('btnUpdateFromWebradiodb');
                elGetById('webradiodbCheckState').textContent = tn('Alternative stream uri');
            }
        }
        else {
            elHideId('btnAddToWebradiodb');
            if (compareWebradioDb() === false) {
                elShowId('btnUpdateWebradiodb');
                elShowId('btnUpdateFromWebradiodb');
                elHideId('btnCheckWebradiodb');
                elGetById('webradiodbCheckState').textContent = tn('Favorite and WebradioDb entry are different');
            }
            else {
                elHideId('btnUpdateWebradiodb');
                elHideId('btnUpdateFromWebradiodb');
                elShowId('btnCheckWebradiodb');
                elGetById('webradiodbCheckState').textContent = tn('Favorite is uptodate');
            }
        }
    }
    else {
        elHideId('btnAddToWebradiodb');
        elHideId('btnUpdateWebradiodb');
        elHideId('btnUpdateFromWebradiodb');
        elShowId('btnCheckWebradiodb');
        elGetById('webradiodbCheckState').textContent = tn('Empty uri');
    }
    btnWaitingId('btnCheckWebradiodb', false);
}

/**
 * Compares the local webradio favorite with the entry from webradioDB
 * @returns {boolean} true if entries are equal, else false
 */
function compareWebradioDb() {
    let v1 = '';
    let v2 = '';
    const webradio = streamUriToName(elGetById('editRadioFavoriteStreamUri').value) + '.m3u';
    for (const v of ['Name', 'StreamUri', 'Genre', 'Homepage', 'Image', 'Country', 'Language', 'Description', 'Codec', 'Bitrate']) {
        if (v === 'Image') {
            v1 += basename(elGetById('editRadioFavorite' + v).value, false);
        }
        else {
            v1 += elGetById('editRadioFavorite' + v).value;
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
    const webradio = streamUriToName(elGetById('editRadioFavoriteStreamUri').value) + '.m3u';
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
        '&title=' + encodeURIComponent('[Add Webradio]: ' + elGetById('editRadioFavoriteName').value) +
        '&name=' + encodeURIComponent(elGetById('editRadioFavoriteName').value) +
        '&streamuri=' + encodeURIComponent(elGetById('editRadioFavoriteStreamUri').value) +
        '&genre=' + encodeURIComponent(elGetById('editRadioFavoriteGenre').value) +
        '&homepage=' + encodeURIComponent(elGetById('editRadioFavoriteHomepage').value) +
        '&image=' + encodeURIComponent(elGetById('editRadioFavoriteImage').value) +
        '&country=' + encodeURIComponent(elGetById('editRadioFavoriteCountry').value) +
        '&language=' + encodeURIComponent(elGetById('editRadioFavoriteLanguage').value) +
        '&codec=' + encodeURIComponent(elGetById('editRadioFavoriteCodec').value) +
        '&bitrate=' + encodeURIComponent(elGetById('editRadioFavoriteBitrate').value) +
        '&description=' + encodeURIComponent(elGetById('editRadioFavoriteDescription').value);
    window.open(uri, '_blank');
}

/**
 * Updates the webradioDB entry from the local webradio favorite
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function updateWebradioDb() {
    const uri = 'https://github.com/jcorporation/webradiodb/issues/new?labels=ModifyWebradio&template=modify-webradio.yml' +
        '&modifyWebradio='  + encodeURIComponent(elGetById('editRadioFavoriteStreamUriOld').value) +
        '&title=' + encodeURIComponent('[Modify Webradio]: ' + elGetById('editRadioFavoriteName').value) +
        '&name=' + encodeURIComponent(elGetById('editRadioFavoriteName').value) +
        '&streamuri=' + encodeURIComponent(elGetById('editRadioFavoriteStreamUri').value) +
        '&genre=' + encodeURIComponent(elGetById('editRadioFavoriteGenre').value) +
        '&homepage=' + encodeURIComponent(elGetById('editRadioFavoriteHomepage').value) +
        '&image=' + encodeURIComponent(elGetById('editRadioFavoriteImage').value) +
        '&country=' + encodeURIComponent(elGetById('editRadioFavoriteCountry').value) +
        '&language=' + encodeURIComponent(elGetById('editRadioFavoriteLanguage').value) +
        '&codec=' + encodeURIComponent(elGetById('editRadioFavoriteCodec').value) +
        '&bitrate=' + encodeURIComponent(elGetById('editRadioFavoriteBitrate').value) +
        '&description=' + encodeURIComponent(elGetById('editRadioFavoriteDescription').value);
    window.open(uri, '_blank');
}

/**
 * Handler for the MYMPD_API_WEBRADIO_FAVORITE_SAVE jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function saveRadioFavoriteClose(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        uiElements.modalRadioFavoriteEdit.hide();
        if (app.id === 'BrowseRadioFavorites') {
            getRadioFavoriteList();
        }
    }
}
