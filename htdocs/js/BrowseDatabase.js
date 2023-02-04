"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module BrowseDatabase_js */

/**
 * BrowseDatabaseAlbumList handler
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
 */
function handleBrowseDatabaseAlbumDetail() {
    sendAPI("MYMPD_API_DATABASE_ALBUM_DETAIL", {
        "album": app.current.tag,
        "albumartist": app.current.search,
        "cols": settings.colsBrowseDatabaseAlbumDetailFetch
    }, parseAlbumDetails, true);
}

/**
 * Initializes the browse database elements
 */
function initBrowseDatabase() {
    document.getElementById('BrowseDatabaseTagListList').addEventListener('click', function(event) {
        if (event.target.classList.contains('row')) {
            return;
        }
        app.current.search = '';
        document.getElementById('searchDatabaseTagListStr').value = '';
        appGoto(app.current.card, app.current.tab, 'AlbumList', 0, undefined, 'Album', 'AlbumArtist', 'Album',
            '((' + app.current.tag + ' == \'' + escapeMPD(getData(event.target.parentNode, 'tag')) + '\'))');
    }, false);

    document.getElementById('searchDatabaseTagListStr').addEventListener('keyup', function(event) {
        clearSearchTimer();
        const value = this.value;
        if (event.key === 'Escape') {
            clearSearchTimer();
            this.blur();
        }
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
        const target = getParent(event.target, 'DIV');
        if (target.classList.contains('card-body')) {
            appGoto('Browse', 'Database', 'AlbumDetail', 0, undefined, 'Album', 'AlbumArtist',
                getData(target.parentNode, 'Album'),
                getData(target.parentNode, 'AlbumArtist')
            );
        }
        else if (target.classList.contains('card-footer')){
            showPopover(event);
        }
    }, false);

    document.getElementById('BrowseDatabaseAlbumListList').addEventListener('contextmenu', function(event) {
        if (event.target.classList.contains('row') ||
            event.target.parentNode.classList.contains('not-clickable'))
        {
            return;
        }
        showPopover(event);
    }, false);

    document.getElementById('BrowseDatabaseAlbumListList').addEventListener('long-press', function(event) {
        if (event.target.classList.contains('row') ||
            event.target.parentNode.classList.contains('not-clickable'))
        {
            return;
        }
        showPopover(event);
    }, false);

    document.getElementById('BrowseDatabaseAlbumDetailList').addEventListener('click', function(event) {
        if (event.target.nodeName === 'A') {
            //action td
            handleActionTdClick(event);
            return;
        }
        const target = getParent(event.target, 'TR');
        if (checkTargetClick(target) === true) {
            clickSong(getData(target, 'uri'));
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

    document.getElementById('searchDatabaseAlbumListStr').addEventListener('keyup', function(event) {
        clearSearchTimer();
        const value = this.value;
        if (event.key === 'Escape') {
            clearSearchTimer();
            this.blur();
        }
        else if (event.key === 'Enter') {
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
        }
        else {
            searchTimer = setTimeout(function() {
                searchDatabaseAlbumList(value);
            }, searchTimerTimeout);
        }
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
        }
        else if (event.target.nodeName === 'BUTTON') {
            //edit search expression
            event.preventDefault();
            event.stopPropagation();
            selectTag('searchDatabaseAlbumListTags', 'searchDatabaseAlbumListTagsDesc', getData(event.target,'filter-tag'));
            document.getElementById('searchDatabaseAlbumListStr').value = unescapeMPD(getData(event.target, 'filter-value'));
            document.getElementById('searchDatabaseAlbumListMatch').value = getData(event.target, 'filter-op');
            event.target.remove();
            app.current.filter = getData(event.target,'filter-tag');
            searchDatabaseAlbumList(document.getElementById('searchDatabaseAlbumListStr').value);
            if (document.getElementById('searchDatabaseAlbumListCrumb').childElementCount === 0) {
                elHideId('searchDatabaseAlbumListCrumb');
            }
        }
    }, false);
}

/**
 * Parsed the MYMPD_API_DATABASE_ALBUM_LIST response
 * @param {object} obj jsonrpc response object
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
    const cols = cardContainer.querySelectorAll('.col');
    for (let i = 0; i < nrItems; i++) {
        //id is used only to check if card should be refreshed
        const id = genId('database' + obj.result.data[i].Album + obj.result.data[i].AlbumArtist);

        if (cols[i] !== undefined &&
            cols[i].firstChild.firstChild.getAttribute('id') === id) {
            continue;
        }

        let image = '';
        const card = elCreateEmpty('div', {"data-popover": "album", "class": ["card", "card-grid", "clickable"]});

        image = '/albumart-thumb?offset=0&uri=' + myEncodeURIComponent(obj.result.data[i].FirstSongUri);
        card.appendChild(
            elCreateEmpty('div', {"class": ["card-body", "album-cover-loading", "album-cover-grid", "d-flex"], "id": id})
        );
        const taglist = [];
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
        setData(card, 'AlbumArtist', obj.result.data[i].AlbumArtist);
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
    for (let i = cols.length - 1; i >= nrItems; i--) {
        cols[i].remove();
    }

    setPagination(obj.result.totalEntities, obj.result.returnedEntities);
    setScrollViewHeight(cardContainer);
    scrollToPosY(cardContainer.parentNode, app.current.scrollPos);
}

/**
 * Saves the fields for the database album list
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
    const cols = cardContainer.querySelectorAll('.col');
    for (let i = 0; i < nrItems; i++) {
        //id is used only to check if card should be refreshed
        const id = genId('database' + obj.result.data[i].value);

        if (cols[i] !== undefined &&
            cols[i].firstChild.firstChild.getAttribute('id') === id) {
            continue;
        }

        let image = '';
        const card = elCreateEmpty('div', {"data-popover": "album", "class": ["card", "card-grid", "clickable"]});

        image = '/tagart?uri=' + myEncodeURIComponent(obj.result.tag + '/' + obj.result.data[i].value);
        if (obj.result.pics === true) {
            card.appendChild(
                elCreateEmpty('div', {"class": ["card-body", "album-cover-loading", "album-cover-grid", "d-flex"], "id": id})
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
 */
function addAlbumPlayButton(parentEl) {
    const div = pEl.coverPlayBtn.cloneNode(true);
    parentEl.appendChild(div);
    div.addEventListener('click', function(event) {
        event.preventDefault();
        event.stopPropagation();
        clickAlbumPlay(getData(event.target.parentNode.parentNode, 'AlbumArtist'), getData(event.target.parentNode.parentNode, 'Album'));
    }, false);
}

/**
 * Parses the MYMPD_API_DATABASE_ALBUM_DETAIL response
 * @param {object} obj jsonrpc response object
 */
function parseAlbumDetails(obj) {
    const table = document.getElementById('BrowseDatabaseAlbumDetailList');
    const tfoot = table.querySelector('tfoot');
    const colspan = settings.colsBrowseDatabaseAlbumDetail.length;
    const infoEl = document.getElementById('viewDatabaseAlbumDetailInfoTags');

    if (checkResultId(obj, 'BrowseDatabaseAlbumDetailList') === false) {
        elClear(infoEl);
        elClear(tfoot);
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
 */
//eslint-disable-next-line no-unused-vars
function backToAlbumGrid() {
    appGoto('Browse', 'Database', 'AlbumList');
}

/**
 * Wrapper for _addAlbum for add buttons in the album detail view
 * @param {string} action action to perform
 */
//eslint-disable-next-line no-unused-vars
function addAlbum(action) {
    // @ts-ignore search in this view an array
    _addAlbum(action, app.current.search, app.current.tag, undefined);
}

/**
 * Appends an album to the queue.
 * Wrapper for _addAlbum for home icon action.
 * @param {*} type not used
 * @param {Array} albumArtist array of albumartists
 * @param {string} album album name
 */
//eslint-disable-next-line no-unused-vars
function appendQueueAlbum(type, albumArtist, album) {
    //type not used but required for home icon cmd
    _addAlbum('appendQueue', albumArtist, album, undefined);
}

/**
 * Appends an album to the queue and plays it.
 * Wrapper for _addAlbum for home icon action.
 * @param {*} type not used
 * @param {Array} albumArtist array of albumartists
 * @param {string} album album name
 */
//eslint-disable-next-line no-unused-vars
function appendPlayQueueAlbum(type, albumArtist, album) {
    //type not used but required for home icon cmd
    _addAlbum('appendPlayQueue', albumArtist, album, undefined);
}

/**
 * Replaces the queue with an album.
 * Wrapper for _addAlbum for home icon action.
 * @param {*} type not used
 * @param {Array} albumArtist array of albumartists
 * @param {string} album album name
 */
//eslint-disable-next-line no-unused-vars
function replaceQueueAlbum(type, albumArtist, album) {
    //type not used but required for home icon cmd
    _addAlbum('replaceQueue', albumArtist, album, undefined);
}

/**
 * Replaces the queue with an album and plays it.
 * Wrapper for _addAlbum for home icon action.
 * @param {*} type not used
 * @param {Array} albumArtist array of albumartists
 * @param {string} album album name
 */
//eslint-disable-next-line no-unused-vars
function replacePlayQueueAlbum(type, albumArtist, album) {
    //type not used but required for home icon cmd
    _addAlbum('replacePlayQueue', albumArtist, album, undefined);
}

/**
 * Inserts the an album after the current playing song.
 * Wrapper for _addAlbum for home icon action.
 * @param {*} type not used
 * @param {Array} albumArtist array of albumartists
 * @param {string} album album name
 */
//eslint-disable-next-line no-unused-vars
function insertAfterCurrentQueueAlbum(type, albumArtist, album) {
    //type not used but required for home icon cmd
    _addAlbum('insertQueue', albumArtist, album, undefined);
}

/**
 * Inserts the an album after the current playing song and plays it.
 * Wrapper for _addAlbum for home icon action.
 * @param {*} type not used
 * @param {Array} albumArtist array of albumartists
 * @param {string} album album name
 */
//eslint-disable-next-line no-unused-vars
function insertPlayAfterCurrentQueueAlbum(type, albumArtist, album) {
    //type not used but required for home icon cmd
    _addAlbum('insertPlayQueue', albumArtist, album, undefined);
}

/**
 * Adds an album to the queue or a playlist
 * @param {string} action action to perform
 * @param {Array} albumArtist array of albumartists
 * @param {string} album album name
 * @param {string} disc optional disc number, "undefined" to add the whole album
 */
function _addAlbum(action, albumArtist, album, disc) {
    let expression = '((Album == \'' + escapeMPD(album) + '\')';
    for (const artist of albumArtist) {
        expression += ' AND (' + tagAlbumArtist + ' == \'' + escapeMPD(artist) + '\')';
    }
    if (disc !== undefined) {
        expression += ' AND (Disc == \'' + escapeMPD(disc) + '\')';
    }
    expression += ')';

    switch(action) {
        case 'appendQueue':
            appendQueue('search', expression);
            break;
        case 'appendPlayQueue':
            appendPlayQueue('search', expression);
            break;
        case 'replaceQueue':
            replaceQueue('search', expression);
            break;
        case 'replacePlayQueue':
            replacePlayQueue('search', expression);
            break;
        case 'insertAfterCurrentQueue':
            insertAfterCurrentQueue('search', expression);
            break;
        case 'insertPlayAfterCurrentQueue':
            insertPlayAfterCurrentQueue('search', expression);
            break;
        case 'addPlaylist':
            showAddToPlaylist('ALBUM', expression);
            break;
    }
}

/**
 * Creates and executes the mpd filter expression from the search crumbs and current search values
 * for the album grid search.
 * @param {*} searchStr string to search
 */
function searchDatabaseAlbumList(searchStr) {
    const expression = createSearchExpression(document.getElementById('searchDatabaseAlbumListCrumb'), app.current.filter, getSelectValueId('searchDatabaseAlbumListMatch'), searchStr);
    appGoto(app.current.card, app.current.tab, app.current.view,
        0, app.current.limit, app.current.filter, app.current.sort, app.current.tag, expression, 0);
}
