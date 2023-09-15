"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module album_js */

/**
 * Handles single disc actions
 * @param {string} action action to perform
 * @param {string} albumId the album id
 * @param {string} disc disc number as string
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addAlbumDisc(action, albumId, disc) {
    switch(action) {
        case 'appendQueue':
            appendQueue('disc', [albumId, disc]);
            break;
        case 'appendPlayQueue':
            appendPlayQueue('disc', [albumId, disc]);
            break;
        case 'insertAfterCurrentQueue':
            insertAfterCurrentQueue('disc', [albumId, disc]);
            break;
        case 'replaceQueue':
            replaceQueue('disc', [albumId, disc]);
            break;
        case 'replacePlayQueue':
            replacePlayQueue('disc', [albumId, disc]);
            break;
        case 'addPlaylist':
            showAddToPlaylist('disc', [albumId, disc]);
            break;
        default:
            logError('Invalid action: ' + action);
    }
}
