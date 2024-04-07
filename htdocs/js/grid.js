"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module grid_js */

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
