"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module grid_js */

/**
 * Switches the select mode of current displayed grid
 * @param {EventTarget} target triggering button
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function switchGridMode(target) {
    const grid = document.getElementById(app.id + 'List');
    const mode = grid.getAttribute('data-mode');

    if (mode === null) {
        grid.setAttribute('data-mode', 'select');
        target.classList.add('active');
        target.classList.remove('rounded-end');
        target.nextElementSibling.classList.remove('d-none');
    }
    else {
        grid.removeAttribute('data-mode');
        target.classList.remove('active');
        target.classList.add('rounded-end');
        target.nextElementSibling.classList.add('d-none');
        selectAllCards(grid, false);
    }
}

/**
 * Selects all cards in the grid
 * @param {HTMLElement} grid grid element
 * @param {boolean} select true = select all rows, false = clear selection
 * @returns {void}
 */
function selectAllCards(grid, select) {
    const cardCols = grid.querySelectorAll('.col');
    for (const col of cardCols) {
        const card = col.firstElementChild;
        if (card === null) {
            continue;
        }
        const check = card.querySelector('.card-footer > button');
        if (check === null) {
            continue;
        }
        if (select === true) {
            card.classList.add('active');
            check.textContent = ligatures['checked'];
        }
        else {
            card.classList.remove('active');
            check.textContent = ligatures['unchecked'];
        }
    }
    showGridSelectionCount();
}

/**
 * Checks if grid is in select mode and selects the card
 * @param {Event} event triggering event
 * @returns {boolean} true if table in select mode, else false
 */
function selectCard(event) {
    const grid = event.target.closest('.mympd-grid');
    const mode = grid.getAttribute('data-mode');
    if (event.ctrlKey &&
        mode === null)
    {
        //enable select mode
        switchGridMode(document.getElementById('btn' + app.id + 'SelectMode'));
    }
    else if (mode === null) {
        return false;
    }
    //in grid select mode
    const card = event.target.closest('.card');
    if (card.classList.contains('not-clickable')) {
        return true;
    }
    else if (event.shiftKey) {
        let lastPos = getData(grid, 'last-selected');
        if (lastPos === undefined) {
            lastPos = 0;
        }
        const pos = elGetIndex(card.parentNode);
        setData(grid, 'last-selected', pos);
        let first;
        let last;
        if (lastPos < pos) {
            first = lastPos;
            last = pos;
        }
        else {
            first = pos;
            last = lastPos;
        }
        const cardCols = grid.querySelectorAll('.col');
        for (let i = first; i <= last; i++) {
            selectSingleCard(cardCols[i].firstElementChild, true);
        }
    }
    else {
        selectSingleCard(card, null);
        setData(grid, 'last-selected', elGetIndex(card.parentNode));
    }
    showGridSelectionCount();
    event.preventDefault();
    event.stopPropagation();
    return true;
}

/**
 * Selects / unselects a single card
 * @param {HTMLElement} card card to select or unselect
 * @param {boolean} [select] true = select, false = unselect, null = toggle
 * @returns {void}
 */
function selectSingleCard(card, select) {
    const check = card.querySelector('.card-footer > button');
    if (check === null) {
        return;
    }
    if ((select === null && card.classList.contains('active')) ||
        select === false)
    {
        check.textContent = ligatures['unchecked'];
        card.classList.remove('active');
    }
    else {
        check.textContent = ligatures['checked'];
        card.classList.add('active');
    }
}

/**
 * Shows the number of selections in the dropdown
 * @returns {void}
 */
function showGridSelectionCount() {
    const grid = document.getElementById(app.id + 'List');
    const dropdown = document.querySelector('#dropdown' + app.id + 'Selection');
    const cards = grid.querySelectorAll('div > div.active');
    const count = cards.length;
    dropdown.querySelector('small').textContent = count + ' ' + tn('selected');
    const btns = dropdown.querySelectorAll('button');
    for (const btn of btns) {
        if (count === 0) {
            btn.setAttribute('disabled', 'disabled');
        }
        else {
            btn.removeAttribute('disabled');
        }
    }
}
