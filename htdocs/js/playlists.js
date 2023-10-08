"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module playlists_js */

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
    const alertEl = elGetById('playlistDetailAlert');
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
        "limit": settings.webuiSettings.maxElementsPerPage,
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
    const selectEl = elGetById(playlistSelectId);
    //set input element values
    selectEl.value = selectedPlaylist === 'Database'
        ? tn('Database')
        : selectedPlaylist === ''
            ? playlistSelectId === 'modalTimerPlaylistInput'
                ? tn('No playlist')
                : ''
            : selectedPlaylist;
    setData(selectEl, 'value', selectedPlaylist);
    elClear(selectEl.filterResult);
    switch(playlistSelectId) {
        case 'modalTimerPlaylistInput':
            selectEl.addFilterResult('No playlist', '');
            break;
        case 'modalPlaybackJukeboxPlaylistInput':
        case 'modalQueueAddToPlaylistInput':
            selectEl.addFilterResult('Database', 'Database');
            break;
        // No Default
    }

    for (let i = 0; i < obj.result.returnedEntities; i++) {
        selectEl.addFilterResultPlain(obj.result.data[i].uri);
        if (obj.result.data[i].uri === selectedPlaylist) {
            selectEl.filterResult.lastChild.classList.add('active');
        }
    }
}

/**
 * Appends entries to a playlist
 * @param {string} type one of song, stream, dir, search, album, disc, searchdir
 * @param {Array} uris uris to add
 * @param {string} plist playlist to append the uri
 * @param {Function} callback response handling callback
 * @returns {void}
 */
function appendPlaylist(type, uris, plist, callback) {
    switch(type) {
        case 'searchdir':
            sendAPI("MYMPD_API_PLAYLIST_CONTENT_APPEND_SEARCH", {
                "expression": createBaseSearchExpression(uris[0], uris[1]),
                "plist": plist
            }, callback, true);
            break;
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
        case 'disc':
            sendAPI("MYMPD_API_PLAYLIST_CONTENT_APPEND_ALBUM_DISC", {
                "albumid": uris[0],
                "disc": uris[1].toString(),
                "plist": plist
            }, callback, true);
            break;
        default:
            logError('Invalid type: ' + type);
    }
}

/**
 * Inserts entries into a playlist
 * @param {string} type one of song, stream, dir, search, album, disc, searchdir
 * @param {Array} uris uris to add
 * @param {string} plist playlist to insert the uri
 * @param {number} to position to insert
 * @param {Function} callback response handling callback
 * @returns {void}
 */
function insertPlaylist(type, uris, plist, to, callback) {
    switch(type) {
        case 'searchdir':
            sendAPI("MYMPD_API_PLAYLIST_CONTENT_INSERT_SEARCH", {
                "expression": createBaseSearchExpression(uris[0], uris[1]),
                "plist": plist
            }, callback, true);
            break;
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
                "plist": plist,
                "to": to
            }, callback, true);
            break;
        case 'disc':
            sendAPI("MYMPD_API_PLAYLIST_CONTENT_INSERT_ALBUM_DISC", {
                "albumid": uris[0],
                "disc": uris[1].toString(),
                "plist": plist,
                "to": to
            }, callback, true);
            break;
        default:
            logError('Invalid type: ' + type);
    }
}

/**
 * Replaces a playlist
 * @param {string} type one of song, stream, dir, search, album, disc, searchdir
 * @param {Array} uris uris to add
 * @param {string} plist playlist to replace
 * @param {Function} callback response handling callback
 * @returns {void}
 */
function replacePlaylist(type, uris, plist, callback) {
    switch(type) {
        case 'searchdir':
            sendAPI("MYMPD_API_PLAYLIST_CONTENT_REPLACE_SEARCH", {
                "expression": createBaseSearchExpression(uris[0], uris[1]),
                "plist": plist
            }, callback, true);
            break;
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
        case 'disc':
            sendAPI("MYMPD_API_PLAYLIST_CONTENT_REPLACE_ALBUM_DISC", {
                "albumid": uris[0],
                "disc": uris[1].toString(),
                "plist": plist
            }, callback, true);
            break;
        default:
            logError('Invalid type: ' + type);
    }
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
