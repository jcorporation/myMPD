"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module viewBrowseRadioFavorites_js */

/**
 * Browse RadioFavorites handler
 * @returns {void}
 */
function handleBrowseRadioFavorites() {
    handleSearchSimple('BrowseRadioFavorites');
    sendAPI("MYMPD_API_WEBRADIO_FAVORITE_LIST", {
        "offset": app.current.offset,
        "limit": app.current.limit,
        "searchstr": app.current.search
    }, parseRadioFavoritesList, true);
}

/**
 * Initialization function for radio favorites elements
 * @returns {void}
 */
function initBrowseRadioFavorites() {
    initSearchSimple('BrowseRadioFavorites');

    setDataId('editRadioFavoriteImage', 'cb-filter', 'filterImageSelect');
    setDataId('editRadioFavoriteImage', 'cb-filter-options', ['editRadioFavoriteImage']);

    document.getElementById('addToWebradioFavorites').addEventListener('click', function(event) {
        event.preventDefault();
        showEditRadioFavorite(getDataId('RadiobrowserDetailsTitle', 'webradio'));
    }, false);

    document.getElementById('BrowseRadioFavoritesList').addEventListener('click', function(event) {
        const target = gridClickHandler(event);
        if (target !== null) {
            const uri = getData(target.parentNode, 'uri');
            clickRadioFavorites(uri, event);
        }
    }, false);
}

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

/**
 * Saves a webradio favorite
 * @returns {void}
 */
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
        uiElements.modalSaveRadioFavorite.hide();
        if (app.id === 'BrowseRadioFavorites') {
            getRadioFavoriteList();
        }
    }
}

/**
 * Parses the jsonrpc response from MYMPD_API_WEBRADIO_FAVORITE_LIST
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parseRadioFavoritesList(obj) {
    const cardContainer = document.getElementById('BrowseRadioFavoritesList');
    unsetUpdateView(cardContainer);

    if (obj.error !== undefined) {
        elReplaceChild(cardContainer,
            elCreateTextTn('div', {"class": ["col", "not-clickable", "alert", "alert-danger"]}, obj.error.message, obj.error.data)
        );
        setPagination(0, 0);
        return;
    }

    const nrItems = obj.result.returnedEntities;
    if (nrItems === 0) {
        elReplaceChild(cardContainer,
            elCreateTextTn('div', {"class": ["col", "not-clickable", "alert", "alert-secondary"]}, 'Empty list')
        );
        setPagination(0, 0);
        return;
    }

    if (cardContainer.querySelector('.not-clickable') !== null) {
        elClear(cardContainer);
    }
    let cols = cardContainer.querySelectorAll('.col');
    const rowTitle = tn(webuiSettingsDefault.clickRadioFavorites.validValues[settings.webuiSettings.clickRadioFavorites]);
    for (let i = 0; i < nrItems; i++) {
        const card = elCreateNodes('div', {"data-contextmenu": "webradio", "class": ["card", "card-grid", "clickable"], "tabindex": 0}, [
            elCreateEmpty('div', {"class": ["card-body", "album-cover-loading", "album-cover-grid", "d-flex"], "title": rowTitle}),
            elCreateNodes('div', {"class": ["card-footer", "card-footer-grid", "p-2"]}, [
                pEl.gridSelectBtn.cloneNode(true),
                document.createTextNode(obj.result.data[i].Name),
                elCreateEmpty('br', {}),
                elCreateText('small', {}, obj.result.data[i].Genre),
                elCreateEmpty('br', {}),
                elCreateText('small', {}, obj.result.data[i].Country +
                    smallSpace + nDash + smallSpace + obj.result.data[i].Language)
            ])
        ]);
        const image = obj.result.data[i].Image === ''
            ? '/assets/coverimage-stream'
            : obj.result.data[i].Image;
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
            col.firstChild.firstChild.style.backgroundImage = subdir + image;
        }
    }
    //remove obsolete cards
    cols = cardContainer.querySelectorAll('.col');
    for (let i = cols.length - 1; i >= nrItems; i--) {
        cols[i].remove();
    }

    setPagination(obj.result.totalEntities, obj.result.returnedEntities);
    setScrollViewHeight(cardContainer);
    scrollToPosY(cardContainer.parentNode, app.current.scrollPos);
}

/**
 * Adds the quick play button to the webradio favorite icon
 * @param {ChildNode} parentEl the containing element
 * @returns {void}
 */
function addRadioFavoritesPlayButton(parentEl) {
    const div = pEl.coverPlayBtn.cloneNode(true);
    parentEl.appendChild(div);
    div.addEventListener('click', function(event) {
        event.preventDefault();
        event.stopPropagation();
        clickQuickPlay(event.target);
    }, false);
}
