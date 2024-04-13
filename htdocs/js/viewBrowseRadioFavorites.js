"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module viewBrowseRadioFavorites_js */

/**
 * Browse RadioFavorites handler
 * @returns {void}
 */
function handleBrowseRadioFavorites() {
    handleSearchSimple('BrowseRadioFavorites');
    getRadioFavoriteList();
}

/**
 * Initialization function for radio favorites elements
 * @returns {void}
 */
function initViewBrowseRadioFavorites() {
    initSearchSimple('BrowseRadioFavorites');
    setView('BrowseRadioFavorites');
}

/**
 * Click event handler for radio favorites list
 * @param {MouseEvent} event click event
 * @param {HTMLElement} target calculated target
 * @returns {void}
 */
function viewBrowseRadioFavoritesListClickHandler(event, target) {
    const uri = getData(target, 'uri');
    clickRadioFavorites(uri, event);
}

/**
 * Gets the list of webradio favorites
 * @returns {void}
 */
function getRadioFavoriteList() {
    sendAPI("MYMPD_API_WEBRADIO_FAVORITE_LIST", {
        "offset": app.current.offset,
        "limit": app.current.limit,
        "searchstr": app.current.search
    }, parseRadioFavoritesList, true);
}

/**
 * Parses the jsonrpc response from MYMPD_API_WEBRADIO_FAVORITE_LIST
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parseRadioFavoritesList(obj) {
    const table = elGetById('BrowseRadioFavoritesList');
    if (checkResult(obj, table, undefined) === false) {
        return;
    }

    if (settings['view' + app.id].mode === 'table') {
        const rowTitle = tn(settingsWebuiFields.clickRadioFavorites.validValues[settings.webuiSettings.clickRadioFavorites]);
        updateTable(obj, app.id, function(row, data) {
            if (data.Image === '') {
                data.Image = '/assets/coverimage-stream';
            }
            data.Thumbnail = getCssImageUri(data.Image);
            setData(row, 'uri', data.filename);
            setData(row, 'name', data.Name);
            setData(row, 'type', 'webradio');
            setData(row, 'image', data.Image);
            row.setAttribute('title', rowTitle);
        });
        return;
    }
    updateGrid(obj, app.id, function(card, data) {
        if (data.Image === '') {
            data.Image = '/assets/coverimage-stream';
        }
        data.Thumbnail = getCssImageUri(data.Image);
        setData(card, 'uri', data.filename);
        setData(card, 'name', data.Name);
        setData(card, 'type', 'webradio');
        setData(card, 'image', data.Image);
    });
}
