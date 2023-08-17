"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module viewBrowseRadioRadiobrowser_js */

/**
 * Browse Radiobrowser handler
 * @returns {void}
 */
function handleBrowseRadioRadiobrowser() {
    setFocusId('BrowseRadioRadiobrowserSearchStr');
    document.getElementById('BrowseRadioRadiobrowserTagsInput').value = app.current.filter['tags'];
    document.getElementById('BrowseRadioRadiobrowserCountryInput').value = app.current.filter['country'];
    document.getElementById('BrowseRadioRadiobrowserLanguageInput').value = app.current.filter['language'];
    if (app.current.search === '') {
        sendAPI("MYMPD_API_CLOUD_RADIOBROWSER_NEWEST", {
            "offset": app.current.offset,
            "limit": app.current.limit,
        }, parseRadiobrowserList, true);
    }
    else {
        sendAPI("MYMPD_API_CLOUD_RADIOBROWSER_SEARCH", {
            "offset": app.current.offset,
            "limit": app.current.limit,
            "tags": app.current.filter['tags'],
            "country": app.current.filter['country'],
            "language": app.current.filter['language'],
            "searchstr": app.current.search
        }, parseRadiobrowserList, true);
    }
}

/**
 * Initializes the radiobrowser elements
 * @returns {void}
 */
function initViewBrowseRadioRadiobrowser() {
    document.getElementById('BrowseRadioRadiobrowserSearchStr').addEventListener('keyup', function(event) {
        if (ignoreKeys(event) === true) {
            return;
        }
        clearSearchTimer();
        searchTimer = setTimeout(function() {
            searchRadiobrowser();
        }, searchTimerTimeout);
    }, false);

    document.getElementById('BrowseRadioRadiobrowserFilter').addEventListener('show.bs.collapse', function() {
        document.getElementById('BrowseRadioRadiobrowserFilterBtn').classList.add('active');
    }, false);

    document.getElementById('BrowseRadioRadiobrowserFilter').addEventListener('hide.bs.collapse', function() {
        document.getElementById('BrowseRadioRadiobrowserFilterBtn').classList.remove('active');
    }, false);

    document.getElementById('BrowseRadioRadiobrowserList').addEventListener('click', function(event) {
        const target = tableClickHandler(event);
        if (target !== null) {
            const uri = getData(target, 'uri');
            if (settings.webuiSettings.clickRadiobrowser === 'add') {
                showEditRadioFavorite({
                    "Name": getData(target, 'name'),
                    "Genre": getData(target, 'genre'),
                    "Image": getData(target, 'image'),
                    "StreamUri": uri
                });
            }
            else {
                clickRadiobrowser(uri, getData(target, 'RADIOBROWSERUUID'), event);
            }
        }
    }, false);
}

/**
 * Sends a click count message to the radiobrowser api
 * @param {string} uuid station uuid
 * @returns {void}
 */
function countClickRadiobrowser(uuid) {
    if (uuid !== '') {
        sendAPI("MYMPD_API_CLOUD_RADIOBROWSER_CLICK_COUNT", {
            "uuid": uuid
        }, null, false);
    }
}

/**
 * Searches the radiobrowser
 * @returns {void}
 */
function searchRadiobrowser() {
    app.current.filter['tags'] = document.getElementById('BrowseRadioRadiobrowserTagsInput').value;
    app.current.filter['country'] = document.getElementById('BrowseRadioRadiobrowserCountryInput').value;
    app.current.filter['language'] = document.getElementById('BrowseRadioRadiobrowserLanguageInput').value;
    appGoto(app.current.card, app.current.tab, app.current.view,
        0, app.current.limit, app.current.filter, '-', '-', document.getElementById('BrowseRadioRadiobrowserSearchStr').value);
}

/**
 * Parses the MYMPD_API_CLOUD_RADIOBROWSER_NEWEST and
 * MYMPD_API_CLOUD_RADIOBROWSER_SEARCH jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parseRadiobrowserList(obj) {
    if (app.current.filter['tags'] === '' &&
        app.current.filter['country'] === '' &&
        app.current.filter['language'] === '')
    {
        document.getElementById('BrowseRadioRadiobrowserFilterBtn').textContent = 'filter_list_off';
    }
    else {
        document.getElementById('BrowseRadioRadiobrowserFilterBtn').textContent = 'filter_list';
    }

    if (checkResultId(obj, 'BrowseRadioRadiobrowserList') === false) {
        return;
    }

    const rowTitle = tn(settingsWebuiFields.clickRadiobrowser.validValues[settings.webuiSettings.clickRadiobrowser]);
    //set result keys for pagination
    obj.result.returnedEntities = obj.result.data.length;
    obj.result.totalEntities = -1;

    updateTable(obj, 'BrowseRadioRadiobrowser', function(row, data) {
        setData(row, 'uri', data.url_resolved);
        setData(row, 'name', data.name);
        setData(row, 'genre', data.tags);
        setData(row, 'image', data.favicon);
        setData(row, 'homepage', data.homepage);
        setData(row, 'country', data.country);
        setData(row, 'language', data.language);
        setData(row, 'codec', data.codec);
        setData(row, 'bitrate', data.bitrate);
        setData(row, 'RADIOBROWSERUUID', data.stationuuid);
        setData(row, 'type', 'stream');
        row.setAttribute('title', rowTitle);
    });
}
