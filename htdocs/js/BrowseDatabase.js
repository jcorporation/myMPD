"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module BrowseDatabase_js */

/**
 * BrowseDatabaseAlbumList handler
 * @returns {void}
 */
function handleBrowseDatabaseAlbumList() {
    setFocusId('searchDatabaseAlbumListStr');
    selectTag('searchDatabaseAlbumListTags', 'searchDatabaseAlbumListTagsDesc', app.current.filter);
    selectTag('BrowseDatabaseAlbumListTagDropdown', 'btnBrowseDatabaseAlbumListTagDesc', app.current.tag);
    toggleBtnChkId('databaseAlbumListSortDesc', app.current.sort.desc);
    selectTag('databaseAlbumListSortTags', undefined, app.current.sort.tag);
    createSearchCrumbs(app.current.search, document.getElementById('searchDatabaseAlbumListStr'), document.getElementById('searchDatabaseAlbumListCrumb'));
    if (app.current.search === '') {
        document.getElementById('searchDatabaseAlbumListStr').value = '';
    }
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
    setFocusId('searchDatabaseTagListStr');
    document.getElementById('searchDatabaseTagListStr').value = app.current.search;
    selectTag('BrowseDatabaseTagListTagDropdown', 'btnBrowseDatabaseTagListTagDesc', app.current.tag);
    mirrorBtnId('databaseTagListSortDesc', app.current.sort.desc);
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
function initBrowseDatabase() {
    document.getElementById('BrowseDatabaseTagListList').addEventListener('click', function(event) {
        if (event.target.classList.contains('row')) {
            return;
        }
        app.current.search = '';
        document.getElementById('searchDatabaseTagListStr').value = '';
        appGoto(app.current.card, app.current.tab, 'AlbumList', 0, undefined, 'Album', tagAlbumArtist, 'Album',
            '((' + app.current.tag + ' == \'' + escapeMPD(getData(event.target.parentNode, 'tag')) + '\'))');
    }, false);

    document.getElementById('searchDatabaseTagListStr').addEventListener('keyup', function(event) {
        if (ignoreKeys(event) === true) {
            return;
        }
        clearSearchTimer();
        const value = this.value;
        searchTimer = setTimeout(function() {
            appGoto(app.current.card, app.current.tab, app.current.view,
                0, app.current.limit, app.current.filter, app.current.sort, app.current.tag, value);
        }, searchTimerTimeout);
    }, false);

    document.getElementById('databaseTagListSortDesc').addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        app.current.sort.desc = app.current.sort.desc === true ? false : true;
        appGoto(app.current.card, app.current.tab, app.current.view, 0, app.current.limit, app.current.filter, app.current.sort, app.current.tag, app.current.search);
    }, false);

    document.getElementById('BrowseDatabaseAlbumListList').addEventListener('click', function(event) {
        if (event.target.classList.contains('row')) {
            return;
        }
        //select mode
        if (selectCard(event) === true) {
            return;
        }
        const target = event.target.closest('DIV');
        if (target === null) {
            return;
        }
        if (target.classList.contains('card-body')) {
            appGoto('Browse', 'Database', 'AlbumDetail', 0, undefined, getData(target.parentNode, 'AlbumId'));
        }
        else if (target.classList.contains('card-footer')){
            showContextMenu(event);
        }
    }, false);

    document.getElementById('BrowseDatabaseAlbumListList').addEventListener('contextmenu', function(event) {
        if (event.target.classList.contains('row') ||
            event.target.classList.contains('album-grid-mouseover') ||
            event.target.parentNode.classList.contains('not-clickable'))
        {
            return;
        }
        showContextMenu(event);
    }, false);

    document.getElementById('BrowseDatabaseAlbumListList').addEventListener('long-press', function(event) {
        if (event.target.classList.contains('row') ||
            event.target.parentNode.classList.contains('not-clickable'))
        {
            return;
        }
        showContextMenu(event);
    }, false);

    document.getElementById('BrowseDatabaseAlbumDetailList').addEventListener('click', function(event) {
        //select mode
        if (selectRow(event) === true) {
            return;
        }
        if (event.target.nodeName === 'A') {
            //action td
            handleActionTdClick(event);
            return;
        }
        //table body
        const target = event.target.closest('TR');
        if (target === null) {
            return;
        }
        if (target.parentNode.nodeName === 'TBODY' &&
            checkTargetClick(target) === true)
        {
            clickSong(getData(target, 'uri'), event);
        }
    }, false);

    document.getElementById('BrowseDatabaseAlbumListColsDropdown').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON' &&
            event.target.classList.contains('mi'))
        {
            event.stopPropagation();
            event.preventDefault();
            toggleBtnChk(event.target, undefined);
        }
    }, false);

    document.getElementById('BrowseDatabaseAlbumDetailInfoColsDropdown').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON' &&
            event.target.classList.contains('mi'))
        {
            event.stopPropagation();
            event.preventDefault();
            toggleBtnChk(event.target, undefined);
        }
    }, false);

    document.getElementById('searchDatabaseAlbumListTags').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            app.current.filter = getData(event.target, 'tag');
            searchDatabaseAlbumList(document.getElementById('searchDatabaseAlbumListStr').value);
        }
    }, false);

    document.getElementById('databaseAlbumListSortDesc').addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        toggleBtnChk(this, undefined);
        app.current.sort.desc = app.current.sort.desc === true ? false : true;
        appGoto(app.current.card, app.current.tab, app.current.view, 0, app.current.limit, app.current.filter, app.current.sort, app.current.tag, app.current.search);
    }, false);

    document.getElementById('databaseAlbumListSortTags').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            event.preventDefault();
            event.stopPropagation();
            app.current.sort.tag = getData(event.target, 'tag');
            appGoto(app.current.card, app.current.tab, app.current.view, 0, app.current.limit, app.current.filter, app.current.sort, app.current.tag, app.current.search);
        }
    }, false);

    document.getElementById('searchDatabaseAlbumListStr').addEventListener('keydown', function(event) {
        //handle Enter key on keydown for IME composing compatibility
        if (event.key !== 'Enter') {
            return;
        }
        clearSearchTimer();
        const value = this.value;
        if (value !== '') {
            const op = getSelectValueId('searchDatabaseAlbumListMatch');
            const crumbEl = document.getElementById('searchDatabaseAlbumListCrumb');
            crumbEl.appendChild(createSearchCrumb(app.current.filter, op, value));
            elShow(crumbEl);
            this.value = '';
        }
        else {
            searchTimer = setTimeout(function() {
                searchDatabaseAlbumList(value);
            }, searchTimerTimeout);
        }
    }, false);

    document.getElementById('searchDatabaseAlbumListStr').addEventListener('keyup', function(event) {
        if (ignoreKeys(event) === true) {
            return;
        }
        clearSearchTimer();
        const value = this.value;
        searchTimer = setTimeout(function() {
            searchDatabaseAlbumList(value);
        }, searchTimerTimeout);
    }, false);

    document.getElementById('searchDatabaseAlbumListMatch').addEventListener('change', function() {
        searchDatabaseAlbumList(document.getElementById('searchDatabaseAlbumListStr').value);
    });

    document.getElementById('searchDatabaseAlbumListCrumb').addEventListener('click', function(event) {
        if (event.target.nodeName === 'SPAN') {
            //remove search expression
            event.preventDefault();
            event.stopPropagation();
            event.target.parentNode.remove();
            searchDatabaseAlbumList('');
            document.getElementById('searchDatabaseAlbumListStr').updateBtn();
        }
        else if (event.target.nodeName === 'BUTTON') {
            //edit search expression
            event.preventDefault();
            event.stopPropagation();
            selectTag('searchDatabaseAlbumListTags', 'searchDatabaseAlbumListTagsDesc', getData(event.target,'filter-tag'));
            const searchDatabaseAlbumListStrEl = document.getElementById('searchDatabaseAlbumListStr');
            searchDatabaseAlbumListStrEl.value = unescapeMPD(getData(event.target, 'filter-value'));
            document.getElementById('searchDatabaseAlbumListMatch').value = getData(event.target, 'filter-op');
            event.target.remove();
            app.current.filter = getData(event.target,'filter-tag');
            searchDatabaseAlbumList(searchDatabaseAlbumListStrEl.value);
            if (document.getElementById('searchDatabaseAlbumListCrumb').childElementCount === 0) {
                elHideId('searchDatabaseAlbumListCrumb');
            }
            searchDatabaseAlbumListStrEl.updateBtn();
        }
    }, false);
}

/**
 * Parsed the MYMPD_API_DATABASE_ALBUM_LIST response
 * @param {object} obj jsonrpc response object
 * @returns {void}
 */
function parseDatabaseAlbumList(obj) {
    const cardContainer = document.getElementById('BrowseDatabaseAlbumListList');
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
        if (cols[i] !== undefined &&
            getData(cols[i].firstChild.firstChild, 'AlbumId') === obj.result.data[i].AlbumId)
        {
            continue;
        }

        let image = '';
        const card = elCreateEmpty('div', {"data-contextmenu": "album", "class": ["card", "card-grid", "clickable"]});

        image = '/albumart-thumb?offset=0&uri=' + myEncodeURIComponent(obj.result.data[i].FirstSongUri);
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
    const cols = document.getElementById('BrowseDatabaseAlbumListList').querySelectorAll('.col');
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
    const cardContainer = document.getElementById('BrowseDatabaseTagListList');
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
        if (cols[i] !== undefined &&
            getData(cols[i].firstChild.firstChild,'tag') === obj.result.data[i].value)
        {
            continue;
        }

        let image = '';
        const card = elCreateEmpty('div', {"data-contextmenu": "album", "class": ["card", "card-grid", "clickable"]});

        image = '/tagart?uri=' + myEncodeURIComponent(obj.result.tag + '/' + obj.result.data[i].value);
        if (obj.result.pics === true) {
            card.appendChild(
                elCreateEmpty('div', {"class": ["card-body", "album-cover-loading", "album-cover-grid", "d-flex"]})
            );
        }
        card.appendChild(
            elCreateText('div', {"class": ["card-footer", "card-footer-grid", "p-2"],
                "title": obj.result.data[i].value}, obj.result.data[i].value)
        );
        setData(card, 'image', image);
        setData(card, 'tag', obj.result.data[i].value);

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
    const table = document.getElementById('BrowseDatabaseAlbumDetailList');
    const tfoot = table.querySelector('tfoot');
    const colspan = settings.colsBrowseDatabaseAlbumDetail.length;
    const infoEl = document.getElementById('viewDatabaseAlbumDetailInfoTags');

    if (checkResultId(obj, 'BrowseDatabaseAlbumDetailList') === false) {
        elClear(infoEl);
        return;
    }

    const coverEl = document.getElementById('viewDatabaseAlbumDetailCover');
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

    if (obj.result.bookletPath !== '' &&
        features.featLibrary === true)
    {
        infoEl.appendChild(
            elCreateNodes('div', {"class": ["col-xl-6"]}, [
                elCreateText('span', {"class": ["mi", "me-2"]}, 'description'),
                elCreateTextTn('a', {"target": "_blank", "href": subdir + myEncodeURI(obj.result.bookletPath)}, 'Download booklet')
            ])
        );
    }

    const mbField = addMusicbrainzFields(obj.result, false);
    if (mbField !== null) {
        infoEl.appendChild(mbField);
    }

    const rowTitle = tn(webuiSettingsDefault.clickSong.validValues[settings.webuiSettings.clickSong]);
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
                    elCreateTextTnNr('span', {}, 'Num songs', obj.result.SongCount),
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
function addAlbum(action) {
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
            showAddToPlaylist(['ALBUM', app.current.filter], '');
            break;
        case 'addAlbumToHome': {
            const name = document.querySelector('#viewDatabaseAlbumDetailInfoTags > h1').textContent;
            const images = getDataId('viewDatabaseAlbumDetailCover', 'images');
            addAlbumToHome(app.current.filter, name, (images.length > 0 ? images[0]: ''));
            break;
        }
    }
}

/**
 * Handles single disc actions
 * @param {string} action action to perform
 * @param {string} albumId the album id
 * @param {string} disc disc number as string
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addAlbumDisc(action, albumId, disc) {
    switch(action) {
        case 'appendQueue':
            appendQueue('disc', [albumId, disc]);
            break;
        case 'appendPlayQueue':
            appendPlayQueue('disc', [albumId, disc]);
            break;
        case 'insertAfterCurrentQueue':
            insertAfterCurrentQueue('disc', [albumId, disc]);
            break;
        case 'replaceQueue':
            replaceQueue('disc', [albumId, disc]);
            break;
        case 'replacePlayQueue':
            replacePlayQueue('disc', [albumId, disc]);
            break;
        case 'addPlaylist':
            showAddToPlaylist(['DISC', albumId, disc], '');
            break;
    }
}

/**
 * Creates and executes the mpd filter expression from the search crumbs and current search values
 * for the album grid search.
 * @param {string} searchStr string to search
 * @returns {void}
 */
function searchDatabaseAlbumList(searchStr) {
    const expression = createSearchExpression(document.getElementById('searchDatabaseAlbumListCrumb'), app.current.filter, getSelectValueId('searchDatabaseAlbumListMatch'), searchStr);
    appGoto(app.current.card, app.current.tab, app.current.view,
        0, app.current.limit, app.current.filter, app.current.sort, app.current.tag, expression, 0);
}
