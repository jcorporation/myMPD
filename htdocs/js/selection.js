"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module selection_js */

/**
 * Switches the select mode of current displayed list
 * @param {EventTarget} target triggering button
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function switchListMode(target) {
    const list = elGetById(app.id + 'List');
    const mode = list.getAttribute('data-mode');

    if (mode === null) {
        list.setAttribute('data-mode', 'select');
        target.classList.add('selected');
        target.classList.remove('rounded-end');
        target.nextElementSibling.classList.remove('d-none');
    }
    else {
        list.removeAttribute('data-mode');
        target.classList.remove('selected');
        target.classList.add('rounded-end');
        target.nextElementSibling.classList.add('d-none');
        selectAllEntries(list, false);
    }
}

/**
 * Select or unselect all entries
 * @param {HTMLElement} list table element
 * @param {boolean} select true = select all rows, false = clear selection
 * @returns {void}
 */
function selectAllEntries(list, select) {
    const viewMode = settings['view' + app.id].mode;
    const entries = viewMode === 'table'
        ? list.querySelectorAll('tbody > tr')
        : viewMode === 'grid'
            ? list.querySelectorAll('.col')
            : list.querySelectorAll('.list-group-item');
    let firstType = undefined;
    for (const entry of entries) {
        if (entry.lastElementChild.lastElementChild !== null) {
            firstType = getData(entry, 'type');
            break;
        }
    }
    for (const entry of entries) {
        const check = viewMode === 'table'
            ? entry.lastElementChild.lastElementChild
            : viewMode === 'grid'
                ? entry.querySelector('.card-footer > button')
                : entry.querySelector('.list-actions > button');
        if (check === null ||
            entry.classList.contains('not-clickable') ||
            (getData(entry, 'type') !== firstType && select === true))
        {
            // Skip entries with no select button and entries with different types
            continue;
        }
        if (select === true) {
            if (viewMode === 'table' ||
                viewMode === 'list')
            {
                entry.classList.add('selected');
            }
            else {
                entry.firstElementChild.classList.add('selected');
            }
            check.textContent = ligatures['checked'];
        }
        else {
            if (viewMode === 'table' ||
                viewMode === 'list')
            {
                entry.classList.remove('selected');
            }
            else {
                entry.firstElementChild.classList.remove('selected');
            }
            check.textContent = ligatures['unchecked'];
        }
    }
    showSelectionCount();
}

/**
 * Checks if list is in select mode and selects the entries
 * @param {Event} event triggering event
 * @returns {boolean} true if list is in select mode, else false
 */
function selectEntry(event) {
    const list = settings['view' + app.id].mode === 'table'
        ? event.target.closest('TABLE')
        : settings['view' + app.id].mode === 'grid'
            ? event.target.closest('.mympd-grid')
            : event.target.closest('.list-group');
    const mode = list.getAttribute('data-mode');
    if (event.ctrlKey &&
        mode === null)
    {
        //enable select mode
        switchListMode(elGetById(app.id + 'SelectModeBtn'));
    }
    else if (mode === null) {
        return false;
    }
    //in list select mode
    const entry = settings['view' + app.id].mode === 'table'
        ? event.target.closest('TR')
        : settings['view' + app.id].mode === 'grid'
            ? event.target.closest('.card')
            : event.target.closest('.list-group-item');
    if (entry.classList.contains('not-clickable') &&
        event.target.parentNode.nodeName !== 'TH') {
        return true;
    }
    if (event.target.parentNode.nodeName === 'TH') {
        const select = event.target.textContent === ligatures['unchecked']
            ? true
            : false;
        event.target.textContent = select === true
            ? ligatures['checked']
            : ligatures['unchecked'];
        selectAllEntries(list, select);
    }
    else if (event.shiftKey) {
        let lastPos = getData(list, 'last-selected');
        if (lastPos === undefined) {
            lastPos = 0;
        }
        const pos = settings['view' + app.id].mode === 'table'
            ? elGetIndex(entry)
            : elGetIndex(entry.parentNode);
        setData(list, 'last-selected', pos);
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
        const entries = settings['view' + app.id].mode === 'table'
            ? list.querySelector('tbody').querySelectorAll('tr')
            : list.querySelectorAll('.col');
        const firstType = getData(entries[first], 'type');
        for (let i = first; i <= last; i++) {
            if (getData(entries[i], 'type') !== firstType) {
                continue;
            }
            if (settings['view' + app.id].mode === 'table') {
                selectSingleEntry(entries[i], true);
            }
            else {
                selectSingleEntry(entries[i].firstElementChild, true);
            }
        }
    }
    else {
        selectSingleEntry(entry, null);
        if (settings['view' + app.id].mode === 'table') {
            setData(list, 'last-selected', elGetIndex(entry));
        }
        else {
            setData(list, 'last-selected', elGetIndex(entry.parentNode));
        }
    }
    showSelectionCount();
    event.preventDefault();
    event.stopPropagation();
    return true;
}

/**
 * Selects / unselects a single row
 * @param {HTMLElement} entry entry to select or unselect
 * @param {boolean} [select] true = select, false = unselect, null = toggle
 * @returns {void}
 */
function selectSingleEntry(entry, select) {
    const check = settings['view' + app.id].mode === 'table'
        ? entry.lastElementChild.lastElementChild
        : settings['view' + app.id].mode === 'grid'
            ? entry.querySelector('.card-footer > button')
            : entry.querySelector('.list-actions > button');
    if (check === null) {
        return;
    }
    if ((select === null && entry.classList.contains('selected')) ||
        select === false)
    {
        check.textContent = ligatures['unchecked'];
        entry.classList.remove('selected');
    }
    else {
        check.textContent = ligatures['checked'];
        entry.classList.add('selected');
    }
}

/**
 * Shows the number of selections in the dropdown
 * @returns {void}
 */
function showSelectionCount() {
    const list = elGetById(app.id + 'List');
    const dropdown = document.querySelector('#' + app.id + 'SelectionDropdown');
    const entries = settings['view' + app.id].mode === 'table'
        ? list.querySelectorAll('tbody > tr.selected')
        : list.querySelectorAll('div > div.selected');
    const count = entries.length;
    let validSelection = true;
    if (count > 1) {
        const firstType = getData(entries[0], 'type');
        for (const entry of entries) {
            if (getData(entry, 'type') !== firstType) {
                validSelection = false;
                break;
            }
        }
    }
    if (validSelection === true) {
        dropdown.querySelector('small').textContent = count + ' ' + tn('selected');
    }
    else {
        dropdown.querySelector('small').textContent = tn('Invalid selection');
    }
    const btns = dropdown.querySelectorAll('button');
    for (const btn of btns) {
        if (count === 0 ||
            validSelection === false)
        {
            btn.setAttribute('disabled', 'disabled');
        }
        else {
            btn.removeAttribute('disabled');
        }
    }
}
