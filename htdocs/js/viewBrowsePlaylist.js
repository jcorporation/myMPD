"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module viewBrowsePlaylists_js */

/**
 * Handles BrowsePlaylistDetail
 * @returns {void}
 */
function handleBrowsePlaylistDetail() {
    handleSearchExpression('BrowsePlaylistDetail');

    sendAPI("MYMPD_API_PLAYLIST_CONTENT_LIST", {
        "offset": app.current.offset,
        "limit": app.current.limit,
        "expression": app.current.search,
        "plist": app.current.tag,
        "fields": settings.viewBrowsePlaylistDetailFetch.fields
    }, parsePlaylistDetail, true);
}

/**
 * Handles BrowsePlaylistList
 * @returns {void}
 */
function handleBrowsePlaylistList() {
    handleSearchSimple('BrowsePlaylistList');
    toggleBtnChkId('BrowsePlaylistListSortDesc', app.current.sort.desc);
    selectTag('BrowsePlaylistListSortTagsList', undefined, app.current.sort.tag);

    sendAPI("MYMPD_API_PLAYLIST_LIST", {
        "offset": app.current.offset,
        "limit": app.current.limit,
        "searchstr": app.current.search,
        "type": 0,
        "sort": app.current.sort.tag,
        "sortdesc": app.current.sort.desc,
        "fields": settings.viewBrowsePlaylistList.fields
    }, parsePlaylistList, true);
    elHideId('playlistDetailAlert');
}

/**
 * Initializes the playlist elements
 * @returns {void}
 */
function initViewPlaylist() {
    elGetById('BrowsePlaylistDetailSortTagsDropdown').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            event.preventDefault();
            currentPlaylistSort(getData(event.target, 'tag'), getBtnChkValueId('BrowsePlaylistDetailSortDesc'));
        }
    }, false);

    initSearchSimple('BrowsePlaylistList');
    initSearchExpression('BrowsePlaylistDetail');
    initSortBtns('BrowsePlaylistList');

    setView('BrowsePlaylistList');
    setView('BrowsePlaylistDetail');
}

/**
 * Shows the edit sticker modal for the current playlist
 * @param {Event} event triggering click event
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showPlaylistSticker(event) {
    event.preventDefault();
    const uri = getDataId('BrowsePlaylistDetailList', 'uri');
    showStickerModal(uri, 'playlist');
}

/**
 * Click event handler for playlist list
 * @param {MouseEvent} event click event
 * @param {HTMLElement} target calculated target
 * @returns {void}
 */
function viewPlaylistListListClickHandler(event, target) {
    if (getData(target, 'smartpls-only') === false) {
        clickPlaylist(getData(target, 'uri'), event);
    }
    else {
        showNotification(tn('Playlist is empty'), 'playlist', 'warn');
    }
}

/**
 * Click event handler for playlist detail list
 * @param {MouseEvent} event click event
 * @param {HTMLElement} target calculated target
 * @returns {void}
 */
function viewPlaylistDetailListClickHandler(event, target) {
    clickSong(getData(target, 'uri'), event);
}

/**
 * Parses the MYMPD_API_PLAYLIST_LIST jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parsePlaylistList(obj) {
    const table = elGetById('BrowsePlaylistListList');
    if (checkResult(obj, table, undefined) === false) {
        return;
    }

    if (settings['view' + app.id].mode === 'table') {
        const tfoot = table.querySelector('tfoot');
        elClear(tfoot);
        updateTable(obj, 'BrowsePlaylistList', function(row, data) {
            parsePlaylistListUpdate(row, data);
        });
        addTblFooter(tfoot,
            elCreateTextTnNr('span', {}, 'Num entries', obj.result.totalEntities)
        );
        return;
    }
    if (settings['view' + app.id].mode === 'grid') {
        updateGrid(obj, app.id, function(card, data) {
            parsePlaylistListUpdate(card, data);
        });
        return;
    }
    updateList(obj, app.id, function(card, data) {
        parsePlaylistListUpdate(card, data);
    });
}

/**
 * Callback function for row or card
 * @param {HTMLElement} card Row or card
 * @param {object} data Data object
 * @returns {void}
 */
function parsePlaylistListUpdate(card, data) {
    const rowTitle = settingsWebuiFields.clickPlaylist.validValues[settings.webuiSettings.clickPlaylist];
    setData(card, 'uri', data.uri);
    setData(card, 'type', data.Type);
    setData(card, 'name', data.Name);
    setData(card, 'smartpls-only', data.smartplsOnly);
    card.setAttribute('title', tn(rowTitle));
}

/**
 * Parses the MYMPD_API_PLAYLIST_CONTENT_LIST jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parsePlaylistDetail(obj) {
    const table = elGetById('BrowsePlaylistDetailList');
    const imageEl = elGetById('BrowsePlaylistDetailImage');
    const stickerEl = elGetById('BrowsePlaylistDetailSticker');
    elClear(imageEl);
    elClear(stickerEl);

    if (checkResult(obj, table, undefined) === false) {
        return;
    }

    // set toolbar
    let rw = false;
    if (isMPDplaylist(obj.result.plist) === false) {
        // playlist in music directory
        setData(table, 'ro', true);
        elHideId('BrowsePlaylistDetailContentBtns');
        elHideId('BrowsePlaylistDetailSmartPlaylistContentBtns');
        table.setAttribute('data-rw', 'false');
    }
    else if (obj.result.smartpls === true) {
        // smart playlist
        setData(table, 'ro', true);
        elHideId('BrowsePlaylistDetailContentBtns');
        elShowId('BrowsePlaylistDetailSmartPlaylistContentBtns');
        table.setAttribute('data-rw', 'false');
    }
    else {
        // mpd playlist
        setData(table, 'ro', false);
        elShowId('BrowsePlaylistDetailContentBtns');
        elHideId('BrowsePlaylistDetailSmartPlaylistContentBtns');
        table.setAttribute('data-rw', 'true');
        rw = true;
    }

    if (obj.result.pics === true) {
        const img = elCreateEmpty('div', {});
        img.style.backgroundImage = getCssImageUri('/playlistart?playlist=' + myEncodeURIComponent(obj.result.plist));
        img.addEventListener('click', function(event) {
            zoomPicture(event.target);
        }, false);
        imageEl.appendChild(img);
        elShow(imageEl);
    }
    else {
        elHide(imageEl);
    }

    if (features.featStickers === true) {
        let stickerCount = 0;
        const tbl = elCreateEmpty('table', {'class':['table', 'table-sm']});
        for (const key in obj.result.sticker) {
            tbl.appendChild(
                elCreateNodes('tr', {}, [
                    elCreateText('th', {'class': ['pe-2']}, key ),
                    elCreateText('td', {}, obj.result.sticker[key])
                ])
            );
            stickerCount++;
        }
        if (obj.result.lastPlayedSong.uri !== '') {
            stickerCount++;
            const resumeBtn = pEl.resumeBtn.cloneNode(true);
            resumeBtn.classList.add('ms-3', 'dropdown');
            resumeBtn.classList.remove('dropup');
            setData(resumeBtn, 'pos', obj.result.lastPlayedSong.pos);
            new BSN.Dropdown(resumeBtn.firstElementChild);
            resumeBtn.lastElementChild.firstElementChild.addEventListener('click', function(event) {
                clickResumePlist(event);
            }, false);
            tbl.appendChild(
                elCreateNodes('tr', {}, [
                    elCreateTextTn('th', {'class': ['pe-2']}, 'Last played'),
                    elCreateNodes('td', {}, [
                        document.createTextNode(obj.result.lastPlayedSong.title),
                        resumeBtn
                    ])
                ])
            );
        }
        if (stickerCount > 0) {
            stickerEl.appendChild(elCreateTextTn('h4', {}, 'Sticker'));
            stickerEl.appendChild(tbl);
        }
    }

    setData(table, 'playlistlength', obj.result.totalEntities);
    setData(table, 'uri', obj.result.plist);
    setData(table, 'type', obj.result.smartpls === true ? 'smartpls' : 'plist');
    elGetById('BrowsePlaylistDetailTitle').textContent =
        (obj.result.smartpls === true ? tn('Smart playlist') : tn('Playlist')) + ': ' + obj.result.plist;
    const feedbackGrp = elGetById('BrowsePlaylistDetailFeedback').firstElementChild;
    setData(feedbackGrp, 'uri', obj.result.plist);
    setFeedback(feedbackGrp, obj.result.like, obj.result.rating);

    if (settings['view' + app.id].mode === 'table') {
        const tfoot = table.querySelector('tfoot');
        elClear(tfoot);
        updateTable(obj, app.id, function(row, data) {
            if (rw === true) {
                row.setAttribute('draggable', 'true');
                row.setAttribute('tabindex', 0);
            }
            parsePlaylistDetailUpdate(row, data);
        });
        setPlaylistDetailListFooter(obj.result.totalEntities, obj.result.totalTime);
        return;
    }
    if (settings['view' + app.id].mode === 'grid') {
        updateGrid(obj, app.id, function(card, data) {
            if (rw === true) {
                card.setAttribute('draggable', 'true');
                card.setAttribute('tabindex', '0');
            }
            parsePlaylistDetailUpdate(card, data);
        });
        return;
    }
    updateList(obj, app.id, function(card, data) {
        if (rw === true) {
            card.setAttribute('draggable', 'true');
            card.setAttribute('tabindex', '0');
        }
        parsePlaylistDetailUpdate(card, data);
    });
}

/**
 * Callback function for row or card
 * @param {HTMLElement} card Row or card
 * @param {object} data Data object
 * @returns {void}
 */
function parsePlaylistDetailUpdate(card, data) {
    const rowTitle = settingsWebuiFields.clickSong.validValues[settings.webuiSettings.clickSong];
    card.setAttribute('id', 'playlistSongId' + data.Pos);
    setData(card, 'type', data.Type);
    setData(card, 'uri', data.uri);
    setData(card, 'name', data.Title);
    setData(card, 'pos', data.Pos);
    card.setAttribute('title', tn(rowTitle));
}

/**
 * Sets the footer text for the playlist content view
 * @param {number} entities entity count
 * @param {number} playtime total playtime
 * @returns {void}
 */
function setPlaylistDetailListFooter(entities, playtime) {
    const footerEl = entities === -1
        ? elCreateNode('small', {},
              elCreateText('button', {"data-title-phrase": "Enumerate", "title": "Enumerate", "id": "BrowsePlaylistDetailEnumerateBtn", "class": ["btn", "btn-sm", "btn-secondary", "mi"]}, 'insights')
          )
        : elCreateNodes('small', {}, [
              elCreateTextTnNr('span', {}, 'Num songs', entities),
              elCreateText('span', {}, smallSpace + nDash + smallSpace + fmtDuration(playtime))
          ]);

    const tfoot = elGetById('BrowsePlaylistDetailList').querySelector('tfoot');
    const colspan = settings.viewBrowsePlaylistDetail.fields.length + 1;

    elReplaceChild(tfoot,
        elCreateNode('tr', {"class": ["not-clickable"]},
            elCreateNode('td', {"colspan": colspan}, footerEl)
        )
    );

    if (entities === -1) {
        footerEl.addEventListener('click', function() {
            currentPlaylistEnumerate();
        }, false);
    }
}

/**
 * Enumerates the current displayed playlist
 * @returns {void}
 */
function currentPlaylistEnumerate() {
    btnWaitingId('BrowsePlaylistDetailEnumerateBtn', true);
    sendAPI("MYMPD_API_PLAYLIST_CONTENT_ENUMERATE", {
        "plist": getDataId('BrowsePlaylistDetailList', 'uri')
    }, function(obj) {
        if (obj.result) {
            setPlaylistDetailListFooter(obj.result.entities, obj.result.playtime);
        }
        else {
            setPlaylistDetailListFooter(-1, 0);
        }
    }, true);
}

/**
 * Opens the playlist detail view
 * @param {string} uri shows the playlist detail view
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function playlistDetails(uri) {
    setUpdateViewId('BrowsePlaylistListList');
    appGoto('Browse', 'Playlist', 'Detail', 0, undefined, undefined, {'tag': '', 'desc': false}, uri, '');
}

/**
 * Shuffles the playlist
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function currentPlaylistShuffle() {
    setUpdateViewId('BrowsePlaylistDetailList');
    sendAPI("MYMPD_API_PLAYLIST_CONTENT_SHUFFLE", {
        "plist": getDataId('BrowsePlaylistDetailList', 'uri')
    }, function() {
        unsetUpdateViewId('BrowsePlaylistDetailList');
    }, true);
}

/**
 * Validates the currently displayed playlist
 * @param {boolean} remove true = remove invalid entries, false = count number of invalid entries
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function currentPlaylistValidate(remove) {
    const plist = getDataId('BrowsePlaylistDetailList', 'uri');
    setUpdateViewId('BrowsePlaylistDetailList');
    playlistValidate(plist, remove);
}

/**
 * Deduplicates the currently displayed playlist
 * @param {boolean} remove true = remove invalid entries, false = count number of invalid entries
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function currentPlaylistDedup(remove) {
    setUpdateViewId('BrowsePlaylistDetailList');
    const plist = getDataId('BrowsePlaylistDetailList', 'uri'); 
    playlistDedup(plist, remove);
}

/**
 * Validates and deduplicates the currently displayed playlist
 * @param {boolean} remove true = remove invalid entries, false = count number of invalid entries
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function currentPlaylistValidateDedup(remove) {
    setUpdateViewId('BrowsePlaylistDetailList');
    const plist = getDataId('BrowsePlaylistDetailList', 'uri'); 
    playlistValidateDedup(plist, remove);
}

/**
 * Sorts the playlist by tag
 * @param {string} tag sort tag
 * @param {boolean} sortdesc sort descending
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function currentPlaylistSort(tag, sortdesc) {
    setUpdateViewId('BrowsePlaylistDetailList');
    sendAPI("MYMPD_API_PLAYLIST_CONTENT_SORT", {
        "plist": getDataId('BrowsePlaylistDetailList', 'uri'),
        "tag": tag,
        "sortdesc": sortdesc
    }, function() {
        unsetUpdateViewId('BrowsePlaylistDetailList');
    }, true);
}

/**
 * Moves a song in the current displayed playlist
 * @param {number} from from position
 * @param {number} to to position
 * @returns {void}
 */
function currentPlaylistMoveSong(from, to) {
    sendAPI("MYMPD_API_PLAYLIST_CONTENT_MOVE_POSITION", {
        "plist": getDataId('BrowsePlaylistDetailList', 'uri'),
        "from": from,
        "to": to
    }, null, false);
}

/**
 * Adds the currently displayed playlist to the queue or home screen
 * @param {string} action one of appendQueue, appendPlayQueue,
 *                               insertAfterCurrentQueue, replaceQueue,
 *                               replacePlayQueue, addToHome
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function currentPlaylistAddTo(action) {
    const uri = getDataId('BrowsePlaylistDetailList', 'uri');
    const type = getDataId('BrowsePlaylistDetailList', 'type');
    switch(action) {
        case 'appendQueue':
            appendQueue(type, [uri]);
            break;
        case 'appendPlayQueue':
            appendPlayQueue(type, [uri]);
            break;
        case 'insertAfterCurrentQueue':
            insertAfterCurrentQueue(type, [uri], null);
            break;
        case 'replaceQueue':
            replaceQueue(type, [uri]);
            break;
        case 'replacePlayQueue':
            replacePlayQueue(type, [uri]);
            break;
        case 'addToHome':
            addPlistToHome(uri, type, uri);
            break;
        default:
            logError('Invalid action: ' + action);
    }
}
