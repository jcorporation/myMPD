"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module validate_js */

/**
 * Highlight the input element with the invalid value
 * @param {string} prefix prefix for the input fields
 * @param {object} obj the jsonrpc error object
 * @returns {boolean} true if input element was found, else false
 */
function highlightInvalidInput(prefix, obj) {
    if (obj.error.data.path === undefined) {
        return false;
    }
    // try to find the input element
    const id = ucFirst(obj.error.data.path.split('.').pop());
    const el = document.querySelector('#' + prefix + id + 'Input');
    if (el) {
        let col = el.closest('.col-sm-8');
        if (col === null) {
            col = el.parentNode;
        }
        setIsInvalid(el);
        // append error message if there is no client-side invalid feedback defined
        if (col.querySelector('.invalid-feedback') === null) {
            const invalidServerEl = col.querySelector('.invalid-server');
            if (invalidServerEl === null) {
                // Create new invalid feedback element
                col.appendChild(
                    elCreateTextTn('div', {"class": ["invalid-feedback", "invalid-server"]}, tn(obj.error.message, obj.error.data))
                );
                return true;
            }
        }
        const invalidServerEl = col.querySelector('.invalid-server');
        if (invalidServerEl !== null) {
            // Update invalid feedback from server
            invalidServerEl.textContent = tn(obj.error.message, obj.error.data);
        }
        return true;
    }
    logError('Element not found: #' + prefix + id + 'Input');
    return false;
}

/**
 * Removes all is-invalid classes
 * @param {Element} el element
 * @returns {void}
 */
function removeIsInvalid(el) {
    const els = el.querySelectorAll('.is-invalid');
    for (let i = 0, j = els.length; i < j; i++) {
        els[i].classList.remove('is-invalid');
    }
}

/**
 * Marks an element as invalid
 * @param {string} id element id
 * @returns {void}
 */
function setIsInvalidId(id) {
    setIsInvalid(elGetById(id));
}

/**
 * Marks an element as invalid
 * @param {Element} el element
 * @returns {void}
 */
function setIsInvalid(el) {
    //set is-invalid also on parent node
    el.classList.add('is-invalid');
    const col = el.closest('.col-sm-8');
    col.classList.add('is-invalid');
}

/**
 * Checks if string is a valid uri (not empty)
 * @param {string} uri uri to check 
 * @returns {boolean} true = valid uri, else false
 */
function isValidUri(uri) {
    if (uri === '' ||
        uri === undefined ||
        uri === null ||
        uri.match(/^\s*$/) !== null)
    {
        return false;
    }
    return true;
}

/**
 * Checks if string is a stream uri
 * @param {string} uri uri to check
 * @returns {boolean} true = stream uri, else false
 */
function isStreamUri(uri) {
    if (uri === undefined) {
        return false;
    }
    if (uri.indexOf('://') > -1) {
        return true;
    }
    return false;
}

/**
 * Checks if string is a http uri
 * @param {string} uri uri to check
 * @returns {boolean} true = http uri, else false
 */
function isHttpUri(uri) {
    if (uri.indexOf('http://') === 0 ||
        uri.indexOf('https://') === 0)
    {
        return true;
    }
    return false;
}

/**
 * Checks if the value of the input element is a valid playlist name
 * @param {Element} el input element
 * @returns {boolean} true = valid playlist name, else false
 */
//eslint-disable-next-line no-unused-vars
function validatePlistEl(el) {
    if (validatePlist(el.value) === false) {
        setIsInvalid(el);
        return false;
    }
    return true;
}

/**
 * Checks if the string is a valid playlist name
 * @param {string} str input element
 * @returns {boolean} true = valid playlist name, else false
 */
function validatePlist(str) {
    if (str === '') {
        return false;
    }
    if (str.match(/\/|\r|\n|"|'/) === null) {
        return true;
    }
    return false;
}

/**
 * Checks if the the value of the input element is an unsigned integer
 * @param {Element} el input element
 * @returns {boolean} true = unsigned integer, else false
 */
//eslint-disable-next-line no-unused-vars
function validateUintEl(el) {
    const value = el.value.replace(/[\d]/g, '');
    if (value !== '') {
        setIsInvalid(el);
        return false;
    }
    return true;
}

/**
 * Checks if the the value of the input element is a float
 * @param {Element} el input element
 * @returns {boolean} true = float, else false
 */
function validateFloatEl(el) {
    const value = el.value.replace(/[\d-.]/g, '');
    if (value !== '') {
        setIsInvalid(el);
        return false;
    }
    return true;
}

/**
 * Checks if the the value of the input element is an float in range
 * @param {Element} el input element
 * @param {number} min minimum value (including)
 * @param {number} max maximum value (including)
 * @returns {boolean} true = integer in range, else false
 */
//eslint-disable-next-line no-unused-vars
function validateFloatRangeEl(el, min, max) {
    if (validateFloatEl(el) === false) {
        return false;
    }
    const floatValue = Number(el.value);
    if (floatValue < min || floatValue > max) {
        setIsInvalid(el);
        return false;
    }
    return true;
}

/**
 * Checks if the the value of the input element is a valid stream uri
 * @param {Element} el input element
 * @returns {boolean} true = valid stream uri, else false
 */
function validateStreamEl(el) {
    if (el.value.length === 0) {
        return true;
    }
    if (isStreamUri(el.value) === true) {
        return true;
    }
    setIsInvalid(el);
    return false;
}

/**
 * Checks if the value of the input element contains only printable characters
 * @param {Element} el input element
 * @returns {boolean} true = only printable characters, else false
 */
function validatePrintableEl(el) {
    const value = el.value.replace(/[\w-]+/g, '');
    if (value !== '') {
        setIsInvalid(el);
        return false;
    }
    return true;
}

/**
 * Checks if the the value of the input element is a hex color
 * @param {Element} el input element
 * @returns {boolean} true = valid stream uri, else false
 */
//eslint-disable-next-line no-unused-vars
function validateColorEl(el) {
    if (el.value.match(/^#[a-f\d]+$/i) !== null) {
        return true;
    }
    setIsInvalid(el);
    return false;
}
