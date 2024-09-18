"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module album_js */

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
