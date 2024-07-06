"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
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

    const rowTitle = tn(settingsWebuiFields.clickWebradiodb.validValues[settings.webuiSettings.clickWebradiodb]);
    if (settings['view' + app.id].mode === 'table') {
        const tfoot = table.querySelector('tfoot');
        elClear(tfoot);
        updateTable(obj, app.id, function(row, data) {
            setData(row, 'uri', data.StreamUri);
            setData(row, 'name', data.Name);
            setData(row, 'type', 'webradio');
            row.setAttribute('title', rowTitle);
        });

        if (obj.result.totalEntities > 0) {
            addTblFooter(tfoot,
                elCreateTextTnNr('span', {}, 'Num entries', obj.result.totalEntities)
            );
        }
        return;
    }
    updateGrid(obj, app.id, function(card, data) {
        setData(card, 'uri', data.StreamUri);
        setData(card, 'name', data.Name);
        setData(card, 'type', 'webradio');
        card.setAttribute('title', rowTitle);
    });
}

/**
 * Converts a stream uri to the webradioDB and webradio favorites filename
 * @param {string} uri uri to convert
 * @returns {string} converted string
 */
function streamUriToName(uri) {
    return uri.replace(/[<>/.:?&$!#|;=]/g, '_');
}
