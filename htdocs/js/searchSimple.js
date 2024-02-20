"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module searchSimple_js */

/**
 * Update the search elements for specified appid
 * @param {string} appid the application id
 * @returns {void}
 */
function handleSearchSimple(appid) {
    const searchStrEl = elGetById(appid + 'SearchStr');
    setFocus(searchStrEl);
    if (searchStrEl.value === '' &&
        app.current.search !== '')
    {
        searchStrEl.value = app.current.search;
    }
}

/**
 * Initializes search elements for specified appid
 * @param {string} appid the application id
 * @returns {void}
 */
function initSearchSimple(appid) {
    elGetById(appid + 'SearchStr').addEventListener('keydown', function(event) {
        //handle Enter key on keydown for IME composing compatibility
        if (event.key !== 'Enter') {
            return;
        }
        clearSearchTimer();
        const value = this.value;
        searchTimer = setTimeout(function() {
            execSearchSimple(value);
        }, searchTimerTimeout);
    }, false);

    // Android does not support search on type
    if (userAgentData.isAndroid === false) {
        elGetById(appid + 'SearchStr').addEventListener('keyup', function(event) {
            if (ignoreKeys(event) === true) {
                return;
            }
            clearSearchTimer();
            const value = this.value;
            searchTimer = setTimeout(function() {
                execSearchSimple(value);
            }, searchTimerTimeout);
        }, false);
    }
}

/**
 * Executes the simple search for the current displayed view
 * @param {string} value search string
 * @returns {void}
 */
function execSearchSimple(value) {
    appGoto(app.current.card, app.current.tab, app.current.view,
        0, app.current.limit, app.current.filter, app.current.sort, app.current.tag, value);
}
