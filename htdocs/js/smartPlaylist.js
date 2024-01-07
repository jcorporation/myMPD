"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module smartPlaylist_js */

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
