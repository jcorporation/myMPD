"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function initWebradio() {
    document.getElementById('BrowseRadioRadiobrowserSearchStr').addEventListener('keyup', function(event) {
        clearSearchTimer();
        if (event.key === 'Escape') {
            this.blur();
        }
        else {
            searchTimer = setTimeout(function() {
                searchRadiobrowser();
            }, searchTimerTimeout);
        }
    }, false);

    document.getElementById('BrowseRadioRadiobrowserFilter').addEventListener('show.bs.collapse', function() {
        document.getElementById('BrowseRadioRadiobrowserFilterBtn').classList.add('active');
    }, false);

    document.getElementById('BrowseRadioRadiobrowserFilter').addEventListener('hide.bs.collapse', function() {
        document.getElementById('BrowseRadioRadiobrowserFilterBtn').classList.remove('active');
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
            //action td
            handleActionTdClick(event);
        }
    }, false);

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

    document.getElementById('addToWebradioFavorites').addEventListener('click', function(event) {
        event.preventDefault();
        showEditRadioFavorite(getDataId('RadiobrowserDetailsTitle', 'webradio'));
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
            //action td
            handleActionTdClick(event);
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
        clearSearchTimer();
        if (event.key === 'Escape') {
            this.blur();
        }
        else {
            const value = this.value;
            searchTimer = setTimeout(function() {
                appGoto(app.current.card, app.current.tab, app.current.view,
                    0, app.current.limit, app.current.filter, app.current.sort, '-', value);
            }, searchTimerTimeout);
        }
    }, false);

    setDataId('editRadioFavoriteImage', 'cb-filter', 'filterImageSelect');
    setDataId('editRadioFavoriteImage', 'cb-filter-options', ['editRadioFavoriteImage']);
}

function initWebradiodbFilter(id, dbField, name) {
    document.getElementById(id).addEventListener('change', function() {
        doSearchWebradiodb();
    }, false);
    setDataId(id, 'cb-filter', [id]);
    setDataId(id, 'cb-filter-options', [id, dbField, name]);
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

function showEditRadioFavorite(obj) {
    cleanupModalId('modalSaveRadioFavorite');
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

    uiElements.modalSaveRadioFavorite.show();
}

//eslint-disable-next-line no-unused-vars
function saveRadioFavorite() {
    cleanupModalId('modalSaveRadioFavorite');
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
                subdir + '/browse/pics/thumbs/' + obj.result.data[i].Image;
        }
        setData(card, 'image', image);
        setData(card, 'uri', obj.result.data[i].filename);
        setData(card, 'name', obj.result.data[i].Name);
        setData(card, 'type', 'webradio');
        addRadioFavoritesPlayButton(card.firstChild);

        const col = elCreateNode('div', {"class": ["col", "px-0", "mb-2", "flex-grow-0"]}, card);

        if (i < cols.length) {
            cols[i].replaceWith(col);
        }
        else {
            cardContainer.append(col);
        }

        if (userAgentData.hasIO === true) {
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

function addRadioFavoritesPlayButton(parentEl) {
    const div = pEl.coverPlayBtn.cloneNode(true);
    parentEl.appendChild(div);
    div.addEventListener('click', function(event) {
        event.preventDefault();
        event.stopPropagation();
        clickQuickPlay(event.target);
    }, false);
}

//webradiodb api

function getWebradiodb() {
    const list = document.getElementById('BrowseRadioWebradiodbList').getElementsByTagName('tbody')[0];
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
        const result = searchWebradiodb(app.current.search, app.current.filter.genre,
            app.current.filter.country, app.current.filter.language, app.current.sort,
            app.current.offset, app.current.limit);
        parseSearchWebradiodb(result);
    }, false);
}

function filterWebradiodbFilter(id, source, placeholder, searchStr) {
    searchStr = searchStr.toLowerCase();
    const el = document.getElementById(id);
    elClear(el.filterResult);
    el.addFilterResult(tn(placeholder), '');
    let i = 0;
    for (const value of webradioDb[source]) {
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
        //primary sort by defined tag
		if (a[sort.tag] < b[sort.tag]) {
            return sort.desc === false ? -1 : 1;
		}
		if (a[sort.tag] > b[sort.tag]) {
            return sort.desc === false ? 1 : -1;
		}
		//secondary sort by Name
		if (a.Name < b.Name) {
            return sort.desc === false ? -1 : 1;
		}
		if (a.Name > b.Name) {
            return sort.desc === false ? 1 : -1;
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

function parseSearchWebradiodb(obj) {
    const table = document.getElementById('BrowseRadioWebradiodbList');
    const tfoot = table.getElementsByTagName('tfoot')[0];
    elClear(tfoot);

    if (app.current.filter.genre === '' &&
        app.current.filter.country === '' &&
        app.current.filter.language === '')
    {
        document.getElementById('BrowseRadioWebradiodbFilterBtn').firstElementChild.textContent = 'filter_list_off';
    }
    else {
        document.getElementById('BrowseRadioWebradiodbFilterBtn').firstElementChild.textContent = 'filter_list';
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
                elCreateText('td', {"colspan": colspan}, tn('Num entries', obj.result.totalEntities))
            )
        );
    }
}

function streamUriToName(uri) {
    return uri.replace(/[<>/.:?&$!#|;=]/g, '_');
}

//eslint-disable-next-line no-unused-vars
function showWebradiodbDetails(uri) {
    //reuse the radiobrowser modal
    const tbody = document.getElementById('modalRadiobrowserDetailsList');
    elClearId('modalRadiobrowserDetailsList');
    const m3u = isStreamUri(uri) ? streamUriToName(uri) + '.m3u' : uri;
    const result = webradioDb.webradios[m3u];
    if (result.Image !== '') {
        document.getElementById('RadiobrowserDetailsImage').style.backgroundImage =
            'url("' + myEncodeURIhost(webradioDbPicsUri + result.Image) + '")' +
            ', url("' + subdir + '/assets/coverimage-loading.svg")';
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
                elCreateText('th', {}, tn(field)),
                elCreateNode('td', {}, value)
            ])
        );
    }
    const alternateStreams = Object.keys(result.alternativeStreams);
    if (alternateStreams.length > 0) {
        const td = elCreateEmpty('td', {});
        for (const name of alternateStreams) {
            const p = elCreateText('p', {"class": ["pb-0"]}, result.alternativeStreams[name].Codec + ' / ' + 
                result.alternativeStreams[name].Bitrate + ' ' + tn('kbit'));
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
                elCreateText('th', {}, tn('Alternative streams')),
                td
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

function searchRadiobrowser() {
    app.current.filter.tags = document.getElementById('inputRadiobrowserTags').value;
    app.current.filter.country = document.getElementById('inputRadiobrowserCountry').value;
    app.current.filter.language = document.getElementById('inputRadiobrowserLanguage').value;
    appGoto(app.current.card, app.current.tab, app.current.view,
        0, app.current.limit, app.current.filter, '-', '-', document.getElementById('BrowseRadioRadiobrowserSearchStr').value);
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
    //map fields to webradiodb fields
    setDataId('RadiobrowserDetailsTitle', 'webradio', {
        "Name": result.name,
        "StreamUri": result.url_resolved,
        "Genre": result.tags,
        "Homepage": result.homepage,
        "Country": result.country,
        "Language": result.language,
        "Codec": result.codec,
        "Bitrate": result.bitrate,
        "Description": "",
        "Image": result.favicon
    });
    //friendly names for fields
    const showFields = {
        'url_resolved': 'StreamUri',
        'homepage': 'Homepage',
        'tags': 'Tags',
        'country': 'Country',
        'language': 'Language',
        'codec': 'Codec',
        'bitrate': 'Bitrate',
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
    if (app.current.filter.tags === '' &&
        app.current.filter.country === '' &&
        app.current.filter.language === '')
    {
        document.getElementById('BrowseRadioRadiobrowserFilterBtn').firstElementChild.textContent = 'filter_list_off';
    }
    else {
        document.getElementById('BrowseRadioRadiobrowserFilterBtn').firstElementChild.textContent = 'filter_list';
    }

    if (checkResultId(obj, 'BrowseRadioRadiobrowserList') === false) {
        return;
    }

    const rowTitle = tn(webuiSettingsDefault.clickRadiobrowser.validValues[settings.webuiSettings.clickRadiobrowser]);
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
