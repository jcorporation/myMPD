"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module viewBrowseDatabase_js */

/**
 * BrowseDatabaseAlbumList handler
 * @returns {void}
 */
function handleBrowseDatabaseAlbumList() {
    handleSearchExpression('BrowseDatabaseAlbumList');

    selectTag('BrowseDatabaseAlbumListTagDropdown', 'btnBrowseDatabaseAlbumListTagDesc', app.current.tag);
    toggleBtnChkId('BrowseDatabaseAlbumListSortDesc', app.current.sort.desc);
    selectTag('BrowseDatabaseAlbumListSortTags', undefined, app.current.sort.tag);

    sendAPI("MYMPD_API_DATABASE_ALBUM_LIST", {
        "offset": app.current.offset,
        "limit": app.current.limit,
        "expression": app.current.search,
        "sort": app.current.sort.tag,
        "sortdesc": app.current.sort.desc,
        "cols": settings.colsBrowseDatabaseAlbumListFetch
    }, parseDatabaseAlbumList, true);
}

/**
 * BrowseDatabaseTagList handler
 * @returns {void}
 */
function handleBrowseDatabaseTagList() {
    handleSearchSimple('BrowseDatabaseTag');
    selectTag('BrowseDatabaseTagListTagDropdown', 'btnBrowseDatabaseTagListTagDesc', app.current.tag);
    mirrorBtnId('BrowseDatabaseTagListSortDesc', app.current.sort.desc);
    sendAPI("MYMPD_API_DATABASE_TAG_LIST", {
        "offset": app.current.offset,
        "limit": app.current.limit,
        "searchstr": app.current.search,
        "tag": app.current.tag,
        "sortdesc": app.current.sort.desc
    }, parseDatabaseTagList, true);
}

/**
 * Handles BrowseDatabaseAlbumDetail
 * @returns {void}
 */
function handleBrowseDatabaseAlbumDetail() {
    sendAPI("MYMPD_API_DATABASE_ALBUM_DETAIL", {
        "albumid": app.current.filter,
        "cols": settings.colsBrowseDatabaseAlbumDetailFetch
    }, parseAlbumDetails, true);
}

/**
 * Initializes the browse database elements
 * @returns {void}
 */
function initViewBrowseDatabase() {
    elGetById('BrowseDatabaseTagListList').addEventListener('click', function(event) {
        if (event.target.classList.contains('row')) {
            return;
        }
        app.current.search = '';
        if (event.target.nodeName === 'DIV') {
            elGetById('BrowseDatabaseTagSearchStr').value = '';
            // clear album search input
            elGetById('BrowseDatabaseAlbumListSearchStr').value = '';
            gotoBrowse(event);
        }
        else if (event.target.nodeName === 'A') {
            event.preventDefault();
            event.stopPropagation();
            if (event.target.getAttribute('data-list') === 'song') {
                elGetById('SearchSearchStr').value = '';
                const tag = getData(event.target.parentNode.parentNode, 'tag');
                const value = getData(event.target.parentNode.parentNode, 'name');
                gotoSearch(tag, value);
            }
            else {
                elGetById('BrowseDatabaseTagSearchStr').value = '';
                // clear album search input
                elGetById('BrowseDatabaseAlbumListSearchStr').value = '';
                gotoBrowse(event);
            }
        }
    }, false);

    initSearchSimple('BrowseDatabaseTag');

    elGetById('BrowseDatabaseTagListSortDesc').addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        app.current.sort.desc = app.current.sort.desc === true ? false : true;
        appGoto(app.current.card, app.current.tab, app.current.view, 0, app.current.limit, app.current.filter, app.current.sort, app.current.tag, app.current.search);
    }, false);

    elGetById('BrowseDatabaseAlbumListList').addEventListener('click', function(event) {
        const target = gridClickHandler(event);
        if (target !== null) {
            appGoto('Browse', 'Database', 'AlbumDetail', 0, undefined, getData(target.parentNode, 'AlbumId'));
        }
    }, false);

    elGetById('BrowseDatabaseAlbumDetailList').addEventListener('click', function(event) {
        const target = tableClickHandler(event);
        if (target !== null) {
            clickSong(getData(target, 'uri'), event);
        }
    }, false);

    elGetById('BrowseDatabaseAlbumListColsDropdown').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON' &&
            event.target.classList.contains('mi'))
        {
            event.stopPropagation();
            event.preventDefault();
            toggleBtnChk(event.target, undefined);
        }
    }, false);

    elGetById('BrowseDatabaseAlbumDetailInfoColsDropdown').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON' &&
            event.target.classList.contains('mi'))
        {
            event.stopPropagation();
            event.preventDefault();
            toggleBtnChk(event.target, undefined);
        }
    }, false);

    elGetById('BrowseDatabaseAlbumListSortDesc').addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        toggleBtnChk(this, undefined);
        app.current.sort.desc = app.current.sort.desc === true ? false : true;
        appGoto(app.current.card, app.current.tab, app.current.view, 0, app.current.limit, app.current.filter, app.current.sort, app.current.tag, app.current.search);
    }, false);

    elGetById('BrowseDatabaseAlbumListSortTags').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            event.preventDefault();
            event.stopPropagation();
            app.current.sort.tag = getData(event.target, 'tag');
            appGoto(app.current.card, app.current.tab, app.current.view, 0, app.current.limit, app.current.filter, app.current.sort, app.current.tag, app.current.search);
        }
    }, false);

    initSearchExpression('BrowseDatabaseAlbumList');
}

/**
 * Parsed the MYMPD_API_DATABASE_ALBUM_LIST response
 * @param {object} obj jsonrpc response object
 * @returns {void}
 */
function parseDatabaseAlbumList(obj) {
    const cardContainer = elGetById('BrowseDatabaseAlbumListList');
    unsetUpdateView(cardContainer);

    if (obj.error !== undefined) {
        elReplaceChild(cardContainer,
            elCreateTextTn('div', {"class": ["col", "not-clickable", "alert", "alert-warning"]}, obj.error.message, obj.error.data)
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
    for (let i = 0; i < nrItems; i++) {
        const card = elCreateEmpty('div', {"data-contextmenu": "album", "class": ["card", "card-grid", "clickable"]});
        const image = obj.result.data[i].FirstSongUri !== 'albumid'
            ? '/albumart-thumb?offset=0&uri=' + myEncodeURIComponent(obj.result.data[i].FirstSongUri)
            : '/albumart-thumb/' + obj.result.data[i].AlbumId;
        card.appendChild(
            elCreateEmpty('div', {"class": ["card-body", "album-cover-loading", "album-cover-grid", "d-flex"]})
        );
        const taglist = [
            pEl.gridSelectBtn.cloneNode(true)
        ];
        for (const tag of settings.colsBrowseDatabaseAlbumList) {
            taglist.push(
                elCreateNode((tag === 'Album' ? 'span' : 'small'), {"class": ["d-block"]},
                    printValue(tag, obj.result.data[i][tag])
                )
            );
        }
        card.appendChild(
            elCreateNodes('div', {"class": ["card-footer", "card-footer-grid", "p-2"],
                "title": obj.result.data[i][tagAlbumArtist] + ': ' + obj.result.data[i].Album}, taglist)
        );
        setData(card, 'image', image);
        setData(card, 'uri', obj.result.data[i].FirstSongUri.replace(/\/[^/]+$/, ''));
        setData(card, 'type', 'album');
        setData(card, 'name', obj.result.data[i].Album);
        setData(card, 'Album', obj.result.data[i].Album);
        setData(card, tagAlbumArtist, obj.result.data[i][tagAlbumArtist]);
        setData(card, 'AlbumId', obj.result.data[i].AlbumId);
        addAlbumPlayButton(card.firstChild);
        const col = elCreateNode('div', {"class": ["col", "px-0", "mb-2", "flex-grow-0"]}, card);

        if (i < cols.length) {
            cols[i].replaceWith(col);
        }
        else {
            cardContainer.append(col);
        }

        if (userAgentData.hasIO === true) {
            const observer = new IntersectionObserver(setGridImage, {root: null, rootMargin: '0px'});
            observer.observe(col);
        }
        else {
            col.firstChild.firstChild.style.backgroundImage = getCssImageUri(image);
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
 * Saves the fields for the database album list
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function saveColsDatabaseAlbumList() {
    //remove ids to force card refresh
    const cols = elGetById('BrowseDatabaseAlbumListList').querySelectorAll('.col');
    for (const col of cols) {
        col.firstChild.firstChild.removeAttribute('id');
    }

    saveColsDropdown('colsBrowseDatabaseAlbumList', 'BrowseDatabaseAlbumListColsDropdown');
}

/**
 * Parsed the MYMPD_API_DATABASE_TAG_LIST response
 * @param {object} obj jsonrpc response object
 * @returns {void}
 */
 function parseDatabaseTagList(obj) {
    const cardContainer = elGetById('BrowseDatabaseTagListList');
    unsetUpdateView(cardContainer);

    if (obj.error !== undefined) {
        elReplaceChild(cardContainer,
            elCreateTextTn('div', {"class": ["col", "not-clickable", "alert", "alert-warning"]}, obj.error.message, obj.error.data)
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

    const listAlbums = settings.tagListAlbum.includes(obj.result.tag);

    if (cardContainer.querySelector('.not-clickable') !== null) {
        elClear(cardContainer);
    }
    let cols = cardContainer.querySelectorAll('.col');
    for (let i = 0; i < nrItems; i++) {
        if (cols[i] !== undefined &&
            getData(cols[i].firstChild,'tag') === obj.result.data[i].value)
        {
            continue;
        }

        const card = elCreateEmpty('div', {"data-contextmenu": "album", "class": ["card", "card-grid", "clickable"]});
        const image = '/tagart?tag=' + myEncodeURIComponent(obj.result.tag) + '&value=' + myEncodeURIComponent(obj.result.data[i].value);
        if (obj.result.pics === true) {
            card.appendChild(
                elCreateEmpty('div', {"class": ["card-body", "album-cover-loading", "album-cover-grid", "d-flex"]})
            );
        }
        const footerElements = [
            elCreateText('div', {}, obj.result.data[i].value)
        ];
        if (listAlbums === true) {
            footerElements.push(
                elCreateText('a', {"class": ["mi", "mi-sm"], "href": "#", "data-list": "song", "data-title-phrase": "Show songs", "title": tn("Show songs")}, 'music_note'),
                elCreateText('a', {"class": ["mi", "mi-sm"], "href": "#", "data-list": "album", "data-title-phrase": "Show albums", "title": tn("Show albums")}, 'album')
            );
        }
        card.appendChild(
            elCreateNodes('div', {"class": ["card-footer", "card-footer-grid", "card-footer-tags", "text-center", "p-2"], "title": obj.result.data[i].value}, footerElements)
        );
        setData(card, 'image', image);
        setData(card, 'tag', obj.result.tag);
        setData(card, 'name', obj.result.data[i].value);

        const col = elCreateNode('div', {"class": ["col", "px-0", "mb-2", "flex-grow-0"]}, card);

        if (i < cols.length) {
            cols[i].replaceWith(col);
        }
        else {
            cardContainer.append(col);
        }
        if (obj.result.pics === true) {
            if (userAgentData.hasIO === true) {
                const observer = new IntersectionObserver(setGridImage, {root: null, rootMargin: '0px'});
                observer.observe(col);
            }
            else {
                col.firstChild.firstChild.style.backgroundImage = getCssImageUri(image);
            }
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
 * Adds the album play button
 * @param {ChildNode | HTMLElement} parentEl parent element for the button
 * @returns {void}
 */
function addAlbumPlayButton(parentEl) {
    const playBtn = pEl.coverPlayBtn.cloneNode(true);
    parentEl.appendChild(playBtn);
    playBtn.addEventListener('click', function(event) {
        event.preventDefault();
        event.stopPropagation();
        clickQuickPlay(event.target);
    }, false);
}

/**
 * Parses the MYMPD_API_DATABASE_ALBUM_DETAIL response
 * @param {object} obj jsonrpc response object
 * @returns {void}
 */
function parseAlbumDetails(obj) {
    const table = elGetById('BrowseDatabaseAlbumDetailList');
    const tfoot = table.querySelector('tfoot');
    const colspan = settings.colsBrowseDatabaseAlbumDetail.length;
    const infoEl = elGetById('viewDatabaseAlbumDetailInfoTags');

    if (checkResultId(obj, 'BrowseDatabaseAlbumDetailList') === false) {
        elClear(infoEl);
        return;
    }

    const coverEl = elGetById('viewDatabaseAlbumDetailCover');
    coverEl.style.backgroundImage = getCssImageUri('/albumart?offset=0&uri=' + myEncodeURIComponent(obj.result.data[0].uri));
    setData(coverEl, 'images', obj.result.images);
    setData(coverEl, 'embeddedImageCount', obj.result.embeddedImageCount);
    setData(coverEl, 'uri', obj.result.data[0].uri);

    elClear(infoEl);
    infoEl.appendChild(
        elCreateText('h1', {}, obj.result.Album)
    );
    for (const col of settings.colsBrowseDatabaseAlbumDetailInfo) {
        infoEl.appendChild(
            elCreateNodes('div', {"class": ["col-xl-6"]}, [
                elCreateTextTn('small', {}, col),
                elCreateNode('p', {},
                    printValue(col, obj.result[col])
                )
            ])
        );
    }

    if (obj.result.bookletPath !== '') {
        infoEl.appendChild(
            elCreateNodes('div', {"class": ["col-xl-6"]}, [
                elCreateText('span', {"class": ["mi", "me-2"]}, 'description'),
                elCreateTextTn('a', {"target": "_blank", "href": subdir + myEncodeURI(obj.result.bookletPath)}, 'Download booklet')
            ])
        );
    }

    if (obj.result.infoTxtPath !== '') {
        const infoTxtEl = elCreateNodes('div', {"class": ["col-xl-6"]}, [
            elCreateText('span', {"class": ["mi", "me-2"]}, 'article'),
            elCreateTextTn('span', {"class": ["clickable"]}, 'Album info')
        ]);
        setData(infoTxtEl, 'uri', obj.result.infoTxtPath);
        infoTxtEl.addEventListener('click', function(event) {
            showInfoTxt(event.target);
        }, false);
        infoEl.appendChild(infoTxtEl);
    }

    const mbField = addMusicbrainzFields(obj.result, false);
    if (mbField !== null) {
        infoEl.appendChild(mbField);
    }

    const rowTitle = tn(settingsWebuiFields.clickSong.validValues[settings.webuiSettings.clickSong]);
    updateTable(obj, 'BrowseDatabaseAlbumDetail', function(row, data) {
        setData(row, 'type', 'song');
        setData(row, 'name', data.Title);
        setData(row, 'uri', data.uri);
        row.setAttribute('title', rowTitle);
    });

    elReplaceChild(tfoot,
        elCreateNode('tr', {"class": ["not-clickable"]},
            elCreateNode('td', {"colspan": colspan + 1},
                elCreateNodes('small', {}, [
                    elCreateTextTnNr('span', {}, 'Num songs', obj.result.returnedEntities),
                    elCreateText('span', {}, smallSpace + nDash + smallSpace + fmtDuration(obj.result.Duration))
                ])
            )
        )
    );
}

/**
 * Go's to the last browse database grid view
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function backToAlbumGrid() {
    appGoto('Browse', 'Database', 'AlbumList');
}

/**
 * Wrapper for add buttons in the album detail view
 * @param {string} action action to perform
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function currentAlbumAdd(action) {
    switch(action) {
        case 'appendQueue':
            appendQueue('album', [app.current.filter]);
            break;
        case 'appendPlayQueue':
            appendPlayQueue('album', [app.current.filter]);
            break;
        case 'insertAfterCurrentQueue':
            insertAfterCurrentQueue('album', [app.current.filter]);
            break;
        case 'replaceQueue':
            replaceQueue('album', [app.current.filter]);
            break;
        case 'replacePlayQueue':
            replacePlayQueue('album', [app.current.filter]);
            break;
        case 'addPlaylist':
            showAddToPlaylist('album', [app.current.filter]);
            break;
        case 'addAlbumToHome': {
            const name = document.querySelector('#viewDatabaseAlbumDetailInfoTags > h1').textContent;
            const images = getDataId('viewDatabaseAlbumDetailCover', 'images');
            addAlbumToHome(app.current.filter, name, (images.length > 0 ? images[0]: ''));
            break;
        }
        default:
            logError('Invalid action: ' + action);
    }
}
