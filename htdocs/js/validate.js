"use strict";
// SPDX-License-Identifier: GPL-2.0-or-later
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

function removeIsInvalid(parentEl) {
    const els = parentEl.getElementsByClassName('is-invalid');
    for (let i = 0; i < els.length; i++) {
        els[i].classList.remove('is-invalid');
    }
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
        el.classList.add('is-invalid');
        return false;
    }
    el.classList.remove('is-invalid');
    return true;
}

function validateFilenameList(el) {
    el.classList.remove('is-invalid');
    
    const filenames = el.value.split(',');
    for (let i = 0; i < filenames.length; i++) {
        if (validateFilenameString(filenames[i].trim()) === false) {
            el.classList.add('is-invalid');
            return false;
        }
    }
    return true;
}

function validatePath(el) {
    if (el.value === '') {
        el.classList.add('is-invalid');
        return false;
    }
    if (el.value.match(/^\/[/.\w-]+$/) !== null) {
        el.classList.remove('is-invalid');
        return true;
    }
    el.classList.add('is-invalid');
    return false;
}

function validatePlnameEl(el) {
    if (validatePlname(el.value) === false) {
        el.classList.add('is-invalid');
        return false;
    }
    el.classList.remove('is-invalid');
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
        el.classList.add('is-invalid');
        return false;
    }
    el.classList.remove('is-invalid');
    return true;
}

function validateInt(el) {
    const value = el.value.replace(/[\d-]/g, '');
    if (value !== '') {
        el.classList.add('is-invalid');
        return false;
    }
    el.classList.remove('is-invalid');
    return true;
}

function validateFloat(el) {
    const value = el.value.replace(/[\d-.]/g, '');
    if (value !== '') {
        el.classList.add('is-invalid');
        return false;
    }
    el.classList.remove('is-invalid');
    return true;
}

function validateStream(el) {
    if (isStreamUri(el.value) === true) {
        el.classList.remove('is-invalid');
        return true;
    }
    el.classList.add('is-invalid');
    return false;
}

function validateHost(el) {
    if (el.value.match(/^([\w-.]+)$/) !== null) {
        el.classList.remove('is-invalid');
        return true;
    }
    el.classList.add('is-invalid');
    return false;
}

function validateSelect(el) {
    if (getSelectValue(el) !== undefined) {
        el.classList.remove('is-invalid');
        return true;
    }
    el.classList.add('is-invalid');
    return false;
}
