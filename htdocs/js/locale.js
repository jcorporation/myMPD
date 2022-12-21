"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module locale_js */

/**
 * Checks for singular or plural
 * @param {number} number number to check
 * @returns {number} 0 = singular, 1 = plural
 */
function checkSmartCount(number) {
    if (number === 1) { return 0; }
    return 1;
}

/**
 * Translates the phrase and resolves variables
 * Variables are in the format %{name}
 * Phrase can include singular and plural separated by ||||
 * Singular or plural is detected by the special data key smartCount
 * @param {string} phrase the prase to translate
 * @param {object} [data] variable data
 * @returns {string} translated phrase
 */
function tn(phrase, data) {
    // @ts-ignore
    if (isNaN(phrase) === false) {
        //do not translate numbers
        return phrase;
    }
    if (phrase === undefined) {
        logError('Phrase is undefined');
        return 'undefinedPhrase';
    }
    //translate
    let result = phrases[phrase];
/*debug*/    if (result === undefined &&
/*debug*/        locale !== 'en-US')
/*debug*/    {
/*debug*/        logDebug('Phrase "' + phrase + '" for locale ' + locale + ' not found');
/*debug*/    }

    //fallback if phrase is not translated
    if (result === undefined) {
        result = phrasesDefault[phrase] !== undefined ? phrasesDefault[phrase] : phrase;
    }
    //check for smartCount
    if (data !== undefined &&
        data.smartCount !== undefined)
    {
        const p = result.split(' |||| ');
        if (p.length > 1) {
            result = p[checkSmartCount(data.smartCount)];
        }
        result = result.replace('%{smart_count}', data.smartCount);
    }
    //replace variables
    if (data !== undefined) {
        //eslint-disable-next-line no-useless-escape
        const tnRegex = /%\{(\w+)\}/g;
        result = result.replace(tnRegex, function(m0, m1) {
            return data[m1];
        });
    }
    return result;
}

/**
 * Returns timestamp as formatted date string
 * @param {number} secs unix timestamp
 * @returns {string} formatted date
 */
function fmtDate(secs) {
    return new Date(secs * 1000).toLocaleString(locale);
}

/**
 * Returns timestamp as formatted time string
 * @param {number} secs unix timestamp
 * @returns {string} formatted date
 */
function fmtTime(secs) {
    return new Date(secs * 1000).toTimeString(locale).substring(0, 8);
}

/**
 * Returns seconds as formatted duration string
 * @param {number} secs duration to format
 * @returns {string} formatted duration
 */
function fmtDuration(secs) {
    const days = Math.floor(secs / 86400);
    const hours = Math.floor(secs / 3600) - days * 24;
    const minutes = Math.floor(secs / 60) - hours * 60 - days * 1440;
    const seconds = secs - days * 86400 - hours * 3600 - minutes * 60;

    return (days > 0 ? days + smallSpace + tn('Days') + ' ' : '') +
        (hours > 0 ? hours + smallSpace + tn('Hours') + ' ' +
        (minutes < 10 ? '0' : '') : '') + minutes + smallSpace + tn('Minutes') + ' ' +
        (seconds < 10 ? '0' : '') + seconds + smallSpace + tn('Seconds');
}

/**
 * Returns seconds as formatted song duration string
 * @param {number} secs duration to format
 * @returns {string} formatted song duration
 */
function fmtSongDuration(secs) {
    const hours = Math.floor(secs / 3600);
    const minutes = Math.floor(secs / 60) - hours * 60;
    const seconds = Math.floor(secs - hours * 3600 - minutes * 60);

    return (hours > 0 ? hours + ':' + (minutes < 10 ? '0' : '') : '') +
        minutes + ':' + (seconds < 10 ? '0' : '') + seconds;
}

/**
 * Sets and fetches the locale and translates the dom
 * @param {string} newLocale locale to set
 */
function setLocale(newLocale) {
    if (newLocale === 'default') {
        //auto detection
        locale = navigator.language || navigator.userLanguage;
        if (locale.length === 2) {
            locale += '-';
        }
    }
    else {
        locale = newLocale;
    }
    //check if locale is available
    let localeFound = false;
    for (const l in i18n) {
        if (l === 'default') {
            continue;
        }
        if (l.indexOf(locale) === 0) {
            locale = l;
            localeFound = true;
            break;
        }
    }
    //fallback to default locale
    if (localeFound === false) {
        logError('Locale "' + locale + '" not defined');
        locale = 'en-US';
    }
    //get phrases and translate dom
    httpGet(subdir + '/assets/i18n/' + locale + '.json', function(obj) {
        phrases = obj;
        i18nHtml(domCache.body);
    }, true);
}

/**
 * Translates all phrases in the dom
 * @param {HTMLElement} root root element to translate
 */
function i18nHtml(root) {
    const attributes = [
        ['data-phrase', 'textContent'],
        ['data-title-phrase', 'title'],
        ['data-label-phrase', 'label'],
        ['data-placeholder-phrase', 'placeholder']
    ];
    for (let i = 0, j = attributes.length; i < j; i++) {
        const els = root.querySelectorAll('[' + attributes[i][0] + ']');
        const elsLen = els.length;
        for (let k = 0; k < elsLen; k++) {
            //get phrase data
            const data = els[k].getAttribute('data-phrase-data');
            let dataObj = {};
            if (data !== null) {
                dataObj = JSON.parse(data);
            }
            //add smartCount to data from data-phrase-number attribute
            const smartCount = els[k].getAttribute('data-phrase-number');
            if (smartCount !== null) {
                dataObj.smartCount = Number(smartCount);
            }
            //translate
            els[k][attributes[i][1]] = tn(els[k].getAttribute(attributes[i][0]), dataObj);
        }
    }
}
