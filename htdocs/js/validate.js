"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function isValidUri(uri) {
    if (uri === '' || uri === undefined || uri === null) {
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

function removeIsInvalidId(parentElId) {
    removeIsInvalid(document.getElementById(parentElId));
}

function removeIsInvalid(parentEl) {
    const els = parentEl.getElementsByClassName('is-invalid');
    for (let i = els.length - 1; i >= 0; i--) {
        els[i].classList.remove('is-invalid');
    }
}

function setIsInvalidId(id) {
    setIsInvalid(document.getElementById(id));
}

function setIsInvalid(el) {
    el.classList.add('is-invalid');
}

function validateFilenameString(str) {
    if (str === '') {
        return false;
    }
    if (str.match(/^[\w-.]+$/) !== null) {
        return true;
    }
    return false;
}

function validateFilename(el) {
    if (validateFilenameString(el.value) === false) {
        setIsInvalid(el);
        return false;
    }
    removeIsInvalid(el);
    return true;
}

function validateFilenameList(el) {
    removeIsInvalid(el);
    
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
    if (el.value === '') {
        setIsInvalid(el);
        return false;
    }
    if (el.value.match(/^\/[/.\w-]+$/) !== null) {
        removeIsInvalid(el);
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
    removeIsInvalid(el);
    return true;
}

function validatePlname(x) {
    if (x === '') {
        return false;
    }
    if (x.match(/\/|\r|\n|"|'/) === null) {
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
    removeIsInvalid(el);
    return true;
}

function validateInt(el) {
    const value = el.value.replace(/[\d-]/g, '');
    if (value !== '') {
        setIsInvalid(el);
        return false;
    }
    removeIsInvalid(el);
    return true;
}

function validateUint(el) {
    const value = el.value.replace(/[\d]/g, '');
    if (value !== '') {
        setIsInvalid(el);
        return false;
    }
    removeIsInvalid(el);
    return true;
}

function validateIntRange(el, min, max) {
    const value = el.value.replace(/[\d]/g, '');
    if (value !== '') {
        setIsInvalid(el);
        return false;
    }
    const intValue = Number(el.value);
    if (intValue < min || intValue > max) {
        setIsInvalid(el);
        return false;
    }
    removeIsInvalid(el);
    return true;
}

function validateFloat(el) {
    const value = el.value.replace(/[\d-.]/g, '');
    if (value !== '') {
        setIsInvalid(el);
        return false;
    }
    removeIsInvalid(el);
    return true;
}

function validateStream(el) {
    if (isStreamUri(el.value) === true) {
        removeIsInvalid(el);
        return true;
    }
    setIsInvalid(el);
    return false;
}

function validateHost(el) {
    if (el.value.match(/^([\w-.]+)$/) !== null) {
        removeIsInvalid(el);
        return true;
    }
    setIsInvalid(el);
    return false;
}

function validateSelect(el) {
    if (getSelectValue(el) !== undefined) {
        removeIsInvalid(el);
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
    removeIsInvalid(el);
    return true;
}
