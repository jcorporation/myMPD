"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module radioFavorites_js */

/**
 * Constructs a special webradio favorite uri.
 * This uri is served by myMPD.
 * @param {string} filename base uri
 * @returns {string} constructed uri
 */
function getRadioFavoriteUri(filename) {
    //construct special url, it will be resolved by the myMPD api handler
    return 'mympd://webradio/' + myEncodeURI(filename);
}

/**
 * Constructs special webradio favorite uris.
 * This uris are served by myMPD.
 * @param {Array} uris array of base uris
 * @returns {Array} modified array with uris
 */
function getRadioFavoriteUris(uris) {
    for (let i = 0, j = uris.length; i < j; i++) {
        uris[i] = getRadioFavoriteUri(uris[i]);
    }
    return uris;
}

/**
 * Deletes a webradio favorite
 * @param {Array} filenames filenames to delete
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function deleteRadioFavorites(filenames) {
    sendAPI("MYMPD_API_WEBRADIO_FAVORITE_RM", {
        "filenames": filenames
    }, function() {
        getRadioFavoriteList();
    }, false);
}

/**
 * Gets the webradio favorite and opens the edit modal
 * @param {string} filename filename to get
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function editRadioFavorite(filename) {
    sendAPI("MYMPD_API_WEBRADIO_FAVORITE_GET", {
        "filename": filename
    }, function(obj) {
        showEditRadioFavorite(obj.result);
    }, false);
}
