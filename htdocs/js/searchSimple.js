"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
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
 * Initializes simple search elements for specified appid
 * @param {string} appid the application id
 * @returns {void}
 */
function initSearchSimple(appid) {
    initSearchSimpleInput(elGetById(appid + 'SearchStr'), execSearchSimple);
}

/**
 * Initializes simple search element
 * @param {Object} el Element to initialize
 * @param {function} cb Callback
 * @returns {void}
 */
function initSearchSimpleInput(el, cb) {
    el.addEventListener('keydown', function(event) {
        //handle Enter key on keydown for IME composing compatibility
        if (event.key !== 'Enter') {
            return;
        }
        clearSearchTimer();
        cb(event.target.value);
    }, false);

    // Android does not support search on type
    if (userAgentData.isAndroid === false) {
        el.addEventListener('keyup', function(event) {
            if (ignoreKeys(event) === true) {
                return;
            }
            clearSearchTimer();
            const value = event.target.value;
            searchTimer = setTimeout(function() {
                cb(value);
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
