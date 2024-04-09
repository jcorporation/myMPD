"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
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
        "fields": settings.viewBrowseDatabaseAlbumListFetch.fields
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
        "fields": settings.viewBrowseDatabaseAlbumDetailFetch.fields
    }, parseAlbumDetails, true);
}

/**
 * Initializes the browse database elements
 * @returns {void}
 */
function initViewBrowseDatabase() {
    initSearchSimple('BrowseDatabaseTag');

    elGetById('BrowseDatabaseTagListSortDesc').addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        app.current.sort.desc = app.current.sort.desc === true ? false : true;
        appGoto(app.current.card, app.current.tab, app.current.view, 0, app.current.limit, app.current.filter, app.current.sort, app.current.tag, app.current.search);
    }, false);

    initSortBtns('BrowseDatabaseAlbumList');
    initSearchExpression('BrowseDatabaseAlbumList');
}

/**
 * Click event handler for database tag list
 * @param {MouseEvent} event click event
 * @returns {void}
 */
function viewBrowseDatabaseTagListListClickHandler(event) {
    app.current.search = '';
    if (event.target.nodeName === 'DIV') {
        elGetById('BrowseDatabaseTagSearchStr').value = '';
        // clear album search input
        elGetById('BrowseDatabaseAlbumListSearchStr').value = '';
        gotoBrowse(event);
    }
    else if (event.target.nodeName === 'A') {
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
}

/**
 * Click event handler for database album list
 * @param {MouseEvent} event click event
 * @param {HTMLElement} target calculated target
 * @returns {void}
 */
function viewBrowseDatabaseAlbumListListClickHandler(event, target) {
    appGoto('Browse', 'Database', 'AlbumDetail', 0, undefined, getData(target, 'AlbumId'));
}

/**
 * Click event handler for database album detail song list
 * @param {MouseEvent} event click event
 * @param {HTMLElement} target calculated target
 * @returns {void}
 */
function viewBrowseDatabaseAlbumDetailListClickHandler(event, target) {
    clickSong(getData(target, 'uri'), event);
}

/**
 * Parsed the MYMPD_API_DATABASE_ALBUM_LIST response
 * @param {object} obj jsonrpc response object
 * @returns {void}
 */
function parseDatabaseAlbumList(obj) {
    const cardContainer = elGetById('BrowseDatabaseAlbumListList');
    if (checkResult(obj, cardContainer, undefined) === false) {
        return;
    }
    if (settings['view' + app.id].mode === 'table') {
        const tfoot = cardContainer.querySelector('tfoot');
        elClear(tfoot);
        updateTable(obj, app.id, function(row, data) {
            setData(row, 'uri', data.FirstSongUri.replace(/\/[^/]+$/, ''));
            setData(row, 'type', 'album');
            setData(row, 'name', data.Album);
            setData(row, 'Album', data.Album);
            setData(row, tagAlbumArtist, data[tagAlbumArtist]);
            setData(row, 'AlbumId', data.AlbumId);
        });
        addTblFooter(tfoot,
            elCreateTextTnNr('span', {}, 'Num entries', obj.result.totalEntities)
        );
        return;
    }
    updateGrid(obj, app.id, function(card, data) {
        setData(card, 'uri', data.FirstSongUri.replace(/\/[^/]+$/, ''));
        setData(card, 'type', 'album');
        setData(card, 'name', data.Album);
        setData(card, 'Album', data.Album);
        setData(card, tagAlbumArtist, data[tagAlbumArtist]);
        setData(card, 'AlbumId', data.AlbumId);
    });
}

/**
 * Parsed the MYMPD_API_DATABASE_TAG_LIST response
 * @param {object} obj jsonrpc response object
 * @returns {void}
 */
 function parseDatabaseTagList(obj) {
    const cardContainer = elGetById('BrowseDatabaseTagListList');
    if (checkResult(obj, cardContainer, undefined) === false) {
        return;
    }

    unsetUpdateView(cardContainer);
    const listAlbums = settings.tagListAlbum.includes(obj.result.tag);
    let cols = cardContainer.querySelectorAll('.col');
    for (let i = 0; i < obj.result.returnedEntities; i++) {
        if (cols[i] !== undefined &&
            getData(cols[i].firstChild,'tag') === obj.result.data[i].value)
        {
            continue;
        }

        const card = elCreateEmpty('div', {"class": ["card", "card-grid", "clickable"]});
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
    for (let i = cols.length - 1; i >= obj.result.returnedEntities; i--) {
        cols[i].remove();
    }

    setPagination(obj.result.totalEntities, obj.result.returnedEntities);
    setScrollViewHeight(cardContainer);
    scrollToPosY(cardContainer.parentNode, app.current.scrollPos);
}

/**
 * Parses the MYMPD_API_DATABASE_ALBUM_DETAIL response
 * @param {object} obj jsonrpc response object
 * @returns {void}
 */
function parseAlbumDetails(obj) {
    const table = elGetById('BrowseDatabaseAlbumDetailList');
    const tfoot = table.querySelector('tfoot');
    const colspan = settings.viewBrowseDatabaseAlbumDetail.fields.length;
    const infoEl = elGetById('viewDatabaseAlbumDetailInfoTags');

    if (checkResultId(obj, 'BrowseDatabaseAlbumDetailList', 'grid') === false) {
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
    for (const col of settings.viewBrowseDatabaseAlbumDetailInfo.fields) {
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
