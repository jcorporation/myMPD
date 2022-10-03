"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function smartCount(number) {
    if (number === 0) { return 1; }
    else if (number === 1) { return 0; }
    else { return 1; }
}

function tn(phrase, number, data) {
    if (isNaN(phrase) === false) {
        //do not translate numbers
        return phrase;
    }
    if (phrase === undefined) {
        logError('Phrase is undefined');
        return 'undefinedPhrase';
    }
    let result = phrases[phrase];
/*debug*/    if (result !== undefined &&
/*debug*/        locale !== 'en-US')
/*debug*/    {
/*debug*/        logDebug('Phrase "' + phrase + '" for locale ' + locale + ' not found');
/*debug*/    }

    //fallback if phrase is not translated
    if (result === undefined) {
        result = phrasesDefault[phrase] !== undefined ? phrasesDefault[phrase] : phrase;
    }

    if (isNaN(number)) {
        data = number;
    }
    else {
        const p = result.split(' |||| ');
        if (p.length > 1) {
            result = p[smartCount(number)];
        }
        result = result.replace('%{smart_count}', number);
    }

    if (data !== null) {
        //eslint-disable-next-line no-useless-escape
        const tnRegex = /%\{(\w+)\}/g;
        result = result.replace(tnRegex, function(m0, m1) {
            return data[m1];
        });
    }
    return result;
}

function localeDate(secs) {
    return new Date(secs * 1000).toLocaleString(locale);
}

function beautifyDuration(x) {
    const days = Math.floor(x / 86400);
    const hours = Math.floor(x / 3600) - days * 24;
    const minutes = Math.floor(x / 60) - hours * 60 - days * 1440;
    const seconds = x - days * 86400 - hours * 3600 - minutes * 60;

    return (days > 0 ? days + smallSpace + tn('Days') + ' ' : '') +
        (hours > 0 ? hours + smallSpace + tn('Hours') + ' ' +
        (minutes < 10 ? '0' : '') : '') + minutes + smallSpace + tn('Minutes') + ' ' +
        (seconds < 10 ? '0' : '') + seconds + smallSpace + tn('Seconds');
}

function beautifySongDuration(x) {
    const hours = Math.floor(x / 3600);
    const minutes = Math.floor(x / 60) - hours * 60;
    const seconds = Math.floor(x - hours * 3600 - minutes * 60);

    return (hours > 0 ? hours + ':' + (minutes < 10 ? '0' : '') : '') +
        minutes + ':' + (seconds < 10 ? '0' : '') + seconds;
}

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
        for (let k = 0, l = elsLen; k < l; k++) {
            const smartNumber = els[k].getAttribute('data-phrase-number');
            if (smartNumber !== null) {
                els[k][attributes[i][1]] = tn(els[k].getAttribute(attributes[i][0]), smartNumber);
            }
            else {
                const data = els[k].getAttribute('data-phrase-data');
                if (data !== null) {
                    const dataObj = JSON.parse(data);
                    els[k][attributes[i][1]] = tn(els[k].getAttribute(attributes[i][0]), dataObj);
                }
                else {
                    els[k][attributes[i][1]] = tn(els[k].getAttribute(attributes[i][0]));
                }
            }
        }
    }
}
