"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module result_js */

/**
 * Creates an empty list hint
 * @param {number} colspan column count
 * @param {string} mode table or grid
 * @returns {HTMLElement} created row
 */
function emptyMsgEl(colspan, mode) {
    if (mode === 'grid') {
        return elCreateTextTn('div', {"class": ["col", "not-clickable", "alert", "alert-secondary"]}, 'Empty list');
    }
    return elCreateNode('tr', {"class": ["not-clickable"]},
        elCreateNode('td', {"colspan": colspan},
            elCreateTextTn('div', {"class": ["alert", "alert-secondary"]}, 'Empty list')
        )
    );
}

/**
 * Creates a loading list hint
 * @param {number} colspan column count
 * @param {string} mode table or grid
 * @returns {HTMLElement} created row
 */
//eslint-disable-next-line no-unused-vars
function loadingMsgEl(colspan, mode) {
    if (mode === 'grid') {
        return elCreateTextTn('div', {"class": ["col", "not-clickable", "alert", "alert-secondary"]}, 'Loading...');
    }
    return elCreateNode('tr', {"class": ["not-clickable"]},
        elCreateNode('td', {"colspan": colspan},
            elCreateTextTn('div', {"class": ["alert", "alert-secondary"]}, 'Loading...')
        )
    );
}

/**
 * Creates an element with the error message
 * @param {object} obj jsonrpc error object
 * @param {number} colspan column count
 * @param {string} mode table or grid
 * @returns {HTMLElement} created row
 */
function errorMsgEl(obj, colspan, mode) {
    if (mode === 'grid') {
        return elCreateTextTn('div', {"class": ["col", "not-clickable", "alert", "alert-danger"]}, obj.error.message, obj.error.data);
    }
    return elCreateNode('tr', {"class": ["not-clickable"]},
        elCreateNode('td', {"colspan": colspan},
            elCreateTextTn('div', {"class": ["alert", "alert-danger"]}, obj.error.message, obj.error.data)
        )
    );
}

/**
 * Creates an element with the warning message
 * @param {string} message phrase to display
 * @param {number} colspan column count
 * @param {string} mode table or grid
 * @returns {HTMLElement} created row
 */
//eslint-disable-next-line no-unused-vars
function warningMsgEl(message, colspan, mode) {
    if (mode === 'grid') {
        return elCreateTextTn('div', {"class": ["col", "not-clickable", "alert", "alert-warning"]}, message);
    }
    return elCreateNode('tr', {"class": ["not-clickable"]},
        elCreateNode('td', {"colspan": colspan},
            elCreateTextTn('div', {"class": ["alert", "alert-warning"]}, message)
        )
    );
}

/**
 * Wrapper for checkResult with id selector
 * @param {object} obj jsonrpc object to check
 * @param {string} parentid parent id of element to add the result message
 * @param {string} mode table or grid
 * @returns {boolean} true = result is not an error, else false
 */
function checkResultId(obj, parentid, mode) {
    if (mode === undefined) {
        mode = settings['view' + app.id].mode;
    }
    return checkResult(obj, document.querySelector('#' + parentid), mode);
}

/**
 * Checks the json response for an error object or empty result
 * and displays the error in the table body.
 * @param {object} obj jsonrpc object to check
 * @param {HTMLElement} parent element to add the result message
 * @param {string} mode One off table, grid, list or modalTable
 * @returns {boolean} false = result is  empty or an error, else true
 */
function checkResult(obj, parent, mode) {
    if (mode === undefined) {
        mode = settings['view' + app.id].mode;
    }
    //remove old alert
    const alert = parent.querySelector('.alert');
    let colspan = 0;
    if (alert) {
        if (mode === 'table') {
            alert.parentNode.parentNode.remove();
        }
        else {
            alert.remove();
        }
    }
    if (obj.error ||
        obj.result.returnedEntities === 0)
    {
        if (mode === 'table' ||
            mode === 'modalTable')
        {
            const thead = parent.querySelector('tr');
            colspan = thead !== null
                ? thead.querySelectorAll('th').length
                : 0;
            const tfoot = parent.querySelector('tfoot');
            if (tfoot !== null) {
                elClear(tfoot);
            }
            parent = parent.querySelector('tbody');
        }

        if (obj.error) {
            elClear(parent);
            parent.appendChild(errorMsgEl(obj, colspan, mode));
        }
        else if (features.featPagination === true ||
                 mode === 'modalTable' ||
                 obj.result.offset === 0)
        {
            elClear(parent);
            parent.appendChild(emptyMsgEl(colspan, mode));
        }
        if (mode === 'table' ||
            mode === 'modalTable')
        {
            unsetUpdateView(parent.parentNode);
        }
        else {
            unsetUpdateView(parent);
        }
        setPagination(0, 0);
        return false;
    }
    return true;
}
