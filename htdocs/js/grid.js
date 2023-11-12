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
    const grid = elGetById(app.id + 'List');
    const mode = grid.getAttribute('data-mode');

    if (mode === null) {
        grid.setAttribute('data-mode', 'select');
        target.classList.add('selected');
        target.classList.remove('rounded-end');
        target.nextElementSibling.classList.remove('d-none');
    }
    else {
        grid.removeAttribute('data-mode');
        target.classList.remove('selected');
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
            card.classList.add('selected');
            check.textContent = ligatures['checked'];
        }
        else {
            card.classList.remove('selected');
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
        switchGridMode(elGetById(app.id + 'SelectModeBtn'));
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
    if ((select === null && card.classList.contains('selected')) ||
        select === false)
    {
        check.textContent = ligatures['unchecked'];
        card.classList.remove('selected');
    }
    else {
        check.textContent = ligatures['checked'];
        card.classList.add('selected');
    }
}

/**
 * Shows the number of selections in the dropdown
 * @returns {void}
 */
function showGridSelectionCount() {
    const grid = elGetById(app.id + 'List');
    const dropdown = document.querySelector('#' + app.id + 'SelectionDropdown');
    const cards = grid.querySelectorAll('div > div.selected');
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

/**
 * Central grid click handler.
 * Handles clicks on table header and body.
 * @param {MouseEvent} event the event to handle
 * @returns {HTMLElement} the event target (card-body) to handle or null if it was handled or should not be handled
 */
function gridClickHandler(event) {
    if (event.target.classList.contains('row')) {
        return null;
    }
    //select mode
    if (selectCard(event) === true) {
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
