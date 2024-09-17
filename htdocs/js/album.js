"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module album_js */

/**
 * Handles album filtered by tag
 * @param {string} action action to perform
 * @param {string} albumId the album id
 * @param {string} tag MPD tag
 * @param {string} value MPD tag value
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addAlbumTag(action, albumId, tag, value) {
    switch(action) {
        case 'appendQueue':
            appendQueue(tag, [albumId, value]);
            break;
        case 'appendPlayQueue':
            appendPlayQueue(tag, [albumId, value]);
            break;
        case 'insertAfterCurrentQueue':
            insertAfterCurrentQueue(tag, [albumId, value]);
            break;
        case 'replaceQueue':
            replaceQueue(tag, [albumId, value]);
            break;
        case 'replacePlayQueue':
            replacePlayQueue(tag, [albumId, value]);
            break;
        case 'addPlaylist':
            showAddToPlaylist(tag, [albumId, value]);
            break;
        default:
            logError('Invalid action: ' + action);
    }
}

/**
 * Resume album API implementation.
 * Load the album from last played song and start playing.
 * @param {string} albumId Album ID
 * @param {number} pos Position of first song to resume
 * @param {string} action Action
 * @returns {void}
 */
function resumeAlbum(albumId, pos, action) {
    pos++;
    switch(action) {
        case 'append':
        case 'appendPlay':
            sendAPI("MYMPD_API_QUEUE_APPEND_ALBUM_RANGE", {
                'albumid': albumId,
                'start': pos,
                'end': -1,
                'play': true
            }, null, false);
            break;
        case 'insert':
            sendAPI('MYMPD_API_QUEUE_INSERT_ALBUM_RANGE', {
                'albumid': albumId,
                'start': pos,
                'end': -1,
                'play': true,
                'to': 0,
                'whence': 0
            }, null, false);
            break;
        case 'insertAfterCurrent':
        case 'insertPlayAfterCurrent':
            sendAPI('MYMPD_API_QUEUE_INSERT_ALBUM_RANGE', {
                'albumid': albumId,
                'start': pos,
                'end': -1,
                'play': true,
                'to': 0,
                'whence': 1
            }, null, false);
            break;
        case 'replace':
        case 'replacePlay':
            sendAPI("MYMPD_API_QUEUE_REPLACE_ALBUM_RANGE", {
                'albumid': albumId,
                'start': pos,
                'end': -1,
                'play': true
            }, null, false);
            break;
        // No default
    }
}
