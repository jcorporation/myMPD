"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module validate_js */

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
 * Removes all is-invalid classes
 * @param {Element} parentEl root element
 */
function removeIsInvalid(parentEl) {
    const els = parentEl.querySelectorAll('.is-invalid');
    for (let i = 0, j = els.length; i < j; i++) {
        els[i].classList.remove('is-invalid');
    }
}

/**
 * Marks an element as invalid
 * @param {string} id element id
 */
function setIsInvalidId(id) {
    setIsInvalid(document.getElementById(id));
}

/**
 * Marks an element as invalid
 * @param {Element} el element
 */
function setIsInvalid(el) {
    //set is-invalid on parent node
    el.parentNode.classList.add('is-invalid');
    el.classList.add('is-invalid');
}

/**
 * Checks if string is a valid filename
 * @param {string} str string to check
 * @returns {boolean} true = valid filename, else false
 */
function validateFilenameString(str) {
    if (str === '') {
        return false;
    }
    if (str.match(/\/|\r|\n|"|'/) === null) {
        return true;
    }
    return false;
}

/**
 * Checks if the value of the input element is a valid filename
 * @param {Element} el input element
 * @returns {boolean} true = valid filename, else false
 */
function validateFilenameEl(el) {
    if (validateFilenameString(el.value) === false) {
        setIsInvalid(el);
        return false;
    }
    return true;
}

/**
 * Checks if the value of the input element is a valid filename list
 * @param {Element} el input element
 * @returns {boolean} true = valid filename, else false
 */
function validateFilenameListEl(el) {
    const filenames = el.value.split(',');
    for (let i = 0, j = filenames.length; i < j; i++) {
        if (validateFilenameString(filenames[i].trim()) === false) {
            setIsInvalid(el);
            return false;
        }
    }
    return true;
}

/**
 * Checks if the value of the input element is a valid filepath
 * @param {Element} el input element
 * @returns {boolean} true = valid filepath, else false
 */
function validatePathEl(el) {
    if (el.value === '' ||
        el.value.charAt(0) !== '/')
    {
        setIsInvalid(el);
        return false;
    }
    if (el.value.match(/\r|\n|"|'/) === null) {
        return true;
    }
    setIsInvalid(el);
    return false;
}

/**
 * Checks if the value of the input element is a valid playlist name
 * @param {Element} el input element
 * @returns {boolean} true = valid playlist name, else false
 */
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
 * Checks if the the value of the input element is not blank
 * @param {Element} el input element
 * @returns {boolean} true = not empty, else false
 */
function validateNotBlankEl(el) {
    const value = el.value.replace(/\s/g, '');
    if (value === '') {
        setIsInvalid(el);
        return false;
    }
    return true;
}

/**
 * Checks if the the value of the input element is an integer
 * @param {Element} el input element
 * @returns {boolean} true = integer, else false
 */
function validateIntEl(el) {
    const value = el.value.replace(/[\d-]/g, '');
    if (value !== '') {
        setIsInvalid(el);
        return false;
    }
    return true;
}

/**
 * Checks if the the value of the input element is an unsigned integer
 * @param {Element} el input element
 * @returns {boolean} true = unsigned integer, else false
 */
function validateUintEl(el) {
    const value = el.value.replace(/[\d]/g, '');
    if (value !== '') {
        setIsInvalid(el);
        return false;
    }
    return true;
}

/**
 * Checks if the the value of the input element is an integer in range
 * @param {Element} el input element
 * @param {number} min minimum value (including)
 * @param {number} max maximum value (including)
 * @returns {boolean} true = integer in range, else false
 */
function validateIntRangeEl(el, min, max) {
    if (validateIntEl(el) === false) {
        return false;
    }
    const intValue = Number(el.value);
    if (intValue < min || intValue > max) {
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
    if (isStreamUri(el.value) === true) {
        return true;
    }
    setIsInvalid(el);
    return false;
}

/**
 * Checks if the the value of the input element is a valid host
 * @param {Element} el input element
 * @returns {boolean} true = valid host, else false
 */
function validateHostEl(el) {
    if (el.value.match(/^([\w-.]+)$/) !== null) {
        return true;
    }
    setIsInvalid(el);
    return false;
}

/**
 * Checks if the select element has an option selected
 * @param {Element} el select element
 * @returns {boolean} true = valid, else false
 */
function validateSelectEl(el) {
    if (getSelectValue(el) !== undefined) {
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
