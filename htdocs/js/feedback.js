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
function voteSongLike(el) {
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

/**
 * Creates the songs hate/love elements
 * @param {number} like like value 0 - 2
 * @returns {HTMLElement} div element
 */
function createLike(like) {
    const thDown = elCreateText('button', {"data-vote": "0", "data-title-phrase": "Hate song", "class": ["btn", "btn-sm", "btn-secondary", "mi"]}, 'thumb_down');
    if (like === 0) {
        thDown.classList.add('active');
    }
    const thUp = elCreateText('button', {"data-vote": "2", "data-title-phrase": "Love song", "class": ["btn", "btn-sm", "btn-secondary", "mi"]}, 'thumb_up');
    if (like === 2) {
        thUp.classList.add('active');
    }
    return elCreateNodes('div', {"class": ["btn-group", "btn-group-sm"]}, [
        thDown,
        thUp
    ]);
}

/**
 * Song rating event handler
 * @param {EventTarget} el triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function voteSongRating(el) {
    let rating = Number(el.getAttribute('data-vote'));
    let uri = getData(el.parentNode, 'uri');
    if (uri === undefined) {
        //fallback to current song
        uri = getDataId('PlaybackTitle', 'uri');
    }
    setRating(el.parentNode, rating);

    sendAPI("MYMPD_API_RATING", {
        "uri": uri,
        "rating": rating
    }, null, false);
}

/**
 * Sets the star rating element
 * @param {HTMLElement} el container for the stars
 * @param {number} rating the rating (0-10)
 * @returns {void}
 */
function setRating(el, rating) {
    const starEls = el.querySelectorAll('button');
    for (let i = 1; i < 6; i++) {
        starEls[i].textContent = rating >= i * 2
            ? ligatures.stared
            : ligatures.star;
    }
    if (rating === 0) {
        elDisable(starEls[0]);
    }
    else {
        elEnable(starEls[0]);
    }
}

/**
 * Creates a button group for star rating
 * @param {number} rating the rating (0-10)
 * @returns {HTMLElement} div element
 */
function createStarRating(rating) {
    const div = elCreateEmpty('div', {"class": ["btn-group"]});
    const clearEl = elCreateText('button', {"class": ["btn", "btn-secondary", "mi", "px-1"], "data-title-phrase": "Clear", "title": "Clear", "data-vote": "0" }, 'clear');
    if (rating === 0) {
        clearEl.setAttribute('disabled', 'disabled');
    }
    div.appendChild(clearEl);
    for (let i = 1; i < 6; i++) {
        const vote = i * 2;
        const lig = rating >= vote
            ? ligatures.stared
            : ligatures.star;
        const padding = i === 5
                ? ['ps-0', 'pe-1']
                : ['px-0'];
        div.appendChild(
            elCreateText('button', {"class": ["btn", "btn-secondary", "mi", ... padding], "title": vote.toString(), "data-vote": vote.toString() }, lig)
        );
    }
    return div;
}

/**
 * Shows the stars rating
 * @param {number} rating the rating (0-10)
 * @returns {HTMLElement} div element
 */
function showStarRating(rating) {
    const div = elCreateEmpty('div', {});
    for (let i = 1; i < 6; i++) {
        const vote = i * 2;
        const lig = rating >= vote
            ? ligatures.stared
            : ligatures.star;
        div.appendChild(
            elCreateText('span', {"class": ["mi"], "title": vote.toString()}, lig)
        );
    }
    return div;
}
