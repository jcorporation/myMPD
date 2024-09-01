"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module queue_js */

/**
 * Removes a song range from the queue
 * @param {number} start start of the range (including)
 * @param {number} [end] end of the range (excluding), -1 for open end
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function removeFromQueueRange(start, end) {
    sendAPI("MYMPD_API_QUEUE_RM_RANGE", {
        "start": start,
        "end": end
    }, null, false);
}

/**
 * Removes song ids from the queue
 * @param {Array} ids MPD queue song ids
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function removeFromQueueIDs(ids) {
    sendAPI("MYMPD_API_QUEUE_RM_IDS", {
        "songIds": ids
    }, null, false);
}

/**
 * Moves a entry in the queue
 * @param {number} from from position
 * @param {number} to to position
 * @returns {void}
 */
function queueMoveSong(from, to) {
    sendAPI("MYMPD_API_QUEUE_MOVE_POSITION", {
        "from": from,
        "to": to
    }, null, false);
}

/**
 * Plays the selected song(s) after the current song.
 * Sets the priority if MPD is in random mode, else moves the song(s) after current playing song.
 * @param {Array} songIds current playing song ids
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function playAfterCurrent(songIds) {
    if (settings.partition.random === false) {
        //not in random mode - move song after current playing song
        sendAPI("MYMPD_API_QUEUE_MOVE_RELATIVE", {
            "songIds": songIds,
            "to": 0,
            "whence": 1
        }, null, false);
    }
    else {
        //in random mode - set song priority
        sendAPI("MYMPD_API_QUEUE_PRIO_SET_HIGHEST", {
            "songIds": songIds
        }, null, false);
    }
}

/**
 * Clears or crops the queue after confirmation
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function clearQueue() {
    showConfirm(tn('Do you really want to clear the queue?'), tn('Yes, clear it'), function() {
        sendAPI("MYMPD_API_QUEUE_CROP_OR_CLEAR", {}, null, false);
    });
}

/**
 * Appends an element to the queue
 * @param {string} type element type: song, dir, stream, plist, smartpls, webradio, search, album, disc, searchdir
 * @param {Array} uris element uris
 * @param {Function} [callback] callback function
 * @returns {void}
 */
function appendQueue(type, uris, callback) {
    _appendQueue(type, uris, false, callback);
}

/**
 * Appends an element to the queue and plays it
 * @param {string} type element type: song, dir, stream, plist, smartpls, webradio, search, album, disc, searchdir
 * @param {Array} uris element uris
 * @param {Function} [callback] callback function
 * @returns {void}
 */
function appendPlayQueue(type, uris, callback) {
    _appendQueue(type, uris, true, callback);
}

/**
 * Appends elements to the queue
 * @param {string} type element type: song, dir, stream, plist, smartpls, webradio, search, album, disc, searchdir
 * @param {Array} uris element uris
 * @param {boolean} play true = play added entry, false = append only
 * @param {Function} callback callback function
 * @returns {void}
 */
function _appendQueue(type, uris, play, callback) {
    if (type === 'webradio') {
        uris = getRadioFavoriteUris(uris);
    }
    switch(type) {
        case 'searchdir':
            sendAPI("MYMPD_API_QUEUE_APPEND_SEARCH", {
                "expression": createBaseSearchExpression(uris[0], uris[1]),
                "sort": uris[2],
                "sortdesc": uris[3],
                "play": play
            }, callback, true);
            break;
        case 'song':
        case 'dir':
        case 'stream':
            sendAPI("MYMPD_API_QUEUE_APPEND_URIS", {
                "uris": uris,
                "play": play
            }, callback, true);
            break;
        case 'plist':
        case 'smartpls':
        case 'webradio':
            sendAPI("MYMPD_API_QUEUE_APPEND_PLAYLISTS", {
                "plists": uris,
                "play": play
            }, callback, true);
            break;
        case 'search':
            //search is limited to one at a time
            sendAPI("MYMPD_API_QUEUE_APPEND_SEARCH", {
                "expression": uris[0],
                "sort": uris[1],
                "sortdesc": uris[2],
                "play": play
            }, callback, true);
            break;
        case 'album':
            sendAPI("MYMPD_API_QUEUE_APPEND_ALBUMS", {
                "albumids": uris,
                "play": play
            }, callback, true);
            break;
        case 'disc':
            //disc is limited to one at a time
            sendAPI("MYMPD_API_QUEUE_APPEND_ALBUM_DISC", {
                "albumid": uris[0],
                "disc": uris[1].toString(),
                "play": play
            }, callback, true);
            break;
        default:
            logError('Invalid type: ' + type);
    }
}

/**
 * Inserts the element after the current playing song
 * @param {string} type element type: song, dir, stream, plist, smartpls, webradio, search, album, disc, searchdir
 * @param {Array} uris element uris
 * @param {Function} [callback] callback function
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function insertAfterCurrentQueue(type, uris, callback) {
    insertQueue(type, uris, 0, 1, false, callback);
}

/**
 * Inserts the element after the current playing song
 * @param {string} type element type: song, dir, stream, plist, smartpls, webradio, search, album, disc, searchdir
 * @param {Array} uris element uris
 * @param {Function} [callback] callback function
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function insertPlayAfterCurrentQueue(type, uris, callback) {
    insertQueue(type, uris, 0, 1, true, callback);
}

/**
 * Inserts elements into the queue
 * @param {string} type element type: song, dir, stream, plist, smartpls, webradio, search, album, disc, searchdir
 * @param {Array} uris element uris
 * @param {number} to position to insert
 * @param {number} whence how to interpret the to parameter: 0 = absolute, 1 = after, 2 = before current song
 * @param {boolean} play true = play added entry, false = insert only
 * @param {Function} callback callback function
 * @returns {void}
 */
function insertQueue(type, uris, to, whence, play, callback) {
    if (type === 'webradio') {
        uris = getRadioFavoriteUris(uris);
    }
    switch(type) {
        case 'searchdir':
            sendAPI("MYMPD_API_QUEUE_INSERT_SEARCH", {
                "expression": createBaseSearchExpression(uris[0], uris[1]),
                "to": to,
                "whence": whence,
                "sort": uris[2],
                "sortdesc": uris[3],
                "play": play
            }, callback, true);
            break;
        case 'song':
        case 'dir':
        case 'stream':
            sendAPI("MYMPD_API_QUEUE_INSERT_URIS", {
                "uris": uris,
                "to": to,
                "whence": whence,
                "play": play
            }, callback, true);
            break;
        case 'plist':
        case 'smartpls':
        case 'webradio':
            sendAPI("MYMPD_API_QUEUE_INSERT_PLAYLISTS", {
                "plists": uris,
                "to": to,
                "whence": whence,
                "play": play
            }, callback, true);
            break;
        case 'search':
            //search is limited to one at a time
            sendAPI("MYMPD_API_QUEUE_INSERT_SEARCH", {
                "expression": uris[0],
                "to": to,
                "whence": whence,
                "sort": uris[1],
                "sortdesc": uris[2],
                "play": play
            }, callback, true);
            break;
        case 'album':
            sendAPI("MYMPD_API_QUEUE_INSERT_ALBUMS", {
                "albumids": uris,
                "to": to,
                "whence": whence,
                "play": play
            }, callback, true);
            break;
        case 'disc':
            sendAPI("MYMPD_API_QUEUE_INSERT_ALBUM_DISC", {
                "albumid": uris[0],
                "disc": uris[1].toString(),
                "to": to,
                "whence": whence,
                "play": play
            }, callback, true);
            break;
        default:
            logError('Invalid type: ' + type);
    }
}

/**
 * Replaces the queue with the element
 * @param {string} type element type: song, dir, stream, plist, smartpls, webradio, search, album, disc, searchdir
 * @param {Array} uris element uris
 * @param {Function} [callback] callback function
 * @returns {void}
 */
function replaceQueue(type, uris, callback) {
    _replaceQueue(type, uris, false, callback);
}

/**
 * Replaces the queue with the element and plays it
 * @param {string} type element type: song, dir, stream, plist, smartpls, webradio, search, album, disc, searchdir
 * @param {Array} uris element uris
 * @param {Function} [callback] callback function
 * @returns {void}
 */
function replacePlayQueue(type, uris, callback) {
    _replaceQueue(type, uris, true, callback);
}

/**
 * Replaces the queue with the elements
 * @param {string} type element type: song, dir, stream, plist, smartpls, webradio, search, album, disc, searchdir
 * @param {Array} uris element uris
 * @param {boolean} play true = play added entry, false = insert only
 * @param {Function} callback callback function
 * @returns {void}
 */
function _replaceQueue(type, uris, play, callback) {
    if (type === 'webradio') {
        uris = getRadioFavoriteUris(uris);
    }
    switch(type) {
        case 'searchdir':
            sendAPI("MYMPD_API_QUEUE_REPLACE_SEARCH", {
                "expression": createBaseSearchExpression(uris[0], uris[1]),
                "sort": uris[2],
                "sortdesc": uris[3],
                "play": play
            }, callback, true);
            break;
        case 'song':
        case 'stream':
        case 'dir':
            sendAPI("MYMPD_API_QUEUE_REPLACE_URIS", {
                "uris": uris,
                "play": play
            }, callback, true);
            break;
        case 'plist':
        case 'smartpls':
        case 'webradio':
            sendAPI("MYMPD_API_QUEUE_REPLACE_PLAYLISTS", {
                "plists": uris,
                "play": play
            }, callback, true);
            break;
        case 'search':
            //search is limited to one at a time
            sendAPI("MYMPD_API_QUEUE_REPLACE_SEARCH", {
                "expression": uris[0],
                "sort": uris[1],
                "sortdesc": uris[2],
                "play": play
            }, callback, true);
            break;
        case 'album':
            sendAPI("MYMPD_API_QUEUE_REPLACE_ALBUMS", {
                "albumids": uris,
                "play": play
            }, callback, true);
            break;
        case 'disc':
            sendAPI("MYMPD_API_QUEUE_REPLACE_ALBUM_DISC", {
                "albumid": uris[0],
                "disc": uris[1].toString(),
                "play": play
            }, callback, true);
            break;
        default:
            logError('Invalid type: ' + type);
    }
}

/**
 * Resume song API
 * @param {string} uri Song uri
 * @param {string} action Action
 * @returns {void}
 */
function resumeSong(uri, action) {
    switch(action) {
        case 'append':
        case 'appendPlay':
            sendAPI("MYMPD_API_QUEUE_APPEND_URI_RESUME", {
                'uri': uri
            }, null, false);
            break;
        case 'insert':
            sendAPI('MYMPD_API_QUEUE_INSERT_URI_RESUME', {
                'uri': uri,
                'to': 0,
                'whence': 0
            }, null, false);
            break;
        case 'insertAfterCurrent':
        case 'insertPlayAfterCurrent':
            sendAPI('MYMPD_API_QUEUE_INSERT_URI_RESUME', {
                'uri': uri,
                'to': 0,
                'whence': 1
            }, null, false);
            break;
        case 'replace':
        case 'replacePlay':
            sendAPI("MYMPD_API_QUEUE_APPEND_URI_RESUME", {
                'uri': uri
            }, null, false);
            break;
        // No default
    }
}
