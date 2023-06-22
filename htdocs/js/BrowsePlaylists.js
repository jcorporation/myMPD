"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module BrowsePlaylist_js */

/**
 * Handles BrowsePlaylistDetail
 * @returns {void}
 */
function handleBrowsePlaylistDetail() {
    setFocusId('searchPlaylistsDetailStr');
    sendAPI("MYMPD_API_PLAYLIST_CONTENT_LIST", {
        "offset": app.current.offset,
        "limit": app.current.limit,
        "searchstr": app.current.search,
        "plist": app.current.filter,
        "cols": settings.colsBrowsePlaylistDetailFetch
    }, parsePlaylistsDetail, true);
    const searchPlaylistsStrEl = document.getElementById('searchPlaylistsDetailStr');
    if (searchPlaylistsStrEl.value === '' &&
        app.current.search !== '')
    {
        searchPlaylistsStrEl.value = app.current.search;
    }
}

/**
 * Handles BrowsePlaylistList
 * @returns {void}
 */
function handleBrowsePlaylistList() {
    setFocusId('searchPlaylistListStr');
    sendAPI("MYMPD_API_PLAYLIST_LIST", {
        "offset": app.current.offset,
        "limit": app.current.limit,
        "searchstr": app.current.search,
        "type": 0
    }, parsePlaylistList, true);
    const searchPlaylistsStrEl = document.getElementById('searchPlaylistListStr');
    if (searchPlaylistsStrEl.value === '' &&
        app.current.search !== '')
    {
        searchPlaylistsStrEl.value = app.current.search;
    }
    elHideId('playlistDetailAlert');
}

/**
 * Initializes the playlist elements
 * @returns {void}
 */
function initPlaylists() {
    document.getElementById('modalAddToPlaylist').addEventListener('shown.bs.modal', function () {
        if (document.getElementById('addStreamFrm').classList.contains('d-none')) {
            setFocusId('addToPlaylistPlaylist');
        }
        else {
            setFocusId('streamUrl');
            document.getElementById('streamUrl').value = '';
        }
    });

    setDataId('addToPlaylistPlaylist', 'cb-filter', 'filterPlaylistsSelect');
    setDataId('addToPlaylistPlaylist', 'cb-filter-options', [1, 'addToPlaylistPlaylist']);

    document.getElementById('dropdownSortPlaylistTags').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            event.preventDefault();
            playlistSort(getData(event.target, 'tag'));
        }
    }, false);

    document.getElementById('searchPlaylistsDetailStr').addEventListener('keyup', function(event) {
        if (ignoreKeys(event) === true) {
            return;
        }
        clearSearchTimer();
        const value = this.value;
        searchTimer = setTimeout(function() {
            appGoto(app.current.card, app.current.tab, app.current.view,
                0, app.current.limit, app.current.filter, app.current.sort, '-', value);
        }, searchTimerTimeout);
    }, false);

    document.getElementById('searchPlaylistListStr').addEventListener('keyup', function(event) {
        if (ignoreKeys(event) === true) {
            return;
        }
        clearSearchTimer();
        const value = this.value;
        searchTimer = setTimeout(function() {
            appGoto(app.current.card, app.current.tab, app.current.view,
                0, app.current.limit, app.current.filter, app.current.sort, '-', value);
        }, searchTimerTimeout);
    }, false);

    document.getElementById('BrowsePlaylistListList').addEventListener('click', function(event) {
        //select mode
        if (selectRow(event) === true) {
            return;
        }
        //action td
        if (event.target.nodeName === 'A') {
            handleActionTdClick(event);
            return;
        }

        const target = event.target.closest('TR');
        if (checkTargetClick(target) === true) {
            if (getData(target, 'smartpls-only') === false) {
                clickPlaylist(getData(target, 'uri'), event);
            }
            else {
                showNotification(tn('Playlist is empty'), 'playlist', 'warn')
            }
        }
    }, false);

    document.getElementById('BrowsePlaylistDetailList').addEventListener('click', function(event) {
        if (event.target.nodeName === 'CAPTION') {
            return;
        }
        //select mode
        if (selectRow(event) === true) {
            return;
        }
        //action td
        if (event.target.nodeName === 'A') {
            handleActionTdClick(event);
            return;
        }
        const target = event.target.closest('TR');
        if (checkTargetClick(target) === true) {
            clickSong(getData(target, 'uri'), event);
        }
    }, false);
}

/**
 * Parses the MYMPD_API_PLAYLIST_LIST jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parsePlaylistList(obj) {
    if (checkResultId(obj, 'BrowsePlaylistListList') === false) {
        return;
    }

    const rowTitle = webuiSettingsDefault.clickPlaylist.validValues[settings.webuiSettings.clickPlaylist];
    updateTable(obj, 'BrowsePlaylistList', function(row, data) {
        setData(row, 'uri', data.uri);
        setData(row, 'type', data.Type);
        setData(row, 'name', data.name);
        setData(row, 'smartpls-only', data.smartplsOnly);
        row.setAttribute('title', tn(rowTitle));
    }, function(row, data) {
        row.appendChild(
            elCreateNode('td', {"data-col": "Type"},
                elCreateText('span', {"class": ["mi"]}, (data.Type === 'smartpls' ? 'queue_music' : 'list'))
            )
        );
        row.appendChild(
            elCreateText('td', {}, data.name)
        );
        row.appendChild(
            elCreateText('td', {}, fmtDate(data.lastModified))
        );
        row.appendChild(
            pEl.actionPlaylistTd.cloneNode(true)
        );
    });
}

/**
 * Parses the MYMPD_API_PLAYLIST_CONTENT_LIST jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parsePlaylistsDetail(obj) {
    const table = document.getElementById('BrowsePlaylistDetailList');
    const tfoot = table.querySelector('tfoot');
    const colspan = settings.colsBrowsePlaylistDetail.length + 1;

    if (checkResultId(obj, 'BrowsePlaylistDetailList') === false) {
        return;
    }

    // set toolbar
    if (isMPDplaylist(obj.result.plist) === false) {
        // playlist in music directory
        setData(table, 'ro', true);
        elHideId('playlistContentBtns');
        elHideId('smartPlaylistContentBtns');
        table.setAttribute('data-rw', 'false');
    }
    else if (obj.result.smartpls === true) {
        // smart playlist
        setData(table, 'ro', true);
        elHideId('playlistContentBtns');
        elShowId('smartPlaylistContentBtns');
        table.setAttribute('data-rw', 'false');
    }
    else {
        // mpd playlist
        setData(table, 'ro', false);
        elShowId('playlistContentBtns');
        elHideId('smartPlaylistContentBtns');
        table.setAttribute('data-rw', 'true');
    }

    setData(table, 'playlistlength', obj.result.totalEntities);
    setData(table, 'uri', obj.result.plist);
    setData(table, 'type', obj.result.smartpls === true ? 'smartpls' : 'plist');
    table.querySelector('caption').textContent =
        (obj.result.smartpls === true ? tn('Smart playlist') : tn('Playlist')) + ': ' + obj.result.plist;
    const rowTitle = webuiSettingsDefault.clickSong.validValues[settings.webuiSettings.clickSong];

    elReplaceChild(tfoot,
        elCreateNode('tr', {"class": ["not-clickable"]},
            elCreateNode('td', {"colspan": colspan},
                elCreateNodes('small', {}, [
                    elCreateTextTnNr('span', {}, 'Num songs', obj.result.totalEntities), 
                    elCreateText('span', {}, smallSpace + nDash + smallSpace + fmtDuration(obj.result.totalTime))
                ])
            )
        )
    );

    updateTable(obj, 'BrowsePlaylistDetail', function(row, data) {
        row.setAttribute('id', 'playlistSongId' + data.Pos);
        row.setAttribute('draggable', (obj.result.smartpls === true ? 'false' : 'true'));
        row.setAttribute('tabindex', 0);
        setData(row, 'type', data.Type);
        setData(row, 'uri', data.uri);
        setData(row, 'name', data.Title);
        setData(row, 'songpos', data.Pos);
        row.setAttribute('title', tn(rowTitle));
    });
}

/**
 * Opens the playlist detail view
 * @param {string} uri shows the playlist detail view
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function playlistDetails(uri) {
    setUpdateViewId('BrowsePlaylistListList');
    appGoto('Browse', 'Playlist', 'Detail', 0, undefined, uri, '-', '-', '');
}

/**
 * Shuffles the playlist
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function playlistShuffle() {
    setUpdateViewId('BrowsePlaylistDetailList');
    sendAPI("MYMPD_API_PLAYLIST_CONTENT_SHUFFLE", {
        "plist": getDataId('BrowsePlaylistDetailList', 'uri')
    }, null, false);
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
 * Validates the playlist
 * @param {string} plist playlist to validate
 * @param {boolean} remove true = remove invalid entries, false = count number of invalid entries
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function playlistValidate(plist, remove) {
    setUpdateViewId('BrowsePlaylistDetailList');
    sendAPI("MYMPD_API_PLAYLIST_CONTENT_VALIDATE", {
        "plist": plist,
        "remove": remove
    }, playlistValidateDedupCheckError, true);
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
 * Deduplicates the playlist
 * @param {string} plist playlist to deduplicate
 * @param {boolean} remove true = remove invalid entries, false = count number of invalid entries
 * @returns {void}
 */
function playlistDedup(plist, remove) {
    setUpdateViewId('BrowsePlaylistDetailList');
    sendAPI("MYMPD_API_PLAYLIST_CONTENT_DEDUP", {
        "plist": plist,
        "remove": remove
    }, playlistValidateDedupCheckError, true);
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
 * Validates and deduplicates the playlist
 * @param {string} plist playlist to deduplicate
 * @param {boolean} remove true = remove invalid entries, false = count number of invalid entries
 * @returns {void}
 */
function playlistValidateDedup(plist, remove) {
    setUpdateViewId('BrowsePlaylistDetailList');
    sendAPI("MYMPD_API_PLAYLIST_CONTENT_VALIDATE_DEDUP", {
        "plist": plist,
        "remove": remove
    }, playlistValidateDedupCheckError, true);
}

/**
 * Handler for jsonrpc responses:
 *  - MYMPD_API_PLAYLIST_CONTENT_DEDUP
 *  - MYMPD_API_PLAYLIST_CONTENT_VALIDATE
 *  - MYMPD_API_PLAYLIST_CONTENT_VALIDATE_DEDUP
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function playlistValidateDedupCheckError(obj) {
    const alertEl = document.getElementById('playlistDetailAlert');
    unsetUpdateViewId('BrowsePlaylistDetailList');
    if (obj.error) {
        alertEl.firstElementChild.textContent = tn(obj.error.message, obj.error.data);
        elShow(alertEl);
    }
    else {
        elHide(alertEl);
    }
}

/**
 * Sorts the playlist by tag
 * @param {string} tag sort tag
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function playlistSort(tag) {
    setUpdateViewId('BrowsePlaylistDetailList');
    sendAPI("MYMPD_API_PLAYLIST_CONTENT_SORT", {
        "plist": getDataId('BrowsePlaylistDetailList', 'uri'),
        "tag": tag
    }, null, false);
}

/**
 * Updates all smart playlists
 * @param {boolean} force true = forces update of all smart playlists,
 *                        false = updates only outdated smart playlists
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function updateSmartPlaylists(force) {
    sendAPI("MYMPD_API_SMARTPLS_UPDATE_ALL", {
        "force": force
    }, null, false);
}

/**
 * Removes positions from a playlist
 * @param {string} plist the playlist
 * @param {Array} positions Positions to remove
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function removeFromPlaylistPositions(plist, positions) {
    sendAPI("MYMPD_API_PLAYLIST_CONTENT_RM_POSITIONS", {
        "plist": plist,
        "positions": positions
    }, null, false);
    setUpdateViewId('BrowsePlaylistDetailList');
}

/**
 * Removes a range from a playlist
 * @param {string} plist the playlist
 * @param {number} start Start of the range (including) / song pos
 * @param {number} [end] End playlist position (excluding), use -1 for open end
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function removeFromPlaylistRange(plist, start, end) {
    sendAPI("MYMPD_API_PLAYLIST_CONTENT_RM_RANGE", {
        "plist": plist,
        "start": start,
        "end": end
    }, null, false);
    setUpdateViewId('BrowsePlaylistDetailList');
}

/**
 * Parses the MYMPD_API_SMARTPLS_GET jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parseSmartPlaylist(obj) {
    document.getElementById('saveSmartPlaylistName').value = obj.result.plist;
    document.getElementById('saveSmartPlaylistType').value = tn(obj.result.type);
    document.getElementById('saveSmartPlaylistSort').value = obj.result.sort;
    setDataId('saveSmartPlaylistType', 'value', obj.result.type);
    elHideId('saveSmartPlaylistSearch');
    elHideId('saveSmartPlaylistSticker');
    elHideId('saveSmartPlaylistNewest');

    switch(obj.result.type) {
        case 'search':
            elShowId('saveSmartPlaylistSearch');
            document.getElementById('inputSaveSmartPlaylistExpression').value = obj.result.expression;
            break;
        case 'sticker':
            elShowId('saveSmartPlaylistSticker');
            document.getElementById('selectSaveSmartPlaylistSticker').value = obj.result.sticker;
            document.getElementById('inputSaveSmartPlaylistStickerMaxentries').value = obj.result.maxentries;
            document.getElementById('inputSaveSmartPlaylistStickerMinvalue').value = obj.result.minvalue;
            break;
        case 'newest':
            elShowId('saveSmartPlaylistNewest');
            document.getElementById('inputSaveSmartPlaylistNewestTimerange').value = obj.result.timerange / 24 / 60 / 60;
            break;
    }
    cleanupModalId('modalSaveSmartPlaylist');
    uiElements.modalSaveSmartPlaylist.show();
}

/**
 * Saves a smart playlist
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function saveSmartPlaylist() {
    cleanupModalId('modalSaveSmartPlaylist');

    const name = document.getElementById('saveSmartPlaylistName').value;
    const type = getDataId('saveSmartPlaylistType', 'value');
    const sort = getSelectValueId('saveSmartPlaylistSort');
    if (validatePlist(name) === false) {
        document.getElementById('saveSmartPlaylistName').classList.add('is-invalid');
        return;
    }

    switch(type) {
        case 'search':
            sendAPI("MYMPD_API_SMARTPLS_SEARCH_SAVE", {
                "plist": name,
                "expression": document.getElementById('inputSaveSmartPlaylistExpression').value,
                "sort": sort
            }, saveSmartPlaylistClose, true);
            break;
        case 'sticker': {
            const maxentriesEl = document.getElementById('inputSaveSmartPlaylistStickerMaxentries');
            if (!validateIntEl(maxentriesEl)) {
                return;
            }
            const minvalueEl = document.getElementById('inputSaveSmartPlaylistStickerMinvalue');
            if (!validateIntEl(minvalueEl)) {
                return;
            }
            sendAPI("MYMPD_API_SMARTPLS_STICKER_SAVE", {
                "plist": name,
                "sticker": getSelectValueId('selectSaveSmartPlaylistSticker'),
                "maxentries": Number(maxentriesEl.value),
                "minvalue": Number(minvalueEl.value),
                "sort": sort
            }, saveSmartPlaylistClose, true);
            break;
        }
        case 'newest': {
            const timerangeEl = document.getElementById('inputSaveSmartPlaylistNewestTimerange');
            if (!validateIntEl(timerangeEl)) {
                return;
            }
            sendAPI("MYMPD_API_SMARTPLS_NEWEST_SAVE", {
                "plist": name,
                "timerange": Number(timerangeEl.value) * 60 * 60 * 24,
                "sort": sort
            }, saveSmartPlaylistClose, true);
            break;
        }
        default:
            document.getElementById('saveSmartPlaylistType').classList.add('is-invalid');
    }
}

/**
 * Handles the MYMPD_API_SMARTPLS_*_SAVE responses
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function saveSmartPlaylistClose(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        uiElements.modalSaveSmartPlaylist.hide();
        showNotification(tn('Saved smart playlist'), 'playlist', 'info');
    }
}

/**
 * Adds a default smart playlist
 * @param {string} type one of mostPlayed, newest, bestRated
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addSmartpls(type) {
    const obj = {"jsonrpc": "2.0", "id": 0, "result": {"method": "MYMPD_API_SMARTPLS_GET"}};
    switch(type) {
        case 'mostPlayed':
            obj.result.plist = settings.smartplsPrefix + (settings.smartplsPrefix !== '' ? '-' : '') + 'mostPlayed';
            obj.result.type = 'sticker';
            obj.result.sticker = 'playCount';
            obj.result.maxentries = 200;
            obj.result.minvalue = 10;
            break;
        case 'newest':
            obj.result.plist = settings.smartplsPrefix + (settings.smartplsPrefix !== '' ? '-' : '') + 'newestSongs';
            obj.result.type = 'newest';
            //14 days
            obj.result.timerange = 1209600;
            break;
        case 'bestRated':
            obj.result.plist = settings.smartplsPrefix + (settings.smartplsPrefix !== '' ? '-' : '') + 'bestRated';
            obj.result.type = 'sticker';
            obj.result.sticker = 'like';
            obj.result.maxentries = 200;
            obj.result.minvalue = 2;
            break;
    }
    parseSmartPlaylist(obj);
}

/**
 * Gets playlists and populates a select
 * @param {number} type type of the playlist
 *                      0 = all playlists,
 *                      1 = static playlists,
 *                      2 = smart playlists
 * @param {string} elId select element id
 * @param {string} searchstr search string
 * @param {string} selectedPlaylist current selected playlist
 * @returns {void}
 */
function filterPlaylistsSelect(type, elId, searchstr, selectedPlaylist) {
    sendAPI("MYMPD_API_PLAYLIST_LIST", {
        "searchstr": searchstr,
        "offset": 0,
        "limit": settings.webuiSettings.uiMaxElementsPerPage,
        "type": type
    }, function(obj) {
        populatePlaylistSelect(obj, elId, selectedPlaylist);
    }, false);
}

/**
 * Populates the custom input element mympd-select-search
 * @param {object} obj jsonrpc response
 * @param {string} playlistSelectId select element id
 * @param {string} selectedPlaylist current selected playlist
 * @returns {void}
 */
function populatePlaylistSelect(obj, playlistSelectId, selectedPlaylist) {
    const selectEl = document.getElementById(playlistSelectId);
    //set input element values
    selectEl.value = selectedPlaylist === 'Database'
        ? tn('Database')
        : selectedPlaylist === ''
            ? playlistSelectId === 'selectTimerPlaylist'
                ? tn('No playlist')
                : ''
            : selectedPlaylist;
    setData(selectEl, 'value', selectedPlaylist);
    elClear(selectEl.filterResult);
    switch(playlistSelectId) {
        case 'selectTimerPlaylist':
            selectEl.addFilterResult('No playlist', '');
            break;
        case 'selectJukeboxPlaylist':
        case 'selectAddToQueuePlaylist':
            selectEl.addFilterResult('Database', 'Database');
            break;
    }

    for (let i = 0; i < obj.result.returnedEntities; i++) {
        selectEl.addFilterResultPlain(obj.result.data[i].uri);
        if (obj.result.data[i].uri === selectedPlaylist) {
            selectEl.filterResult.lastChild.classList.add('active');
        }
    }
}

/**
 * Shows the copy playlist modal
 * @param {string} srcPlists playlist to remove the entries
 * @returns {void}
 */
function showCopyPlaylist(srcPlists) {
    const modal = document.getElementById('modalCopyPlaylist');
    cleanupModal(modal);
    setData(modal, 'srcPlists', srcPlists);
    filterPlaylistsSelect(1, 'copyPlaylistPlaylist', '', '');
    uiElements.modalCopyPlaylist.show();
}

/**
 * Copies the playlist to another playlist
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function copyPlaylist() {
    const modal = document.getElementById('modalCopyPlaylist');
    cleanupModal(modal);
    const srcPlists = getData(modal, 'srcPlists');
    const mode = getRadioBoxValueId('copyPlaylistMode');
    const plistEl = document.getElementById('copyPlaylistPlaylist');
    if (validatePlistEl(plistEl) === false) {
        return;
    }
    sendAPI("MYMPD_API_PLAYLIST_COPY", {
        "srcPlists": srcPlists,
        "dstPlist": plistEl.value,
        "mode": Number(mode)
    }, copyPlaylistClose, true);
}

/**
 * Handles the response of "copy playlist" modal
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function copyPlaylistClose(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        uiElements.modalCopyPlaylist.hide();
    }
}

/**
 * Shows the move to playlist modal
 * @param {string} srcPlist playlist to remove the entries
 * @param {Array} positions song positions in srcPlist to move
 * @returns {void}
 */
function showMoveToPlaylist(srcPlist, positions) {
    const modal = document.getElementById('modalMoveToPlaylist');
    cleanupModal(modal);
    setData(modal, 'srcPlist', srcPlist);
    setData(modal, 'positions', positions);
    filterPlaylistsSelect(1, 'moveToPlaylistPlaylist', '', '');
    uiElements.modalMoveToPlaylist.show();
}

/**
 * Adds the selected elements from the "move to playlist" modal to the playlist
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function moveToPlaylist() {
    const modal = document.getElementById('modalMoveToPlaylist');
    cleanupModal(modal);
    const srcPlist = getData(modal, 'srcPlist');
    const positions = getData(modal, 'positions');
    const mode = getRadioBoxValueId('moveToPlaylistPos');
    const plistEl = document.getElementById('moveToPlaylistPlaylist');
    if (validatePlistEl(plistEl) === false) {
        return;
    }
    sendAPI("MYMPD_API_PLAYLIST_CONTENT_MOVE_TO_PLAYLIST", {
        "srcPlist": srcPlist,
        "dstPlist": plistEl.value,
        "positions": positions,
        "mode": Number(mode)
    }, moveToPlaylistClose, true);
}

/**
 * Handles the response of "move to playlist" modal
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function moveToPlaylistClose(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        uiElements.modalMoveToPlaylist.hide();
    }
}

/**
 * Shows the add to playlist modal
 * @param {Array} uris the uris or 
 *                     ["STREAM"] to add a stream
 *                     ["SEARCH"] to add a search
 *                     ["ALBUM",...albumIds] to add an album
 * @param {string} searchstr searchstring for uri[0] = SEARCH
 * @returns {void}
 */
function showAddToPlaylist(uris, searchstr) {
    cleanupModalId('modalAddToPlaylist');
    setDataId('addToPlaylistUri', 'uris', uris);
    document.getElementById('addToPlaylistSearch').value = searchstr;
    document.getElementById('addToPlaylistPlaylist').value = '';
    document.getElementById('addToPlaylistPlaylist').filterInput.value = '';
    document.getElementById('addToPlaylistPosAppend').checked = 'checked';
    document.getElementById('streamUrl').value = '';
    if (uris[0] === 'STREAM') {
        //add stream
        toggleAddToPlaylistFrm(document.getElementById('toggleAddToPlaylistQueue'));
        elShowId('addStreamFrm');
        document.getElementById('addToPlaylistCaption').textContent = tn('Add stream');
    }
    else {
        //add to playlist
        toggleAddToPlaylistFrm(document.getElementById('toggleAddToPlaylistPlaylist'));
        elHideId('addStreamFrm');
        document.getElementById('addToPlaylistCaption').textContent = tn('Add to playlist');
    }
    uiElements.modalAddToPlaylist.show();
    if (features.featPlaylists) {
        filterPlaylistsSelect(1, 'addToPlaylistPlaylist', '', '');
    }
}

/**
 * Toggles the view in the add to playlist modal
 * @param {EventTarget} target event target
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function toggleAddToPlaylistFrm(target) {
    toggleBtnGroup(target);
    if (target.getAttribute('id') === 'toggleAddToPlaylistPlaylist') {
        //add to playlist
        elShowId('addToPlaylistFrm');
        elShowId('addToPlaylistPosInsertFirstRow');
        elHideId('addToPlaylistPosInsertRow');
        elHideId('addToPlaylistPosAppendPlayRow');
        elHideId('addToPlaylistPosReplacePlayRow');
    }
    else {
        //add to queue
        elHideId('addToPlaylistFrm');
        elHideId('addToPlaylistPosInsertFirstRow');
        elShowId('addToPlaylistPosInsertRow');
        elShowId('addToPlaylistPosAppendPlayRow');
        elShowId('addToPlaylistPosReplacePlayRow');
    }
}

/**
 * Adds the selected elements from the "add to playlist" modal to the playlist or queue
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addToPlaylist() {
    cleanupModalId('modalAddToPlaylist');
    const uris = getDataId('addToPlaylistUri', 'uris');
    const mode = getRadioBoxValueId('addToPlaylistPos');
    let type;
    switch(uris[0]) {
        case 'SEARCH':
            uris[0] = document.getElementById('addToPlaylistSearch').value;
            type = 'search';
            break;
        case 'ALBUM':
            type = 'album';
            uris.shift();
            break;
        case 'STREAM': {
            const streamUrlEl = document.getElementById('streamUrl');
            if (validateStreamEl(streamUrlEl) === false) {
                return;
            }
            uris[0] = streamUrlEl.value;
            type = 'stream';
            break;
        }
        default:
            //files and dirs
            type = 'song';
    }

    if (document.getElementById('addToPlaylistFrm').classList.contains('d-none') === false) {
        //add to playlist
        const plistEl = document.getElementById('addToPlaylistPlaylist');
        if (validatePlistEl(plistEl) === false) {
            return;
        }
        switch(mode) {
            case 'append':
                appendPlaylist(type, uris, plistEl.value, addToPlaylistClose);
                break;
            case 'insertFirst':
                insertPlaylist(type, uris, plistEl.value, 0, addToPlaylistClose);
                break;
            case 'replace':
                replacePlaylist(type, uris, plistEl.value, addToPlaylistClose);
                break;
        }
    }
    else {
        //add to queue
        switch(mode) {
            case 'append':
                appendQueue(type, uris, addToPlaylistClose);
                break;
            case 'appendPlay':
                appendPlayQueue(type, uris, addToPlaylistClose);
                break;
            case 'insertAfterCurrent':
                insertAfterCurrentQueue(type, uris, addToPlaylistClose);
                break;
            case 'insertPlayAfterCurrent':
                insertPlayAfterCurrentQueue(type, uris, addToPlaylistClose);
                break;
            case 'replace':
                replaceQueue(type, uris, addToPlaylistClose);
                break;
            case 'replacePlay':
                replacePlayQueue(type, uris, addToPlaylistClose);
                break;
        }
    }
}

/**
 * Handles the response of "add to playlist" modal
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function addToPlaylistClose(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        uiElements.modalAddToPlaylist.hide();
    }
}

/**
 * Appends entries to a playlist
 * @param {string} type one of song, stream, dir, search
 * @param {Array} uris uris to add
 * @param {string} plist playlist to append the uri
 * @param {Function} callback response handling callback
 * @returns {void}
 */
function appendPlaylist(type, uris, plist, callback) {
    switch(type) {
        case 'song':
        case 'stream':
        case 'dir':
            sendAPI("MYMPD_API_PLAYLIST_CONTENT_APPEND_URIS", {
                "uris": uris,
                "plist": plist
            }, callback, true);
            break;
        case 'search':
            sendAPI("MYMPD_API_PLAYLIST_CONTENT_APPEND_SEARCH", {
                "expression": uris[0],
                "plist": plist
            }, callback, true);
            break;
        case 'album':
            sendAPI("MYMPD_API_PLAYLIST_CONTENT_APPEND_ALBUMS", {
                "albumids": uris,
                "plist": plist
            }, callback, true);
            break;
    }
}

/**
 * Inserts entries into a playlist
 * @param {string} type one of song, stream, dir, search
 * @param {Array} uris uris to add
 * @param {string} plist playlist to insert the uri
 * @param {number} to position to insert
 * @param {Function} callback response handling callback
 * @returns {void}
 */
function insertPlaylist(type, uris, plist, to, callback) {
    switch(type) {
        case 'song':
        case 'stream':
        case 'dir':
            sendAPI("MYMPD_API_PLAYLIST_CONTENT_INSERT_URIS", {
                "uris": uris,
                "plist": plist,
                "to": to
            }, callback, true);
            break;
        case 'search':
            sendAPI("MYMPD_API_PLAYLIST_CONTENT_INSERT_SEARCH", {
                "expression": uris[0],
                "plist": plist,
                "to": to
            }, callback, true);
            break;
        case 'album':
            sendAPI("MYMPD_API_PLAYLIST_CONTENT_INSERT_ALBUMS", {
                "albumids": uris,
                "plist": plist
            }, callback, true);
            break;
    }
}

/**
 * Replaces a playlist
 * @param {string} type one of song, stream, dir, search
 * @param {Array} uris uris to add
 * @param {string} plist playlist to replace
 * @param {Function} callback response handling callback
 * @returns {void}
 */
function replacePlaylist(type, uris, plist, callback) {
    switch(type) {
        case 'song':
        case 'stream':
        case 'dir':
            sendAPI("MYMPD_API_PLAYLIST_CONTENT_REPLACE_URIS", {
                "uris": uris,
                "plist": plist
            }, callback, true);
            break;
        case 'search':
            sendAPI("MYMPD_API_PLAYLIST_CONTENT_REPLACE_SEARCH", {
                "expression": uris[0],
                "plist": plist
            }, callback, true);
            break;
        case 'album':
            sendAPI("MYMPD_API_PLAYLIST_CONTENT_REPLACE_ALBUMS", {
                "albumids": uris,
                "plist": plist
            }, callback, true);
            break;
    }
}

/**
 * Shows the rename playlist modal
 * @param {string} from original playlist name
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showRenamePlaylist(from) {
    cleanupModalId('modalRenamePlaylist');
    document.getElementById('renamePlaylistFrom').value = from;
    document.getElementById('renamePlaylistTo').value = '';
    uiElements.modalRenamePlaylist.show();
}

/**
 * Renames the playlist
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function renamePlaylist() {
    cleanupModalId('modalRenamePlaylist');

    const from = document.getElementById('renamePlaylistFrom').value;
    const to = document.getElementById('renamePlaylistTo').value;
    if (to !== from && validatePlist(to) === true) {
        sendAPI("MYMPD_API_PLAYLIST_RENAME", {
            "plist": from,
            "newName": to
        }, renamePlaylistClose, true);
    }
    else {
        document.getElementById('renamePlaylistTo').classList.add('is-invalid');
    }
}

/**
 * Handles the MYMPD_API_PLAYLIST_RENAME jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function renamePlaylistClose(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        uiElements.modalRenamePlaylist.hide();
    }
}

/**
 * Shows the settings of the smart playlist
 * @param {string} plist smart playlist name
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showSmartPlaylist(plist) {
    sendAPI("MYMPD_API_SMARTPLS_GET", {
        "plist": plist
    }, parseSmartPlaylist, false);
}

/**
 * Updates a smart playlist
 * @param {string} plist smart playlist name
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function updateSmartPlaylist(plist) {
    sendAPI("MYMPD_API_SMARTPLS_UPDATE", {
        "plist": plist
    }, null, false);
}

/**
 * Click handler for update smart playlist
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function updateSmartPlaylistClick() {
    setUpdateViewId('BrowsePlaylistDetailList');
    updateSmartPlaylist(getDataId('BrowsePlaylistDetailList', 'uri'));
}

/**
 * Click handler for edit smart playlist
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function editSmartPlaylistClick() {
    showSmartPlaylist(getDataId('BrowsePlaylistDetailList', 'uri'));
}

/**
 * Deletes playlists and shows a confirmation modal before
 * @param {Array} plists playlists to delete
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showDelPlaylist(plists) {
    showConfirm(tn('Do you really want to delete the playlist?', {"playlist": joinArray(plists)}), tn('Yes, delete it'), function() {
        sendAPI("MYMPD_API_PLAYLIST_RM", {
            "plists": plists
        }, null, false);
    });
}

/**
 * Clears a playlist and shows a confirmation modal before
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showClearPlaylist() {
    const plist = getDataId('BrowsePlaylistDetailList', 'uri');
    showConfirm(tn('Do you really want to clear the playlist?', {"playlist": plist}), tn('Yes, clear it'), function() {
        sendAPI("MYMPD_API_PLAYLIST_CONTENT_CLEAR", {
            "plist": plist
        }, null, false);
        setUpdateViewId('BrowsePlaylistDetailList');
    });
}

/**
 * Moves a song in the current displayed playlist
 * @param {number} from from position
 * @param {number} to to position
 * @returns {void}
 */
function playlistMoveSong(from, to) {
    sendAPI("MYMPD_API_PLAYLIST_CONTENT_MOVE_POSITION", {
        "plist": app.current.filter,
        "from": from,
        "to": to
    }, null, false);
}

/**
 * Checks if the playlist is a stored playlist of mpd
 * @param {string} uri playlist uri
 * @returns {boolean} true = stored playlist, false = playlist in music directory
 */
function isMPDplaylist(uri) {
    if (uri.charAt(1) === '/' ||
        uri.match(/\.(m3u|pls|asx|xspf)/) !== null)
    {
        return false;
    }
    return true;
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
            appendQueue(type, uri);
            break;
        case 'appendPlayQueue':
            appendPlayQueue(type, uri);
            break;
        case 'insertAfterCurrentQueue':
            insertAfterCurrentQueue(type, uri, null);
            break;
        case 'replaceQueue':
            replaceQueue(type, uri);
            break;
        case 'replacePlayQueue':
            replacePlayQueue(type, uri);
            break;
        case 'addToHome':
            addPlistToHome(uri, type, uri);
            break;
    }
}
