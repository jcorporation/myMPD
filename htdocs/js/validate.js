"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

function validateFilenameString(str) {
    if (str === '') {
        return false;
    }
    else if (str.match(/^[\w-.]+$/) !== null) {
        return true;
    }
    else {
        return false;
    }
}

function validateFilename(el) {
    if (validateFilenameString(el.value) === false) {
        el.classList.add('is-invalid');
        return false;
    }
    else {
        el.classList.remove('is-invalid');
        return true;
    }
}

function validateFilenameList(el) {
    el.classList.remove('is-invalid');
    
    let filenames = el.value.split(',');
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
    else if (el.value.match(/^\/[/.\w-]+$/) !== null) {
        el.classList.remove('is-invalid');
        return true;
    }
    else {
        el.classList.add('is-invalid');
        return false;
    }
}

function validatePlname(x) {
    if (x === '') {
        return false;
    }
    else if (x.match(/\/|\r|\n|"|'/) === null) {
        return true;
    }
    else {
        return false;
    }
}

function validateNotBlank(el) {
    let value = el.value.replace(/\s/g, '');
    if (value === '') {
        el.classList.add('is-invalid');
        return false;
    }
    else {
        el.classList.remove('is-invalid');
        return true;
    }
}

function validateInt(el) {
    let value = el.value.replace(/\d/g, '');
    if (value !== '') {
        el.classList.add('is-invalid');
        return false;
    }
    else {
        el.classList.remove('is-invalid');
        return true;
    }
}

function validateFloat(el) {
    let value = el.value.replace(/[\d-.]/g, '');
    if (value !== '') {
        el.classList.add('is-invalid');
        return false;
    }
    else {
        el.classList.remove('is-invalid');
        return true;
    }
}

function validateStream(el) {
    if (el.value.indexOf('://') > -1) {
        el.classList.remove('is-invalid');
        return true;
    }
    else {
        el.classList.add('is-invalid');
        return false;
    }
}

function validateHost(el) {
    if (el.value.match(/^([\w-.]+)$/) !== null) {
        el.classList.remove('is-invalid');
        return true;
    }
    else {
        el.classList.add('is-invalid');
        return false;
    }
}
