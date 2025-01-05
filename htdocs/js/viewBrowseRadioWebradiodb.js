"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module viewBrowseRadioWebradiodb_js */

/**
 * WebradioDB Browse handler
 * @returns {void}
 */
function handleBrowseRadioWebradiodb() {
    handleSearchExpression('BrowseRadioWebradiodb');
    toggleBtnChkId('BrowseRadioWebradiodbSortDesc', app.current.sort.desc);
    selectTag('BrowseRadioWebradiodbSortTagsList', undefined, app.current.sort.tag);
    const searchMatchEl = elGetById(app.id + 'SearchMatch');

    sendAPI("MYMPD_API_WEBRADIODB_SEARCH", {
        "offset": app.current.offset,
        "limit": app.current.limit,
        "expression": app.current.search,
        "sort": app.current.sort.tag,
        "sortdesc": app.current.sort.desc
    }, parseSearchWebradiodb, true);

    if (app.current.filter === 'Bitrate') {
        elShowId(app.id + 'SearchBitrateMatch');
        searchMatchEl.value = '>=';
    }
    else {
        if (getSelectValue(searchMatchEl) === '>=') {
            searchMatchEl.value = 'contains';
        }
        elHideId(app.id + 'SearchBitrateMatch');
    }
}

/**
 * Initialization function for webradioDB elements
 * @returns {void}
 */
function initViewBrowseRadioWebradiodb() {
    initSearchExpression('BrowseRadioWebradiodb');
    initSortBtns('BrowseRadioWebradiodb');
    setView('BrowseRadioWebradiodb');
}

/**
 * Click event handler for WebradioDB list
 * @param {MouseEvent} event click event
 * @param {HTMLElement} target calculated target
 * @returns {void}
 */
function viewBrowseRadioWebradiodbListClickHandler(event, target) {
    const uri = getData(target, 'uri');
    if (settings.webuiSettings.clickWebradiodb === 'add') {
        showEditRadioFavorite({
            "Name": getData(target, 'name'),
            "Genre": getData(target, 'genre'),
            "Image": getData(target, 'image'),
            "StreamUri": uri
        });
    }
    else {
        clickWebradiodb(uri, event);
    }
}

/**
 * Parses the webradioDB search result
 * @param {object} obj the search result
 * @returns {void}
 */
function parseSearchWebradiodb(obj) {
    const table = elGetById('BrowseRadioWebradiodbList');

    if (checkResult(obj, table, undefined) === false) {
        return;
    }

    if (settings['view' + app.id].mode === 'table') {
        const tfoot = table.querySelector('tfoot');
        elClear(tfoot);
        updateTable(obj, app.id, function(row, data) {
            parseSearchWebradiodbUpdate(row, data);
        });

        if (obj.result.totalEntities > 0) {
            addTblFooter(tfoot,
                elCreateTextTnNr('span', {}, 'Num entries', obj.result.totalEntities)
            );
        }
        return;
    }
    if (settings['view' + app.id].mode === 'grid') {
        updateGrid(obj, app.id, function(card, data) {
            parseSearchWebradiodbUpdate(card, data);
        });
        return;
    }
    updateList(obj, app.id, function(card, data) {
        parseSearchWebradiodbUpdate(card, data);
    });
}

/**
 * Callback function for row or card
 * @param {HTMLElement} card Row or card
 * @param {object} data Data object
 * @returns {void}
 */
function parseSearchWebradiodbUpdate(card, data) {
    const rowTitle = tn(settingsWebuiFields.clickWebradiodb.validValues[settings.webuiSettings.clickWebradiodb]);
    setData(card, 'uri', data.StreamUri);
    setData(card, 'name', data.Name);
    setData(card, 'image', data.Image);
    setData(card, 'type', 'webradio');
    card.setAttribute('title', rowTitle);
}
