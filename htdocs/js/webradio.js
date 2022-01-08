"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

const radiobrowserTags = [
    {'key': 'name', 'desc': 'Name'},
    {'key': 'country', 'desc': 'Country'},
    {'key': 'tag', 'desc': 'Tags'}
];

function initWebradio() {
    const stack = elCreateEmpty('div', {"class": ["d-grid", "gap-2"]});
    for (const tag of radiobrowserTags) {
        stack.appendChild(elCreateText('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": tag.key}, tn(tag.desc)));
    }
    stack.firstElementChild.classList.add('active');
    document.getElementById('BrowseRadioRadiobrowserTags').appendChild(stack);

    document.getElementById('BrowseRadioRadiobrowserSearchStr').addEventListener('keyup', function(event) {
        if (event.key === 'Escape') {
            this.blur();
        }
        else if (event.key === 'Enter') {
            appGoto(app.current.card, app.current.tab, app.current.view,
                0, app.current.limit, app.current.filter, '-', '-', this.value);
        }
    }, false);

    document.getElementById('BrowseRadioRadiobrowserList').addEventListener('click', function(event) {
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
                clickRadiobrowser(uri, getData(event.target.parentNode, 'RADIOBROWSERUUID'));
            }
        }
        else if (event.target.nodeName === 'A') {
            showPopover(event);
        }
    }, false);

    document.getElementById('BrowseRadioRadiobrowserTags').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            document.getElementById('BrowseRadioRadiobrowserTagsBtn').Dropdown.hide();
            app.current.filter = getData(event.target, 'tag');
            appGoto(app.current.card, app.current.tab, app.current.view,
                0, app.current.limit, app.current.filter, '-', '-', this.value);
        }
    }, false);

    document.getElementById('BrowseRadioWebradiodbSearchStr').addEventListener('keyup', function(event) {
        if (event.key === 'Escape') {
            this.blur();
        }
        else {
            doSearchWebradioDB();
        }
    }, false);

    document.getElementById('selectWebradiodbGenre').addEventListener('change', function() {
        doSearchWebradioDB();
    }, false);

    document.getElementById('selectWebradiodbCountry').addEventListener('change', function() {
        doSearchWebradioDB();
    }, false);

    document.getElementById('selectWebradiodbLanguage').addEventListener('change', function() {
        doSearchWebradioDB();
    }, false);

    document.getElementById('BrowseRadioWebradiodbList').getElementsByTagName('tr')[0].addEventListener('click', function(event) {
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
            showPopover(event);
        }
    }, false);

    document.getElementById('BrowseRadioFavoritesList').addEventListener('click', function(event) {
        const target = event.target.nodeName === 'SMALL' ? event.target.parentNode : event.target;
        if (target.classList.contains('row')) {
            return;
        }
        if (target.classList.contains('card-body')) {
            const uri = getData(event.target.parentNode, 'uri');
            clickRadioFavorites(uri);
        }
        else if (target.classList.contains('card-footer')) {
            showPopover(event);
        }
    }, false);

    document.getElementById('BrowseRadioFavoritesList').addEventListener('contextmenu', function(event) {
        if (event.target.classList.contains('row') ||
            event.target.parentNode.classList.contains('not-clickable'))
        {
            return;
        }
        showPopover(event);
    }, false);

    document.getElementById('BrowseRadioFavoritesList').addEventListener('long-press', function(event) {
        if (event.target.classList.contains('row') ||
            event.target.parentNode.classList.contains('not-clickable'))
        {
            return;
        }
        showPopover(event);
    }, false);

    document.getElementById('BrowseRadioFavoritesSearchStr').addEventListener('keyup', function(event) {
        if (event.key === 'Escape') {
            this.blur();
        }
        else {
            appGoto(app.current.card, app.current.tab, app.current.view,
                0, app.current.limit, app.current.filter, app.current.sort, '-', this.value);
        }
    }, false);

    setDataId('editRadioFavoriteImage', 'cb-filter', 'filterImageSelect');
    setDataId('editRadioFavoriteImage', 'cb-filter-options', ['editRadioFavoriteImage']);
    //fetch webradiodb database
    getWebradiodb();
}

function getRadioFavoriteUri(uri) {
    //construct special url, it will be resolved by the myMPD api handler
    return 'mympd://webradio/' + myEncodeURI(uri);
}

function getRadioFavoriteList() {
    sendAPI("MYMPD_API_WEBRADIO_FAVORITE_LIST", {
        "offset": app.current.offset,
        "limit": app.current.limit,
        "searchstr": app.current.search
    }, parseRadioFavoritesList, true);
}

//eslint-disable-next-line no-unused-vars
function deleteRadioFavorite(filename) {
    sendAPI("MYMPD_API_WEBRADIO_FAVORITE_RM", {
        "filename": filename
    }, function() {
        getRadioFavoriteList();
    }, false);
}

//eslint-disable-next-line no-unused-vars
function editRadioFavorite(filename) {
    sendAPI("MYMPD_API_WEBRADIO_FAVORITE_GET", {
        "filename": filename
    }, function(obj) {
        showEditRadioFavorite(obj.result);
    }, false);
}

function showEditRadioFavorite(obj) {
    cleanupModalId('modalSaveRadioFavorite');
    document.getElementById('editRadioFavoriteName').value = obj.Name === undefined ? '' : obj.Name;
    document.getElementById('editRadioFavoriteStreamUri').value = obj.StreamUri === undefined ? '' : obj.StreamUri;
    document.getElementById('editRadioFavoriteStreamUriOld').value = obj.StreamUri;
    document.getElementById('editRadioFavoriteGenre').value = obj.Genre === undefined ? '' : obj.Genre;
    document.getElementById('editRadioFavoriteHomepage').value = obj.Homepage === undefined ? '' : obj.Homepage;
    document.getElementById('editRadioFavoriteCountry').value = obj.Country === undefined ? '' : obj.Country;
    document.getElementById('editRadioFavoriteLanguage').value = obj.Language === undefined ? '' : obj.Language;
    document.getElementById('editRadioFavoriteDescription').value = obj.Description === undefined ? '' : obj.Description;
    document.getElementById('editRadioFavoriteRADIOBROWSERUUID').value = obj.RADIOBROWSERUUID === undefined ? '' : obj.RADIOBROWSERUUID;

    const imageEl = document.getElementById('editRadioFavoriteImage');
    getImageList(imageEl.filterResult, obj.Image, [], 'streams');
    imageEl.value = obj.Image === undefined ? '' : obj.Image;

    const webradio = streamUriToName(obj.StreamUri) + '.m3u';
    if (webradioDb !== null && 
        webradioDb.webradios[webradio] === undefined)
    {
        elShowId('btnAddToWebradiodb');
    }
    else {
        elHideId('btnAddToWebradiodb');
    }

    uiElements.modalSaveRadioFavorite.show();
}

//eslint-disable-next-line no-unused-vars
function saveRadioFavorite() {
    cleanupModalId('modalSaveRadioFavorite');
    const uuid = document.getElementById('editRadioFavoriteRADIOBROWSERUUID').value;
    sendAPI("MYMPD_API_WEBRADIO_FAVORITE_SAVE", {
        "name": document.getElementById('editRadioFavoriteName').value,
        "streamUri": document.getElementById('editRadioFavoriteStreamUri').value,
        "streamUriOld": document.getElementById('editRadioFavoriteStreamUriOld').value,
        "genre": document.getElementById('editRadioFavoriteGenre').value,
        "image": document.getElementById('editRadioFavoriteImage').value,
        "homepage": document.getElementById('editRadioFavoriteHomepage').value,
        "country": document.getElementById('editRadioFavoriteCountry').value,
        "language": document.getElementById('editRadioFavoriteLanguage').value,
        "description": document.getElementById('editRadioFavoriteDescription').value,
        "RADIOBROWSERUUID": uuid
    }, saveRadioFavoriteClose, true);
    countClickRadiobrowser(uuid);
}

//eslint-disable-next-line no-unused-vars
function addToWebradioDb() {
    const uri = 'https://github.com/jcorporation/webradiodb/issues/new??labels=labels=AddWebradio&template=add-webradio.yml' +
        '&title=' + encodeURIComponent('[Add Webradio]: ' + document.getElementById('editRadioFavoriteName').value) +
        '&name=' + encodeURIComponent(document.getElementById('editRadioFavoriteName').value) +
        '&streamuri=' + encodeURIComponent(document.getElementById('editRadioFavoriteStreamUri').value) +
        '&genre=' + encodeURIComponent(document.getElementById('editRadioFavoriteGenre').value) +
        '&homepage=' + encodeURIComponent(document.getElementById('editRadioFavoriteHomepage').value) +
        '&image=' + encodeURIComponent(document.getElementById('editRadioFavoriteImage').value) +
        '&country=' + encodeURIComponent(document.getElementById('editRadioFavoriteCountry').value) +
        '&language=' + encodeURIComponent(document.getElementById('editRadioFavoriteLanguage').value) +
        '&description=' + encodeURIComponent(document.getElementById('editRadioFavoriteDescription').value);
    window.open(uri, '_blank');
}

function saveRadioFavoriteClose(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        uiElements.modalSaveRadioFavorite.hide();
        if (app.id === 'BrowseRadioFavorites') {
            getRadioFavoriteList();
        }
    }
}

function parseRadioFavoritesList(obj) {
    const cardContainer = document.getElementById('BrowseRadioFavoritesList');

    const cols = cardContainer.getElementsByClassName('col');
    cardContainer.classList.remove('opacity05');

    if (obj.error !== undefined) {
        elReplaceChild(cardContainer,
            elCreateText('div', {"class": ["col", "not-clickable", "alert", "alert-danger"]}, tn(obj.error.message, obj.error.data))
        );
        setPagination(0, 0);
        return;
    }

    const nrItems = obj.result.returnedEntities;
    if (nrItems === 0) {
        elReplaceChild(cardContainer,
            elCreateText('div', {"class": ["col", "not-clickable", "alert", "alert-secondary"]}, tn('Empty list'))
        );
        setPagination(0, 0);
        return;
    }

    if (cardContainer.getElementsByClassName('not-clickable').length > 0) {
        elClear(cardContainer);
    }
    const rowTitle = tn(webuiSettingsDefault.clickRadioFavorites.validValues[settings.webuiSettings.clickRadioFavorites]);
    for (let i = 0; i < nrItems; i++) {
        //id is used only to check if card should be refreshed
        const id = genId('WebradioFavorite' + obj.result.data[i].filename);

        if (cols[i] !== undefined &&
            cols[i].firstChild.firstChild.getAttribute('id') === id)
        {
            continue;
        }

        const card = elCreateNodes('div', {"data-popover": "webradio", "class": ["card", "card-grid", "clickable"], "tabindex": 0}, [
            elCreateEmpty('div', {"class": ["card-body", "album-cover-loading", "album-cover-grid", "d-flex"], "id": id, "title": rowTitle}),
            elCreateNodes('div', {"class": ["card-footer", "card-footer-grid", "p-2"]}, [
                document.createTextNode(obj.result.data[i].Name),
                elCreateEmpty('br', {}),
                elCreateText('small', {}, obj.result.data[i].Genre),
                elCreateEmpty('br', {}),
                elCreateText('small', {}, obj.result.data[i].Country +
                    smallSpace + nDash + smallSpace + obj.result.data[i].Language)
            ])
        ]);
        let image;
        if (obj.result.data[i].Image === '') {
            image = subdir + '/assets/coverimage-stream.svg';
        }
        else {
            image = isHttpUri(obj.result.data[i].Image) === true ?
                obj.result.data[i].Image :
                subdir + '/browse/pics/streams/' + obj.result.data[i].Image;
        }
        setData(card, 'image', image);
        setData(card, 'uri', obj.result.data[i].filename);
        setData(card, 'name', obj.result.data[i].Name);
        setData(card, 'RADIOBROWSERUUID', obj.result.data[i].RADIOBROWSERUUID === undefined ? '' : obj.result.data[i].RADIOBROWSERUUID);
        setData(card, 'type', 'webradio');

        const col = elCreateNode('div', {"class": ["col", "px-0", "mb-2", "flex-grow-0"]}, card);

        if (i < cols.length) {
            cols[i].replaceWith(col);
        }
        else {
            cardContainer.append(col);
        }

        if (hasIO === true) {
            const options = {
                root: null,
                rootMargin: '0px',
            };
            const observer = new IntersectionObserver(setGridImage, options);
            observer.observe(col);
        }
        else {
            col.firstChild.firstChild.style.backgroundImage = myEncodeURIhost(image);
        }
    }
    for (let i = cols.length - 1; i >= nrItems; i--) {
        cols[i].remove();
    }

    setPagination(obj.result.totalEntities, obj.result.returnedEntities);
    setScrollViewHeight(cardContainer);
}

//webradiodb api

function getWebradiodb() {
    sendAPI("MYMPD_API_CLOUD_WEBRADIODB_COMBINED_GET", {}, function(obj) {
        webradioDb = obj.result.data;
        populateSelectId('selectWebradiodbGenre', webradioDb.webradioGenres, '');
        populateSelectId('selectWebradiodbCountry', webradioDb.webradioCountries, '');
        populateSelectId('selectWebradiodbLanguage', webradioDb.webradioLanguages, '');
    }, false);
}

function doSearchWebradioDB() {
    const searchstr = document.getElementById('BrowseRadioWebradiodbSearchStr').value;
    const genre = getSelectValueId('selectWebradiodbGenre');
    const country = getSelectValueId('selectWebradiodbCountry');
    const language = getSelectValueId('selectWebradiodbLanguage');
    appGoto('Browse', 'Radio', 'Webradiodb',
        0, app.current.limit, {"genre": genre, "country": country, "language": language},
        app.current.sort, undefined, searchstr, 0);
}

function searchWebradioDB(name, genre, country, language, sort, offset, limit) {
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
			(genre === ''    || webradioDb.webradios[key].Genre.includes(genre)) &&
			(country === ''  || country === webradioDb.webradios[key].Country) &&
			(language === '' || language === webradioDb.webradios[key].Language)
		) {
			obj.result.data.push(webradioDb.webradios[key]);
			obj.result.totalEntities++;
		}
	}
	obj.result.data.sort(function(a, b) {
		if (a[sort.tag] < b[sort.tag]) {
			return sort.desc === false ? -1 : 1;
		}
		if (a[sort.tag] > b[sort.tag]) {
            return sort.desc === false ? 1 : -1;
		}
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

function parseSearchWebradioDB(obj) {
    const table = document.getElementById('BrowseRadioWebradiodbList');
    const tfoot = table.getElementsByTagName('tfoot')[0];
    elClear(tfoot);

    if (checkResultId(obj, 'BrowseRadioWebradiodbList') === false) {
        return;
    }

    const rowTitle = tn(webuiSettingsDefault.clickRadiobrowser.validValues[settings.webuiSettings.clickRadiobrowser]);

    updateTable(obj, 'BrowseRadioWebradiodb', function(row, data) {
        setData(row, 'uri', data.StreamUri);
        setData(row, 'name', data.Name);
        setData(row, 'genre', data.Genre);
        setData(row, 'image', webradioDbUri + data.Image);
        setData(row, 'homepage', data.Homepage);
        setData(row, 'country', data.Country);
        setData(row, 'language', data.Language);
        setData(row, 'description', data.Description);
        setData(row, 'type', 'stream');
        row.setAttribute('title', rowTitle);
    });

    if (obj.result.totalEntities > 0) {
        const colspan = settings.colsBrowseRadioWebradiodb.length + 1;
        tfoot.appendChild(
            elCreateNode('tr', {},
                elCreateText('td', {"colspan": colspan}, tn('Num entries', obj.result.totalEntities))
            )
        );
    }
}

function streamUriToName(uri) {
    return uri.replace(/[<>/.:?&$!#|]/g, '_');
}

//eslint-disable-next-line no-unused-vars
function showWebradiodbDetails(uri) {
    const tbody = document.getElementById('modalRadiobrowserDetailsList');
    elClearId('modalRadiobrowserDetailsList');
    const m3u = isStreamUri(uri) ? streamUriToName(uri) + '.m3u' : uri;
    const result = webradioDb.webradios[m3u];
    if (result.Image !== '') {
        document.getElementById('RadiobrowserDetailsImage').style.backgroundImage =
            'url("' + myEncodeURIhost(webradioDbUri + result.Image) + '")' +
            ', url("' + subdir + '/assets/coverimage-loading.svg")';
    }
    else {
        document.getElementById('RadiobrowserDetailsImage').style.backgroundImage =
            'url("' + subdir + '/assets/coverimage-notavailable.svg")';
    }
    document.getElementById('RadiobrowserDetailsTitle').textContent = result.name;
    const showFields = [
        'StreamUri',
        'Homepage',
        'Genre',
        'Country',
        'Language',
        'Description'
    ];
    for (const field of showFields) {
        const value = printValue(field, result[field]);
        tbody.appendChild(
            elCreateNodes('tr', {}, [
                elCreateText('th', {}, tn(field)),
                elCreateNode('td', {}, value)
            ])
        );
    }
    uiElements.modalRadiobrowserDetails.show();
}

//radio-browser.info api

function countClickRadiobrowser(uuid) {
    if (uuid !== '') {
        sendAPI("MYMPD_API_CLOUD_RADIOBROWSER_CLICK_COUNT", {
            "uuid": uuid
        });
    }
}

//eslint-disable-next-line no-unused-vars
function showRadiobrowserDetails(uuid) {
    sendAPI("MYMPD_API_CLOUD_RADIOBROWSER_STATION_DETAIL", {
        "uuid": uuid
    }, parseRadiobrowserDetails, true);
    uiElements.modalRadiobrowserDetails.show();
    elReplaceChildId('modalRadiobrowserDetailsList',
        elCreateNode('tr', {}, 
            elCreateText('td', {"colspan": 2}, tn('Loading...'))
        )
    );
    countClickRadiobrowser(uuid);
}

function parseRadiobrowserDetails(obj) {
    const tbody = document.getElementById('modalRadiobrowserDetailsList');
    if (checkResult(obj, tbody) === false) {
        return;
    }
    elClearId('modalRadiobrowserDetailsList');
    const result = obj.result.data[0];
    if (result.favicon !== '') {
        document.getElementById('RadiobrowserDetailsImage').style.backgroundImage =
            'url("' + myEncodeURIhost(result.favicon) + '")' +
            ', url("' + subdir + '/assets/coverimage-loading.svg")';
    }
    else {
        document.getElementById('RadiobrowserDetailsImage').style.backgroundImage =
            'url("' + subdir + '/assets/coverimage-notavailable.svg")';
    }
    document.getElementById('RadiobrowserDetailsTitle').textContent = result.name;
    const showFields = {
        'url_resolved': 'StreamUri',
        'homepage': 'Homepage',
        'tags': 'Tags',
        'country': 'Country',
        'language': 'Language',
        'votes': 'Votes',
        'lastchangetime': 'Last change time',
        'lastcheckok': 'State',
        'clickcount': 'Click count'
    };
    for (const field in showFields) {
        const value = printValue(field, result[field]);
        tbody.appendChild(
            elCreateNodes('tr', {}, [
                elCreateText('th', {}, tn(showFields[field])),
                elCreateNode('td', {}, value)
            ])
        );
    }
}

function parseRadiobrowserList(obj) {
    if (checkResultId(obj, 'BrowseRadioRadiobrowserList') === false) {
        return;
    }

    const rowTitle = tn(webuiSettingsDefault.clickRadiobrowser.validValues[settings.webuiSettings.clickRadiobrowser]);
    obj.result.returnedEntities = obj.result.data.length;

    updateTable(obj, 'BrowseRadioRadiobrowser', function(row, data) {
        setData(row, 'uri', data.url_resolved);
        setData(row, 'name', data.name);
        setData(row, 'genre', data.tags);
        setData(row, 'image', data.favicon);
        setData(row, 'homepage', data.homepage);
        setData(row, 'country', data.country);
        setData(row, 'language', data.language);
        setData(row, 'RADIOBROWSERUUID', data.stationuuid);
        setData(row, 'type', 'stream');
        row.setAttribute('title', rowTitle);
    });
}
