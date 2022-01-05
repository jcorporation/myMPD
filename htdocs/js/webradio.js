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
    document.getElementById('BrowseRadioRadioBrowserTags').appendChild(stack);

    document.getElementById('BrowseRadioRadioBrowserSearchStr').addEventListener('keyup', function(event) {
        if (event.key === 'Escape') {
            this.blur();
        }
        else if (this.value.length >= 3 ||
            this.value.length === 0 ||
            event.key === 'Enter')
        {
            appGoto(app.current.card, app.current.tab, app.current.view,
                0, app.current.limit, app.current.filter, '-', '-', this.value);
        }
    }, false);

    document.getElementById('BrowseRadioRadioBrowserList').addEventListener('click', function(event) {
        if (event.target.nodeName === 'TD') {
            const uri = getData(event.target.parentNode, 'uri');
            if (settings.webuiSettings.clickRadioBrowser === 'add') {
                const name = getData(event.target.parentNode, 'name');
                const genre = getData(event.target.parentNode, 'genre');
                const image = getData(event.target.parentNode, 'image');
                showEditRadioFavorite(name, genre, image, uri);
            }
            else {
                clickRadioBrowser(uri, getData(event.target.parentNode, 'uuid'));
            }
        }
        else if (event.target.nodeName === 'A') {
            showPopover(event);
        }
    }, false);

    document.getElementById('BrowseRadioRadioBrowserTags').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            document.getElementById('BrowseRadioRadioBrowserTagsBtn').Dropdown.hide();
            app.current.filter = getData(event.target, 'tag');
            appGoto(app.current.card, app.current.tab, app.current.view,
                0, app.current.limit, app.current.filter, '-', '-', this.value);
        }
    }, false);

    document.getElementById('BrowseRadioWebradioDbSearchStr').addEventListener('keyup', function(event) {
        if (event.key === 'Escape') {
            this.blur();
        }
        else {
            doSearchWebradioDB();
        }
    }, false);

    document.getElementById('selectWebradioDbGenre').addEventListener('change', function(event) {
        doSearchWebradioDB();
    }, false);

    document.getElementById('selectWebradioDbCountry').addEventListener('change', function(event) {
        doSearchWebradioDB();
    }, false);

    document.getElementById('selectWebradioDbLanguage').addEventListener('change', function(event) {
        doSearchWebradioDB();
    }, false);

    document.getElementById('BrowseRadioWebradioDbList').addEventListener('click', function(event) {
        if (event.target.nodeName === 'TD') {
            const uri = getData(event.target.parentNode, 'uri');
            if (settings.webuiSettings.clickRadioBrowser === 'add') {
                const name = getData(event.target.parentNode, 'name');
                const genre = getData(event.target.parentNode, 'genre');
                const image = getData(event.target.parentNode, 'image');
                showEditRadioFavorite(name, genre, image, uri);
            }
            else {
                clickWebradioDb(uri);
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
        showEditRadioFavorite(obj.result.PLAYLIST, obj.result.EXTGENRE,
            obj.result.EXTIMG, obj.result.streamUri, obj.result.HOMEPAGE,
            obj.result.COUNTRY, obj.result.LANGUAGE, obj.result.RADIOBROWSERUUID);
    }, false);
}

function showEditRadioFavorite(name, genre, image, streamUri, homepage, country, language, uuid) {
    cleanupModalId('modalSaveRadioFavorite');
    document.getElementById('editRadioFavoriteName').value = name;
    document.getElementById('editRadioFavoriteStreamUri').value = streamUri;
    document.getElementById('editRadioFavoriteStreamUriOld').value = streamUri;
    document.getElementById('editRadioFavoriteGenre').value = genre;
    document.getElementById('editRadioFavoriteHomepage').value = homepage;
    document.getElementById('editRadioFavoriteCountry').value = country;
    document.getElementById('editRadioFavoriteLanguage').value = language;
    document.getElementById('editRadioFavoriteUUID').value = uuid === undefined ? '' : uuid;

    const imageEl = document.getElementById('editRadioFavoriteImage');
    getImageList(imageEl.filterResult, image, [], 'streams');
    imageEl.value = image;

    uiElements.modalSaveRadioFavorite.show();
}

//eslint-disable-next-line no-unused-vars
function saveRadioFavorite() {
    cleanupModalId('modalSaveRadioFavorite');
    const uuid = document.getElementById('editRadioFavoriteUUID').value;
    sendAPI("MYMPD_API_WEBRADIO_FAVORITE_SAVE", {
        "name": document.getElementById('editRadioFavoriteName').value,
        "streamUri": document.getElementById('editRadioFavoriteStreamUri').value,
        "streamUriOld": document.getElementById('editRadioFavoriteStreamUriOld').value,
        "genre": document.getElementById('editRadioFavoriteGenre').value,
        "image": document.getElementById('editRadioFavoriteImage').value,
        "homepage": document.getElementById('editRadioFavoriteHomepage').value,
        "country": document.getElementById('editRadioFavoriteCountry').value,
        "language": document.getElementById('editRadioFavoriteLanguage').value,
        "uuid": uuid
    }, saveRadioFavoriteClose, true);
    countClickRadioBrowser(uuid);
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
                document.createTextNode(obj.result.data[i].PLAYLIST),
                elCreateEmpty('br', {}),
                elCreateText('small', {}, obj.result.data[i].EXTGENRE),
                elCreateEmpty('br', {}),
                elCreateText('small', {}, obj.result.data[i].COUNTRY +
                    smallSpace + nDash + smallSpace + obj.result.data[i].LANGUAGE)
            ])
        ]);
        const image = isHttpUri(obj.result.data[i].EXTIMG) === true ?
            obj.result.data[i].EXTIMG :
            subdir + '/browse/pics/streams/' + obj.result.data[i].EXTIMG;
        setData(card, 'image', image);
        setData(card, 'uri', obj.result.data[i].filename);
        setData(card, 'name', obj.result.data[i].PLAYLIST);
        setData(card, 'uuid', obj.result.data[i].RADIOBROWSERUUID === undefined ? '' : obj.result.data[i].RADIOBROWSERUUID);
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
        populateSelectId('selectWebradioDbGenre', webradioDb.webradioGenres, '');
        populateSelectId('selectWebradioDbCountry', webradioDb.webradioCountries, '');
        populateSelectId('selectWebradioDbLanguage', webradioDb.webradioLanguages, '');
    }, false);
}

function doSearchWebradioDB() {
    if (webradioDb === null) {
        setTimeout(function() {
            dosearchWebradioDB()
        }, 1000);
    }
    const searchstr = document.getElementById('BrowseRadioWebradioDbSearchStr').value;
    const genre = getSelectValueId('selectWebradioDbGenre');
    const country = getSelectValueId('selectWebradioDbCountry');
    const language = getSelectValueId('selectWebradioDbLanguage');
    appGoto('Browse', 'Radio', 'WebradioDb',
        0, app.current.limit, {"genre": genre, "country": country, "language": language},
        app.current.sort, undefined, searchstr, 0);
}

function searchWebradioDB(name, genre, country, language, sort) {
	const result = {
		"returnedEntities": 0,
		"data": []
	};
    if (webradioDb === null) {
        logDebug('WebradioDb is empty');
        return result;
    }

	for (const key in webradioDb.webradios) {
		if (webradioDb.webradios[key].PLAYLIST.toLowerCase().indexOf(name) > -1 &&
			(genre === ''    || webradioDb.webradios[key].EXTGENRE.includes(genre)) &&
			(country === ''  || country === webradioDb.webradios[key].COUNTRY) &&
			(language === '' || language === webradioDb.webradios[key].LANGUAGE)
		) {
			result.data.push(webradioDb.webradios[key]);
			result.returnedEntities++;
		}
	}
	result.data.sort(function(a, b) {
		if (a[sort] < b[sort]) {
			return -1;
		}
		if (a[sort] > b[sort]) {
			return 1;
		}
		return 0;
	});
	return result;
}

function parseSearchWebradioDB(result) {
    const table = document.getElementById('BrowseRadioWebradioDbList');
    const tbody = table.getElementsByTagName('tbody')[0];
    setScrollViewHeight(table);

    elClear(tbody);
    const rowTitle = tn(webuiSettingsDefault.clickRadioBrowser.validValues[settings.webuiSettings.clickRadioBrowser]);
    let i = 0;
    const last = app.current.offset + app.current.limit;
    for (const station of result.data) {
        if (i < app.current.offset) {
			i++;
			continue;
		}
		if (i >= last) {
            break;
        }
        i++;
        const row = elCreateNodes('tr', {"title": rowTitle}, [
            elCreateText('td', {}, station.PLAYLIST),
            elCreateText('td', {}, station.COUNTRY + smallSpace + nDash + smallSpace + station.LANGUAGE),
            elCreateText('td', {}, station.EXTGENRE.join(', '))
        ]);
        setData(row, 'uri', station.streamUri);
        setData(row, 'name', station.PLAYLIST);
        setData(row, 'genre', station.EXTGENRE);
        setData(row, 'image', webradioDbUri + station.EXTIMG);
        setData(row, 'homepage', station.HOMEPAGE);
        setData(row, 'country', station.COUNTRY);
        setData(row, 'language', station.LANGUAGE);
        setData(row, 'type', 'stream');
        row.appendChild(
            elCreateNode('td', {},
                elCreateText('a', {"data-col": "Action", "href": "#", "class": ["mi", "color-darkgrey"], "title": tn('Actions')}, ligatureMore)
            )
        );
        tbody.appendChild(row);
    }
    table.classList.remove('opacity05');

    setPagination(result.returnedEntities, result.returnedEntities);

    if (result.returnedEntities === 0) {
        tbody.appendChild(emptyRow(4));
    }
}

function streamUriToName(uri) {
    return uri.replace(/[<>\/.:?&$!#\|]/g, '_');
}

function showWebradioDbDetails(uri) {
    const tbody = document.getElementById('modalRadioBrowserDetailsList');
    elClearId('modalRadioBrowserDetailsList');
    const m3u = streamUriToName(uri) + '.m3u';
    const result = webradioDb.webradios[m3u];
    if (result.EXTIMG !== '') {
        document.getElementById('RadioBrowserDetailsImage').style.backgroundImage =
            'url("' + myEncodeURIhost(webradioDbUri + result.EXTIMG) + '")' +
            ', url("' + subdir + '/assets/coverimage-loading.svg")';
    }
    else {
        document.getElementById('RadioBrowserDetailsImage').style.backgroundImage =
            'url("' + subdir + '/assets/coverimage-notavailable.svg")';
    }
    document.getElementById('RadioBrowserDetailsTitle').textContent = result.name;
    const showFields = {
        'streamUri': 'Stream url',
        'HOMEPAGE': 'Homepage',
        'EXTGENRE': 'Genre',
        'COUNTRY': 'Country',
        'LANGUAGE': 'Language',
        'DESCRIPTION': 'Description'
    };
    for (const field in showFields) {
        let value;
        switch(field) {
            case 'HOMEPAGE':
                if (result.HOMEPAGE !== '') {
                    value = elCreateText('a', {"class": ["text-success", "external"],
                        "href": myEncodeURIhost(result.HOMEPAGE),
                        "target": "_blank"}, result.HOMEPAGE);
                }
                else {
                    value = document.createTextNode('');
                }
                break;
            default:
                value = document.createTextNode(result[field]);
        }
        tbody.appendChild(
            elCreateNodes('tr', {}, [
                elCreateText('th', {}, tn(showFields[field])),
                elCreateNode('td', {}, value)
            ])
        );
    }
    uiElements.modalRadioBrowserDetails.show();
}

//radio-browser.info api

function countClickRadioBrowser(uuid) {
    if (uuid !== '') {
        sendAPI("MYMPD_API_CLOUD_RADIOBROWSER_CLICK_COUNT", {
            "uuid": uuid
        });
    }
}

//eslint-disable-next-line no-unused-vars
function showRadioBrowserDetails(uuid) {
    sendAPI("MYMPD_API_CLOUD_RADIOBROWSER_STATION_DETAIL", {
        "uuid": uuid
    }, parseRadioBrowserDetails, true);
    uiElements.modalRadioBrowserDetails.show();
    elReplaceChildId('modalRadioBrowserDetailsList',
        elCreateNode('tr', {}, 
            elCreateText('td', {"colspan": 2}, tn('Loading...'))
        )
    );
    countClickRadioBrowser(uuid);
}

function parseRadioBrowserDetails(obj) {
    const tbody = document.getElementById('modalRadioBrowserDetailsList');
    if (checkResult(obj, tbody) === false) {
        return;
    }
    elClearId('modalRadioBrowserDetailsList');
    const result = obj.result.data[0];
    if (result.favicon !== '') {
        document.getElementById('RadioBrowserDetailsImage').style.backgroundImage =
            'url("' + myEncodeURIhost(result.favicon) + '")' +
            ', url("' + subdir + '/assets/coverimage-loading.svg")';
    }
    else {
        document.getElementById('RadioBrowserDetailsImage').style.backgroundImage =
            'url("' + subdir + '/assets/coverimage-notavailable.svg")';
    }
    document.getElementById('RadioBrowserDetailsTitle').textContent = result.name;
    const showFields = {
        'url_resolved': 'Stream url',
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
        let value;
        switch(field) {
            case 'homepage':
                if (result.homepage !== '') {
                    value = elCreateText('a', {"class": ["text-success", "external"],
                        "href": myEncodeURIhost(result.homepage),
                        "target": "_blank"}, result.homepage);
                }
                else {
                    value = document.createTextNode('');
                }
                break;
            case 'lastcheckok':
                value = elCreateText('span', {"class": ["mi"]},
                        (result[field] === 1 ? 'check_circle' : 'error')
                    );
                break;
            default:
                value = document.createTextNode(result[field]);
        }
        tbody.appendChild(
            elCreateNodes('tr', {}, [
                elCreateText('th', {}, tn(showFields[field])),
                elCreateNode('td', {}, value)
            ])
        );
    }
}

function parseRadiobrowserList(obj) {
    const table = document.getElementById('BrowseRadioRadioBrowserList');
    const tbody = table.getElementsByTagName('tbody')[0];
    setScrollViewHeight(table);

    if (checkResult(obj, tbody) === false) {
        return;
    }

    elClear(tbody);
    const nrItems = obj.result.data.length;
    const rowTitle = tn(webuiSettingsDefault.clickRadioBrowser.validValues[settings.webuiSettings.clickRadioBrowser]);
    for (const station of obj.result.data) {
        const row = elCreateNodes('tr', {"title": rowTitle}, [
            elCreateText('td', {}, station.name),
            elCreateText('td', {}, station.country + smallSpace + nDash + smallSpace + station.language),
            elCreateText('td', {}, station.tags.replace(/,/g, ', '))
        ]);
        setData(row, 'uri', station.url_resolved);
        setData(row, 'name', station.name);
        setData(row, 'genre', station.tags);
        setData(row, 'image', station.favicon);
        setData(row, 'homepage', station.homepage);
        setData(row, 'country', station.country);
        setData(row, 'language', station.language);
        setData(row, 'type', 'stream');
        setData(row, 'uuid', station.stationuuid);
        row.appendChild(
            elCreateNode('td', {},
                elCreateText('a', {"data-col": "Action", "href": "#", "class": ["mi", "color-darkgrey"], "title": tn('Actions')}, ligatureMore)
            )
        );
        tbody.appendChild(row);
    }
    table.classList.remove('opacity05');

    setPagination(-1, nrItems);

    if (nrItems === 0) {
        tbody.appendChild(emptyRow(4));
    }
}
