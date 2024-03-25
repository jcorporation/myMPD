"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module grid_js */

/**
 * Central grid click handler.
 * @param {MouseEvent} event the event to handle
 * @returns {HTMLElement} the event target (card-body) to handle or null if it was handled or should not be handled
 */
function gridClickHandler(event) {
    if (event.target.classList.contains('row')) {
        return null;
    }
    //select mode
    if (selectEntry(event) === true) {
        return null;
    }
    const target = event.target.closest('DIV');
    if (target === null) {
        return null;
    }
    if (target.classList.contains('card-footer')){
        showContextMenu(event);
        return null;
    }
    return target;
}

/**
 * Adds the quick play button to a grid element
 * @param {ChildNode} parentEl the containing element
 * @returns {void}
 */
function addGridQuickPlayButton(parentEl) {
    const div = pEl.coverPlayBtn.cloneNode(true);
    parentEl.appendChild(div);
    div.addEventListener('click', function(event) {
        event.preventDefault();
        event.stopPropagation();
        clickQuickPlay(event.target);
    }, false);
}
