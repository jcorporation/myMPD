"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module BrowseRadioWebradiodb_js */

/**
 * WebradioDB Browse handler
 * @returns {void}
 */
function handleBrowseRadioWebradiodb() {
    setFocusId('BrowseRadioWebradiodbSearchStr');
    if (webradioDb === null) {
        //fetch webradiodb database
        getWebradiodb();
        return;
    }
    setDataId('filterWebradiodbGenre', 'value', app.current.filter['genre']);
    document.getElementById('filterWebradiodbGenre').value = app.current.filter['genre'];
    setDataId('filterWebradiodbCountry', 'value', app.current.filter['country']);
    document.getElementById('filterWebradiodbCountry').value = app.current.filter['country'];
    setDataId('filterWebradiodbLanguage', 'value', app.current.filter['language']);
    document.getElementById('filterWebradiodbLanguage').value = app.current.filter['language'];
    setDataId('filterWebradiodbCodec', 'value', app.current.filter['codec']);
    document.getElementById('filterWebradiodbCodec').value = app.current.filter['codec'];
    setDataId('filterWebradiodbBitrate', 'value', app.current.filter['bitrate']);
    document.getElementById('filterWebradiodbBitrate').value = app.current.filter['bitrate'];

    const result = searchWebradiodb(app.current.search, app.current.filter['genre'],
        app.current.filter['country'], app.current.filter['language'], app.current.filter['codec'],
        app.current.filter['bitrate'], app.current.sort, app.current.offset, app.current.limit);
    parseSearchWebradiodb(result);
}

/**
 * Initialization function for webradioDB elements
 */
function initBrowseRadioWebradiodb() {
    document.getElementById('BrowseRadioWebradiodbSearchStr').addEventListener('keyup', function(event) {
        clearSearchTimer();
        if (event.key === 'Escape') {
            this.blur();
        }
        else {
            searchTimer = setTimeout(function() {
                doSearchWebradiodb();
            }, searchTimerTimeout);
        }
    }, false);

    document.getElementById('BrowseRadioWebradiodbFilter').addEventListener('show.bs.collapse', function() {
        document.getElementById('BrowseRadioWebradiodbFilterBtn').classList.add('active');
    }, false);

    document.getElementById('BrowseRadioWebradiodbFilter').addEventListener('hide.bs.collapse', function() {
        document.getElementById('BrowseRadioWebradiodbFilterBtn').classList.remove('active');
    }, false);

    initWebradiodbFilter('filterWebradiodbGenre', 'webradioGenres', 'Genre');
    initWebradiodbFilter('filterWebradiodbCountry', 'webradioCountries', 'Country');
    initWebradiodbFilter('filterWebradiodbLanguage', 'webradioLanguages', 'Language');
    initWebradiodbFilter('filterWebradiodbCodec', 'webradioCodecs', 'Codec');
    initWebradiodbFilter('filterWebradiodbBitrate', 'webradioBitrates', 'Bitrate');

    document.querySelector('#BrowseRadioWebradiodbList > thead > tr').addEventListener('click', function(event) {
        const colName = event.target.getAttribute('data-col');
        toggleSort(event.target, colName);
        appGoto(app.current.card, app.current.tab, app.current.view,
            app.current.offset, app.current.limit, app.current.filter, app.current.sort, '-', app.current.search);
    }, false);

    document.getElementById('BrowseRadioWebradiodbList').addEventListener('click', function(event) {
        if (event.target.nodeName === 'TD') {
            const uri = getData(event.target.parentNode, 'uri');
            if (settings.webuiSettings.clickRadiobrowser === 'add') {
                showEditRadioFavorite({
                    "Name": getData(event.target.parentNode, 'name'),
                    "Genre": getData(event.target.parentNode, 'genre'),
                    "Image": getData(event.target.parentNode, 'image'),
                    "StreamUri": uri
                });
            }
            else {
                clickWebradiodb(uri);
            }
        }
        else if (event.target.nodeName === 'A') {
            //action td
            handleActionTdClick(event);
        }
    }, false);
}

/**
 * Initializes the webradioDB filter elements
 * @param {string} id input id to initialize
 * @param {string} dbField database fields
 * @param {string} name name of the field
 */
function initWebradiodbFilter(id, dbField, name) {
    document.getElementById(id).addEventListener('change', function() {
        doSearchWebradiodb();
    }, false);
    setDataId(id, 'cb-filter', [id]);
    setDataId(id, 'cb-filter-options', [id, dbField, name]);
}

/**
 * Fetches the webradioDB
 */
function getWebradiodb() {
    const list = document.querySelector('#BrowseRadioWebradiodbList > tbody');
    elReplaceChild(list, 
        loadingRow(settings.colsBrowseRadioWebradiodb.length + 1)
    );
    sendAPI("MYMPD_API_CLOUD_WEBRADIODB_COMBINED_GET", {}, function(obj) {
        webradioDb = obj.result.data;
        filterWebradiodbFilter('filterWebradiodbGenre', 'webradioGenres', 'Genre', '');
        filterWebradiodbFilter('filterWebradiodbCountry', 'webradioCountries', 'Country', '');
        filterWebradiodbFilter('filterWebradiodbLanguage', 'webradioLanguages', 'Language', '');
        filterWebradiodbFilter('filterWebradiodbCodec', 'webradioCodecs', 'Codec', '');
        filterWebradiodbFilter('filterWebradiodbBitrate', 'webradioBitrates', 'Bitrate', '');
        const result = searchWebradiodb(app.current.search, app.current.filter['genre'],
            app.current.filter['country'], app.current.filter['language'], app.current.filter['codec'],
            app.current.filter['bitrate'], app.current.sort, app.current.offset, app.current.limit);
        parseSearchWebradiodb(result);
    }, false);
}

/**
 * Callback function for the custom select filter for webradioDB
 * @param {string} id element id
 * @param {string} dbField name of the array to filter
 * @param {string} placeholder placeholder value
 * @param {string} searchStr search string
 */
function filterWebradiodbFilter(id, dbField, placeholder, searchStr) {
    searchStr = searchStr.toLowerCase();
    const el = document.getElementById(id);
    elClear(el.filterResult);
    el.addFilterResult(tn(placeholder), '');
    let i = 0;
    for (const value of webradioDb[dbField]) {
        if (searchStr === '' ||
            value.toLowerCase().indexOf(searchStr) > -1)
        {
            el.addFilterResult(value, value);
            i++;
        }
        if (i === 50) {
            break;
        }
    }
}

/**
 * Starts the webradioDB search
 */
function doSearchWebradiodb() {
    const searchstr = document.getElementById('BrowseRadioWebradiodbSearchStr').value;
    const genre = getDataId('filterWebradiodbGenre', 'value');
    const country = getDataId('filterWebradiodbCountry', 'value');
    const language = getDataId('filterWebradiodbLanguage', 'value');
    const codec = getDataId('filterWebradiodbCodec', 'value');
    const bitrate = getDataId('filterWebradiodbBitrate', 'value');
    appGoto('Browse', 'Radio', 'Webradiodb',
        0, app.current.limit, {"genre": genre, "country": country, "language": language, "codec": codec, "bitrate": bitrate},
        app.current.sort, undefined, searchstr, 0);
}

/**
 * Searches the webradioDB
 * @param {string} name webradio name
 * @param {string} genre webradio genre
 * @param {string} country webradio country
 * @param {string} language webradio language
 * @param {string} codec webradio codec
 * @param {number} bitrate webradio bitrate
 * @param {object} sort webradio sort
 * @param {number} offset start offset
 * @param {number} limit maximum number of results
 * @returns {object} the search result
 */
function searchWebradiodb(name, genre, country, language, codec, bitrate, sort, offset, limit) {
    name = name.toLowerCase();
    const obj = {
        "result": {
            "totalEntities": 0,
            "returnedEntities": 0,
            "data": []
        }
    };
    if (webradioDb === null) {
        logDebug('WebradioDb is empty');
        return obj;
    }

    for (const key in webradioDb.webradios) {
        if (webradioDb.webradios[key].Name.toLowerCase().indexOf(name) > -1 &&
            (genre === '' || webradioDb.webradios[key].Genre.includes(genre)) &&
            (country === '' || country === webradioDb.webradios[key].Country) &&
            (language === '' || language === webradioDb.webradios[key].Language) &&
            (codec === '' || webradioDb.webradios[key].allCodecs.includes(codec)) &&
            (bitrate === 0 || bitrate <= webradioDb.webradios[key].highestBitrate)
        ) {
            obj.result.data.push(webradioDb.webradios[key]);
            obj.result.totalEntities++;
        }
    }
    obj.result.data.sort(function(a, b) {
        //case insensitive sorting
        let lca;
        let lcb;
        if (typeof a === 'string') {
            lca = a[sort.tag].toLowerCase();
            lcb = b[sort.tag].toLowerCase();
        }
        else {
            lca = a[sort.tag];
            lcb = b[sort.tag];
        }
        //primary sort by defined tag
        if (lca < lcb) {
            return sort.desc === false ? -1 : 1;
        }
        if (lca > lcb) {
            return sort.desc === false ? 1 : -1;
        }
        //secondary sort by Name
        if (sort.tag !== 'Name') {
            lca = a.Name.toLowerCase();
            lcb = b.Name.toLowerCase();
            if (lca < lcb) {
                return sort.desc === false ? -1 : 1;
            }
            if (lca > lcb) {
                return sort.desc === false ? 1 : -1;
            }
        }
        //equal
        return 0;
    });
    if (offset > 0) {
        obj.result.data.splice(0, offset - 1);
    }
    const last = obj.result.data.length - limit;
    if (last > 0) {
        obj.result.data.splice(limit, last);
    }
    obj.result.returnedEntities = obj.result.data.length;
    return obj;
}

/**
 * Parses the webradioDB search result
 * @param {object} obj the search result
 * @returns {void}
 */
function parseSearchWebradiodb(obj) {
    const tfoot = document.querySelector('#BrowseRadioWebradiodbList > tfoot');
    elClear(tfoot);

    if (app.current.filter['genre'] === '' &&
        app.current.filter['country'] === '' &&
        app.current.filter['language'] === '')
    {
        document.getElementById('BrowseRadioWebradiodbFilterBtn').textContent = 'filter_list_off';
    }
    else {
        document.getElementById('BrowseRadioWebradiodbFilterBtn').textContent = 'filter_list';
    }

    if (checkResultId(obj, 'BrowseRadioWebradiodbList') === false) {
        return;
    }

    const rowTitle = tn(webuiSettingsDefault.clickRadiobrowser.validValues[settings.webuiSettings.clickRadiobrowser]);

    updateTable(obj, 'BrowseRadioWebradiodb', function(row, data) {
        setData(row, 'uri', data.StreamUri);
        setData(row, 'name', data.Name);
        setData(row, 'genre', data.Genre);
        setData(row, 'image', webradioDbPicsUri + data.Image);
        setData(row, 'homepage', data.Homepage);
        setData(row, 'country', data.Country);
        setData(row, 'language', data.Language);
        setData(row, 'description', data.Description);
        setData(row, 'codec', data.Codec);
        setData(row, 'bitrate', data.Bitrate);
        setData(row, 'type', 'stream');
        row.setAttribute('title', rowTitle);
    });

    if (obj.result.totalEntities > 0) {
        const colspan = settings.colsBrowseRadioWebradiodb.length + 1;
        tfoot.appendChild(
            elCreateNode('tr', {},
                elCreateTextTnNr('td', {"colspan": colspan}, 'Num entries', obj.result.totalEntities)
            )
        );
    }
}

/**
 * Converts a stream uri to the webradioDB and webradio favorites filename
 * @param {string} uri uri to convert
 * @returns {string} converted string
 */
function streamUriToName(uri) {
    return uri.replace(/[<>/.:?&$!#|;=]/g, '_');
}

/**
 * Shows the details of a webradioDB entry
 * @param {string} uri webradio uri
 */
//eslint-disable-next-line no-unused-vars
function showWebradiodbDetails(uri) {
    //reuse the radiobrowser modal
    const tbody = document.getElementById('modalRadiobrowserDetailsList');
    elClearId('modalRadiobrowserDetailsList');
    const m3u = isStreamUri(uri) ? streamUriToName(uri) + '.m3u' : uri;
    const result = webradioDb.webradios[m3u];
    if (result.Image !== '') {
        document.getElementById('RadiobrowserDetailsImage').style.backgroundImage = getCssImageUri(webradioDbPicsUri + result.Image);
    }
    else {
        document.getElementById('RadiobrowserDetailsImage').style.backgroundImage =
            'url("' + subdir + '/assets/coverimage-notavailable.svg")';
    }
    document.getElementById('RadiobrowserDetailsTitle').textContent = result.Name;
    setDataId('RadiobrowserDetailsTitle', 'webradio', result);
    const showFields = [
        'StreamUri',
        'Homepage',
        'Genre',
        'Country',
        'Language',
        'Codec',
        'Bitrate',
        'Description'
    ];
    for (const field of showFields) {
        const value = printValue(field, result[field]);
        tbody.appendChild(
            elCreateNodes('tr', {}, [
                elCreateTextTn('th', {}, field),
                elCreateNode('td', {}, value)
            ])
        );
    }
    const alternateStreams = Object.keys(result.alternativeStreams);
    if (alternateStreams.length > 0) {
        const td = elCreateEmpty('td', {});
        for (const name of alternateStreams) {
            const p = elCreateTextTn('p', {"class": ["pb-0"]}, 'Webradioformat',
                {"codec": result.alternativeStreams[name].Codec, "bitrate": result.alternativeStreams[name].Bitrate});
            const btn = elCreateText('button', {"class": ["btn", "btn-sm", "btn-secondary", "mi", "mi-small", "ms-2"]}, 'favorite');
            p.appendChild(btn);
            td.appendChild(p);
            btn.addEventListener('click', function(event) {
                event.preventDefault();
                showEditRadioFavorite({
                    "Name": result.Name,
                    "StreamUri": result.alternativeStreams[name].StreamUri,
                    "Genre": result.Genre,
                    "Homepage": result.Homepage,
                    "Country": result.Country,
                    "Language": result.Language,
                    "Codec": result.alternativeStreams[name].Codec,
                    "Bitrate": result.alternativeStreams[name].Bitrate,
                    "Description": result.Description,
                    "Image": result.Image
                });
            }, false);
        }
        tbody.appendChild(
            elCreateNodes('tr', {}, [
                elCreateTextTn('th', {}, 'Alternative streams'),
                td
            ])
        );
    }
    uiElements.modalRadiobrowserDetails.show();
}
