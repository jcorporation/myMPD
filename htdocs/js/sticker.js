"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module sort_js */

/**
 * Calls the MYMPD_API_STICKER_RESET_PLAYED API
 * @param {string} uri Song uri
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function resetStickerPlayed(uri) {
    sendAPI("MYMPD_API_STICKER_RESET_PLAYED", {
        "uri": uri
    }, null, false);
}

/**
 * Calls the MYMPD_API_STICKER_SET_PLAYED API
 * @param {string} uri Song uri
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function setStickerPlayed(uri) {
    sendAPI("MYMPD_API_STICKER_SET_PLAYED", {
        "uri": uri
    }, null, false);
}
