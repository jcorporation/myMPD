"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module song_js */

/**
 * Song love/hate event handler
 * @param {EventTarget} el triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function voteSong(el) {
    if (el.nodeName === 'DIV') {
        return;
    }
    let vote = Number(el.getAttribute('data-vote'));
    if (vote === 0 &&
        el.classList.contains('active'))
    {
        vote = 1;
        el.classList.remove('active');
    }
    else if (vote === 2 &&
             el.classList.contains('active'))
    {
        vote = 1;
        el.classList.remove('active');
    }
    const aEl = el.parentNode.querySelector('.active');
    if (aEl !== null) {
        aEl.classList.remove('active');
    }
    if (vote === 0 ||
        vote === 2)
    {
        el.classList.add('active');
    }
    let uri = getData(el.parentNode, 'uri');
    if (uri === undefined) {
        //fallback to current song
        uri = getDataId('PlaybackTitle', 'uri');
    }
    sendAPI("MYMPD_API_LIKE", {
        "uri": uri,
        "like": vote
    }, null, false);
}
