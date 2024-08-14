"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module feedback_js */

/**
 * Handler for click on feedback (like or rating)
 * @param {EventTarget} target The target
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function clickFeedback(target) {
    const parent = target.closest('.btn-group');
    const feedbackType = parent.getAttribute('data-feedback');
    if (feedbackType === 'like') {
        voteSongLike(target);
    }
    else {
        voteSongRating(target);
    }
}

/**
 * Sets the feedback mode buttons
 * @param {string} id Element id
 * @param {string} stickerType MPD sticker type
 * @returns {void}
 */
function setFeedbacktypeId(id, stickerType) {
    const el = document.getElementById(id);
    setFeedbacktype(el, stickerType);
}

/**
 * Sets the feedback mode buttons
 * @param {Element} el Element
 * @param {string} stickerType MPD sticker type
 * @returns {void}
 */
function setFeedbacktype(el, stickerType) {
    if (features.featLike === true &&
        el.getAttribute('data-feedback') !== 'like')
    {
        elClear(el);
        el.appendChild(createLike(1, stickerType));
    }
    else if (features.featRating === true  &&
             el.getAttribute('data-feedback') !== 'rating')
    {
        elClear(el);
        el.appendChild(createStarRating(0, stickerType));
    }
    else {
        elClear(el);
    }
}

/**
 * Sets the current feedback
 * @param {Element} el Feedback element group
 * @param {number} like Like feedback value
 * @param {number} rating Star rating feedback value
 * @returns {void}
 */
function setFeedback(el, like, rating) {
    if (el.getAttribute('data-feedback') === 'like') {
        setLike(el, like);
    }
    else {
        setRating(el, rating);
    }
}

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
    const uri = getData(el.parentNode, 'uri');
    const stickerType = el.parentNode.getAttribute('data-type');
    sendAPI("MYMPD_API_LIKE", {
        "uri": uri,
        "like": vote,
        "type": stickerType
    }, null, false);
}

/**
 * Creates the songs hate/love elements
 * @param {number} like like value 0 - 2
 * @param {string} stickerType MPD sticker type
 * @returns {HTMLElement} div element
 */
function createLike(like, stickerType) {
    const thDown = elCreateText('button', {"data-vote": "0", "data-title-phrase": "Hate song", "class": ["btn", "btn-secondary", "mi"]}, 'thumb_down');
    if (like === 0) {
        thDown.classList.add('active');
    }
    const thUp = elCreateText('button', {"data-vote": "2", "data-title-phrase": "Love song", "class": ["btn", "btn-secondary", "mi"]}, 'thumb_up');
    if (like === 2) {
        thUp.classList.add('active');
    }
    return elCreateNodes('div', {"class": ["btn-group"], "data-feedback": "like", "data-type": stickerType}, [
        thDown,
        thUp
    ]);
}

/**
 * Sets the song vote button group
 * @param {HTMLElement | Element} el container for the thumbs
 * @param {number} vote The vote
 * @returns {void}
 */
function setLike(el, vote) {
    const btnHate = el.firstElementChild;
    const btnLove = el.lastElementChild;
    switch(vote) {
        case 0:
            btnLove.classList.remove('active');
            btnHate.classList.add('active');
            break;
        case 2:
            btnLove.classList.add('active');
            btnHate.classList.remove('active');
            break;
        default:
            btnLove.classList.remove('active');
            btnHate.classList.remove('active');
            break;
    }
}

/**
 * Song rating event handler
 * @param {EventTarget} el triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function voteSongRating(el) {
    const rating = Number(el.getAttribute('data-vote'));
    const stickerType = el.parentNode.getAttribute('data-type');
    const uri = getData(el.parentNode, 'uri');
    setRating(el.parentNode, rating);

    sendAPI("MYMPD_API_RATING", {
        "uri": uri,
        "rating": rating,
        "type": stickerType
    }, null, false);
}

/**
 * Sets the star rating element
 * @param {HTMLElement | Element} el container for the stars
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
 * @param {string} stickerType MPD sticker type
 * @returns {HTMLElement} div element
 */
function createStarRating(rating, stickerType) {
    const div = elCreateEmpty('div', {"class": ["btn-group"], "data-feedback": "rating", "data-type": stickerType});
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
