"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function isValidUri(uri) {
    if (uri === '' ||
        uri === undefined ||
        uri === null)
    {
        return false;
    }
    return true;
}

function isStreamUri(uri) {
    if (uri.indexOf('://') > -1) {
        return true;
    }
    return false;
}

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
 * @param {Element} parentEl 
 */
function removeIsInvalid(parentEl) {
    const els = parentEl.querySelectorAll('.is-invalid');
    for (let i = 0, j = els.length; i < j; i++) {
        els[i].classList.remove('is-invalid');
    }
}

/**
 * Marks an element as invalid
 * @param {string} id 
 */
function setIsInvalidId(id) {
    setIsInvalid(document.getElementById(id));
}

/**
 * Marks an element as invalid
 * @param {Element} el 
 */
function setIsInvalid(el) {
    //set is-invalid on parent node
    el.parentNode.classList.add('is-invalid');
    el.classList.add('is-invalid');
}

function validateFilenameString(str) {
    if (str === '') {
        return false;
    }
    if (str.match(/\/|\r|\n|"|'/) === null) {
        return true;
    }
    return false;
}

function validateFilename(el) {
    if (validateFilenameString(el.value) === false) {
        setIsInvalid(el);
        return false;
    }
    return true;
}

function validateFilenameList(el) {
    const filenames = el.value.split(',');
    for (let i = 0, j = filenames.length; i < j; i++) {
        if (validateFilenameString(filenames[i].trim()) === false) {
            setIsInvalid(el);
            return false;
        }
    }
    return true;
}

function validatePath(el) {
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

function validatePlnameEl(el) {
    if (validatePlname(el.value) === false) {
        setIsInvalid(el);
        return false;
    }
    return true;
}

function validatePlname(str) {
    if (str === '') {
        return false;
    }
    if (str.match(/\/|\r|\n|"|'/) === null) {
        return true;
    }
    return false;
}

function validateNotBlank(el) {
    const value = el.value.replace(/\s/g, '');
    if (value === '') {
        setIsInvalid(el);
        return false;
    }
    return true;
}

function validateInt(el) {
    const value = el.value.replace(/[\d-]/g, '');
    if (value !== '') {
        setIsInvalid(el);
        return false;
    }
    return true;
}

function validateUint(el) {
    const value = el.value.replace(/[\d]/g, '');
    if (value !== '') {
        setIsInvalid(el);
        return false;
    }
    return true;
}

function validateIntRange(el, min, max) {
    if (validateInt(el) === false) {
        return false;
    }
    const intValue = Number(el.value);
    if (intValue < min || intValue > max) {
        setIsInvalid(el);
        return false;
    }
    return true;
}

function validateFloat(el) {
    const value = el.value.replace(/[\d-.]/g, '');
    if (value !== '') {
        setIsInvalid(el);
        return false;
    }
    return true;
}

function validateFloatRange(el, min, max) {
    if (validateFloat(el) === false) {
        return false;
    }
    const floatValue = Number(el.value);
    if (floatValue < min || floatValue > max) {
        setIsInvalid(el);
        return false;
    }
    return true;
}

function validateStream(el) {
    if (isStreamUri(el.value) === true) {
        return true;
    }
    setIsInvalid(el);
    return false;
}

function validateHost(el) {
    if (el.value.match(/^([\w-.]+)$/) !== null) {
        return true;
    }
    setIsInvalid(el);
    return false;
}

function validateSelect(el) {
    if (getSelectValue(el) !== undefined) {
        return true;
    }
    setIsInvalid(el);
    return false;
}

function validatePrintable(el) {
    const value = el.value.replace(/[\w-]+/g, '');
    if (value !== '') {
        setIsInvalid(el);
        return false;
    }
    return true;
}
