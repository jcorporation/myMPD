"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
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
        "cols": settings.colsBrowsePlaylistDetailFetch
    }, parsePlaylistDetail, true);
}

/**
 * Handles BrowsePlaylistList
 * @returns {void}
 */
function handleBrowsePlaylistList() {
    handleSearchSimple('BrowsePlaylistList');

    sendAPI("MYMPD_API_PLAYLIST_LIST", {
        "offset": app.current.offset,
        "limit": app.current.limit,
        "searchstr": app.current.search,
        "type": 0
    }, parsePlaylistList, true);
    elHideId('playlistDetailAlert');
}

/**
 * Initializes the playlist elements
 * @returns {void}
 */
function initViewPlaylists() {
    document.getElementById('BrowsePlaylistDetailSortTagsDropdown').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            event.preventDefault();
            currentPlaylistSort(getData(event.target, 'tag'));
        }
    }, false);

    initSearchSimple('BrowsePlaylistList');
    initSearchExpression('BrowsePlaylistDetail');

    document.getElementById('BrowsePlaylistListList').addEventListener('click', function(event) {
        const target = tableClickHandler(event);
        if (target !== null) {
            if (getData(target, 'smartpls-only') === false) {
                clickPlaylist(getData(target, 'uri'), event);
            }
            else {
                showNotification(tn('Playlist is empty'), 'playlist', 'warn');
            }
        }
    }, false);

    document.getElementById('BrowsePlaylistDetailList').addEventListener('click', function(event) {
        const target = tableClickHandler(event);
        if (target !== null) {
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

    const rowTitle = settingsWebuiFields.clickPlaylist.validValues[settings.webuiSettings.clickPlaylist];
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
function parsePlaylistDetail(obj) {
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
    }

    setData(table, 'playlistlength', obj.result.totalEntities);
    setData(table, 'uri', obj.result.plist);
    setData(table, 'type', obj.result.smartpls === true ? 'smartpls' : 'plist');
    table.querySelector('caption').textContent =
        (obj.result.smartpls === true ? tn('Smart playlist') : tn('Playlist')) + ': ' + obj.result.plist;
    const rowTitle = settingsWebuiFields.clickSong.validValues[settings.webuiSettings.clickSong];

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
        setData(row, 'pos', data.Pos);
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
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function currentPlaylistSort(tag) {
    setUpdateViewId('BrowsePlaylistDetailList');
    sendAPI("MYMPD_API_PLAYLIST_CONTENT_SORT", {
        "plist": getDataId('BrowsePlaylistDetailList', 'uri'),
        "tag": tag
    }, null, false);
}

/**
 * Moves a song in the current displayed playlist
 * @param {number} from from position
 * @param {number} to to position
 * @returns {void}
 */
function currentPlaylistMoveSong(from, to) {
    sendAPI("MYMPD_API_PLAYLIST_CONTENT_MOVE_POSITION", {
        "plist": app.current.filter,
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
