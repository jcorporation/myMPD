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

    setView('BrowseDatabaseAlbumList');
    setView('BrowseDatabaseAlbumDetail');
    setView('BrowseDatabaseTagList');
}

/**
 * Click event handler for database tag list
 * @param {MouseEvent} event click event
 * @param {HTMLElement} target calculated target
 * @returns {void}
 */
function viewBrowseDatabaseTagListListClickHandler(event, target) {
    event.preventDefault();
    event.stopPropagation();
    app.current.search = '';
    const tag = getData(target, 'tag');
    if (settings.tagListAlbum.includes(tag)) {
        elGetById('BrowseDatabaseTagSearchStr').value = '';
        // clear album search input
        elGetById('BrowseDatabaseAlbumListSearchStr').value = '';
        gotoBrowse(event);
    }
    else {
        elGetById('SearchSearchStr').value = '';
        const value = getData(event.target.parentNode, 'name');
        gotoSearch(tag, value);
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
            row.setAttribute('title', tn('Show album'));
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
        card.setAttribute('title', tn('Show album'));
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

    const rowTitle = tn(settings.tagListAlbum.includes(obj.result.tag) ? 'Show albums' : 'Show songs');
    if (settings['view' + app.id].mode === 'table') {
        const tfoot = cardContainer.querySelector('tfoot');
        const colspan = settings['view' + app.id].fields.length;
        const smallWidth = uiSmallWidthTagRows();
        const actionTd = elCreateEmpty('td', {"data-col": "Action"});
        addActionLinks(actionTd, obj.result.tag);
        elClear(tfoot);
        updateTable(obj, app.id, function(row, data, result) {
            if (result.pics === true) {
                data.Thumbnail = getCssImageUri('/tagart?tag=' + myEncodeURIComponent(result.tag) + '&value=' + myEncodeURIComponent(data.value));
            }
            setData(row, 'tag', result.tag);
            setData(row, 'name', data.Value);
            row.setAttribute('title', rowTitle);
        }, function(row, data) {
            tableRow(row, data, app.id, colspan, smallWidth, actionTd);
        });
        addTblFooter(tfoot,
            elCreateTextTnNr('span', {}, 'Num entries', obj.result.totalEntities)
        );
        return;
    }
    updateGrid(obj, app.id, function(card, data, result) {
        if (result.pics === true) {
            data.Thumbnail = getCssImageUri('/tagart?tag=' + myEncodeURIComponent(result.tag) + '&value=' + myEncodeURIComponent(data.Value));
        }
        setData(card, 'tag', result.tag);
        setData(card, 'name', data.Value);
        card.setAttribute('title', rowTitle);
        card.classList.add('no-contextmenu');
    }, undefined, function(footer, data, result) {
        addActionLinks(footer, result.tag);
    });
}

/**
 * Shows the edit sticker modal for the current album
 * @param {Event} event triggering click event
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showAlbumSticker(event) {
    event.preventDefault();
    const uri = getDataId('viewDatabaseAlbumDetailCover', 'AlbumId');
    showStickerModal(uri, 'mympd_album');
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
    setData(coverEl, 'AlbumId', obj.result.AlbumId);
    const feedbackGrp = elGetById('BrowseDatabaseAlbumDetailFeedback').firstElementChild;
    setData(feedbackGrp, 'uri', obj.result.AlbumId);
    setFeedback(feedbackGrp, obj.result.like, obj.result.rating);

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
