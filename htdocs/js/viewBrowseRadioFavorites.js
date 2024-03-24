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

    elGetById('BrowseRadioFavoritesList').addEventListener('click', function(event) {
        const target = gridClickHandler(event);
        if (target !== null) {
            const uri = getData(target.parentNode, 'uri');
            clickRadioFavorites(uri, event);
        }
    }, false);
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
    const cardContainer = elGetById('BrowseRadioFavoritesList');
    if (checkResult(obj, cardContainer, undefined) === false) {
        return;
    }

    unsetUpdateView(cardContainer);
    let cols = cardContainer.querySelectorAll('.col');
    const rowTitle = tn(settingsWebuiFields.clickRadioFavorites.validValues[settings.webuiSettings.clickRadioFavorites]);
    for (let i = 0; i < obj.result.returnedEntities; i++) {
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
        addGridQuickPlayButton(card.firstChild);

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
    for (let i = cols.length - 1; i >= obj.result.returnedEntities; i--) {
        cols[i].remove();
    }

    setPagination(obj.result.totalEntities, obj.result.returnedEntities);
    setScrollViewHeight(cardContainer);
    scrollToPosY(cardContainer.parentNode, app.current.scrollPos);
}
