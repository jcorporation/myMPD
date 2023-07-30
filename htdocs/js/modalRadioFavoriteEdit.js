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
    document.getElementById('editRadioFavoriteName').value = obj.Name === undefined ? '' : obj.Name;
    document.getElementById('editRadioFavoriteStreamUri').value = obj.StreamUri === undefined ? '' : obj.StreamUri;
    document.getElementById('editRadioFavoriteStreamUriOld').value = obj.StreamUri === undefined ? '' : obj.StreamUri;
    document.getElementById('editRadioFavoriteGenre').value = obj.Genre === undefined ? '' : obj.Genre;
    document.getElementById('editRadioFavoriteHomepage').value = obj.Homepage === undefined ? '' : obj.Homepage;
    document.getElementById('editRadioFavoriteCountry').value = obj.Country === undefined ? '' : obj.Country;
    document.getElementById('editRadioFavoriteLanguage').value = obj.Language === undefined ? '' : obj.Language;
    document.getElementById('editRadioFavoriteCodec').value = obj.Codec === undefined ? '' : obj.Codec;
    document.getElementById('editRadioFavoriteBitrate').value = obj.Bitrate === undefined ? '' : obj.Bitrate;
    document.getElementById('editRadioFavoriteDescription').value = obj.Description === undefined ? '' : obj.Description;

    const imageEl = document.getElementById('editRadioFavoriteImage');
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
        "name": document.getElementById('editRadioFavoriteName').value,
        "streamUri": document.getElementById('editRadioFavoriteStreamUri').value,
        "streamUriOld": document.getElementById('editRadioFavoriteStreamUriOld').value,
        "genre": document.getElementById('editRadioFavoriteGenre').value,
        "image": document.getElementById('editRadioFavoriteImage').value,
        "homepage": document.getElementById('editRadioFavoriteHomepage').value,
        "country": document.getElementById('editRadioFavoriteCountry').value,
        "language": document.getElementById('editRadioFavoriteLanguage').value,
        "codec": document.getElementById('editRadioFavoriteCodec').value,
        "bitrate": Number(document.getElementById('editRadioFavoriteBitrate').value),
        "description": document.getElementById('editRadioFavoriteDescription').value,
    }, saveRadioFavoriteClose, true);
}

/**
 * Wrapper for _checkWebradioDb that fetches the webradioDB if needed
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function checkWebradioDb() {
    document.getElementById('webradiodbCheckState').textContent = tn('Checking...');
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
    const streamUri = document.getElementById('editRadioFavoriteStreamUri').value;
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
                document.getElementById('webradiodbCheckState').textContent = tn('Uri not found in WebradioDB');
            }
            else {
                elHideId('btnAddToWebradiodb');
                elHideId('btnUpdateWebradiodb');
                elHideId('btnUpdateFromWebradiodb');
                document.getElementById('webradiodbCheckState').textContent = tn('Alternative stream uri');
            }
        }
        else {
            elHideId('btnAddToWebradiodb');
            if (compareWebradioDb() === false) {
                elShowId('btnUpdateWebradiodb');
                elShowId('btnUpdateFromWebradiodb');
                elHideId('btnCheckWebradiodb');
                document.getElementById('webradiodbCheckState').textContent = tn('Favorite and WebradioDb entry are different');
            }
            else {
                elHideId('btnUpdateWebradiodb');
                elHideId('btnUpdateFromWebradiodb');
                elShowId('btnCheckWebradiodb');
                document.getElementById('webradiodbCheckState').textContent = tn('Favorite is uptodate');
            }
        }
    }
    else {
        elHideId('btnAddToWebradiodb');
        elHideId('btnUpdateWebradiodb');
        elHideId('btnUpdateFromWebradiodb');
        elShowId('btnCheckWebradiodb');
        document.getElementById('webradiodbCheckState').textContent = tn('Empty uri');
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
    const webradio = streamUriToName(document.getElementById('editRadioFavoriteStreamUri').value) + '.m3u';
    for (const v of ['Name', 'StreamUri', 'Genre', 'Homepage', 'Image', 'Country', 'Language', 'Description', 'Codec', 'Bitrate']) {
        if (v === 'Image') {
            v1 += basename(document.getElementById('editRadioFavorite' + v).value, false);
        }
        else {
            v1 += document.getElementById('editRadioFavorite' + v).value;
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
    const webradio = streamUriToName(document.getElementById('editRadioFavoriteStreamUri').value) + '.m3u';
    for (const v of ['Name', 'StreamUri', 'Genre', 'Homepage', 'Image', 'Country', 'Language', 'Description', 'Codec', 'Bitrate']) {
        if (v === 'Image') {
            document.getElementById('editRadioFavorite' + v).value = webradioDbPicsUri + webradioDb.webradios[webradio][v];
        }
        else {
            document.getElementById('editRadioFavorite' + v).value = webradioDb.webradios[webradio][v];
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
        '&title=' + encodeURIComponent('[Add Webradio]: ' + document.getElementById('editRadioFavoriteName').value) +
        '&name=' + encodeURIComponent(document.getElementById('editRadioFavoriteName').value) +
        '&streamuri=' + encodeURIComponent(document.getElementById('editRadioFavoriteStreamUri').value) +
        '&genre=' + encodeURIComponent(document.getElementById('editRadioFavoriteGenre').value) +
        '&homepage=' + encodeURIComponent(document.getElementById('editRadioFavoriteHomepage').value) +
        '&image=' + encodeURIComponent(document.getElementById('editRadioFavoriteImage').value) +
        '&country=' + encodeURIComponent(document.getElementById('editRadioFavoriteCountry').value) +
        '&language=' + encodeURIComponent(document.getElementById('editRadioFavoriteLanguage').value) +
        '&codec=' + encodeURIComponent(document.getElementById('editRadioFavoriteCodec').value) +
        '&bitrate=' + encodeURIComponent(document.getElementById('editRadioFavoriteBitrate').value) +
        '&description=' + encodeURIComponent(document.getElementById('editRadioFavoriteDescription').value);
    window.open(uri, '_blank');
}

/**
 * Updates the webradioDB entry from the local webradio favorite
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function updateWebradioDb() {
    const uri = 'https://github.com/jcorporation/webradiodb/issues/new?labels=ModifyWebradio&template=modify-webradio.yml' +
        '&modifyWebradio='  + encodeURIComponent(document.getElementById('editRadioFavoriteStreamUriOld').value) +
        '&title=' + encodeURIComponent('[Modify Webradio]: ' + document.getElementById('editRadioFavoriteName').value) +
        '&name=' + encodeURIComponent(document.getElementById('editRadioFavoriteName').value) +
        '&streamuri=' + encodeURIComponent(document.getElementById('editRadioFavoriteStreamUri').value) +
        '&genre=' + encodeURIComponent(document.getElementById('editRadioFavoriteGenre').value) +
        '&homepage=' + encodeURIComponent(document.getElementById('editRadioFavoriteHomepage').value) +
        '&image=' + encodeURIComponent(document.getElementById('editRadioFavoriteImage').value) +
        '&country=' + encodeURIComponent(document.getElementById('editRadioFavoriteCountry').value) +
        '&language=' + encodeURIComponent(document.getElementById('editRadioFavoriteLanguage').value) +
        '&codec=' + encodeURIComponent(document.getElementById('editRadioFavoriteCodec').value) +
        '&bitrate=' + encodeURIComponent(document.getElementById('editRadioFavoriteBitrate').value) +
        '&description=' + encodeURIComponent(document.getElementById('editRadioFavoriteDescription').value);
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
