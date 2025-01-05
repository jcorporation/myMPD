"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module utility_js */

/**
 * Ignore keys for inputs
 * @param {KeyboardEvent} event triggering key event
 * @returns {boolean} true if key event should be ignored, else false
 */
function ignoreKeys(event) {
    if (event === undefined) {
        return true;
    }
    switch (event.key) {
        case undefined:
        case 'Unidentified':
            return true;
        case 'Escape':
            // @ts-ignore
            event.target.blur();
            return true;
        case 'Backspace':
        case 'Delete':
            // do not ignore some special keys
            return false;
        // No Default
    }
    if (event.key.length > 1) {
        // ignore all special keys
        return true;
    }
    return false;
}

/**
 * Checks if event should be executed
 * @param {EventTarget} target triggering event target
 * @returns {boolean} true if target is clickable else false
 */
function checkTargetClick(target) {
    if (target === null ||
        target.classList.contains('not-clickable') ||
        target.parentNode.nodeName === 'TH')
    {
        return false;
    }
    return true;
}

/**
 * Sets the updating indicator(s) for a view with the given id
 * @param {string} id element id
 * @returns {void}
 */
function setUpdateViewId(id) {
    setUpdateView(elGetById(id));
}

/**
 * Sets the updating indicator(s) for the element
 * @param {Element} el element
 * @returns {void}
 */
function setUpdateView(el) {
    el.classList.add('opacity05');
    domCache.main.classList.add('border-progress');
}

/**
 * Removes the updating indicator(s) for a view with the given id
 * @param {string} id element id
 * @returns {void}
 */
function unsetUpdateViewId(id) {
    unsetUpdateView(elGetById(id));
}

/**
 * Removes the updating indicator(s) for the element
 * @param {Element | ParentNode} el element
 * @returns {void}
 */
function unsetUpdateView(el) {
    el.classList.remove('opacity05');
    domCache.main.classList.remove('border-progress');
}

/**
 * Custom encoding function, works like encodeURIComponent but
 * - does not escape /
 * - escapes further reserved characters
 * @param {string} str string to encode
 * @returns {string} the encoded string
 */
function myEncodeURI(str) {
    return encodeURI(str).replace(/[!'()*#?;:,@&=+$~]/g, function(c) {
        return '%' + c.charCodeAt(0).toString(16);
    });
}

/**
 * Custom encoding function, works like encodeURIComponent but
 * escapes further reserved characters
 * @param {string} str string to encode
 * @returns {string} the encoded string
 */
function myEncodeURIComponent(str) {
    return encodeURIComponent(str).replace(/[!'()*~]/g, function(c) {
        return '%' + c.charCodeAt(0).toString(16);
    });
}

/**
 * Concatenates two arrays and checks if second array is defined
 * @param {Array} c1 first array
 * @param {Array} c2 second array, can be undefined
 * @returns {Array} concatenated array
 */
function concatArrays(c1, c2) {
    return c2 === undefined
        ? c1
        : c1.concat(c2);
}

/**
 * Joins an array to a comma separated text
 * @param {Array} a array to join
 * @returns {string} joined array
 */
function joinArray(a) {
    return a === undefined
        ? ''
        : a.join(', ');
}

/**
 * Joins an array to a multi-line string
 * @param {Array} a array to join
 * @returns {string} joined array
 */
function arrayToLines(a) {
    return a === undefined
        ? ''
        : a.join('\n');
}

/**
 * Parses a comma separated string to an array
 * @param {string} str string to parse
 * @returns {Array}  Parsed string as array
 */
function stringToArray(str) {
    const a = str.split(/,/);
    for (let i = 0, j=a.length; i < j; i++) {
        a[i] = a[i].trim();
    }
    return a;
}

/**
 * Escape a MPD filter value
 * @param {string} str value to escape
 * @returns {string} escaped value
 */
function escapeMPD(str) {
    if (typeof str === 'number') {
        return str;
    }
    return str.replace(/(["'])/g, function(m0, m1) {
        switch(m1) {
            case '"':  return '\\"';
            case '\'': return '\\\'';
            case '\\': return '\\\\';
            // No Default
        }
    });
}

/**
 * Unescape a MPD filter value
 * @param {string} str value to unescape
 * @returns {string} unescaped value
 */
function unescapeMPD(str) {
    if (typeof str === 'number') {
        return str;
    }
    return str.replace(/(\\'|\\"|\\\\)/g, function(m0, m1) {
        switch(m1) {
            case '\\"':  return '"';
            case '\\\'': return '\'';
            case '\\\\': return '\\';
            // No Default
        }
    });
}

/**
 * Pads a number with zeros
 * @param {number} num number to pad
 * @param {number} places complete width
 * @returns {string} padded number
 */
function zeroPad(num, places) {
    const zero = places - num.toString().length + 1;
    return Array(+(zero > 0 && zero)).join("0") + num;
}

/**
 * Gets the directory from the given uri
 * @param {string} uri the uri
 * @returns {string} directory part of the uri
 */
function dirname(uri) {
    return uri.replace(/\/[^/]*$/, '');
}

/**
 * Gets the filename from the given uri
 * @param {string} uri the uri
 * @param {boolean} removeQuery true = remove query string or hash
 * @returns {string} filename part of the uri
 */
function basename(uri, removeQuery) {
    if (removeQuery === true) {
        return uri.split('/').reverse()[0].split(/[?#]/)[0];
    }
    return uri.split('/').reverse()[0];
}

/**
 * Splits a string in path + filename and extension
 * @param {string} filename filename to split
 * @returns {object} object with file and ext keys
 */
function splitFilename(filename) {
    const parts = filename.match(/^(.*)\.([^.]+)$/);
    return {
        "file": parts[1],
        "ext": parts[2]
    };
 }

/**
 * Returns a description of the filetype from uri
 * @param {string} uri the uri
 * @param {boolean} long return long description?
 * @returns {string} description of filetype
 */
function filetype(uri, long) {
    if (uri === undefined) {
        return '';
    }
    if (isStreamUri(uri) === true) {
        return 'Stream';
    }
    const ext = uri.split('.').pop().toUpperCase();
    if (long === false) {
        return ext;
    }
    switch(ext) {
        case 'MP3':  return ext + smallSpace + nDash + smallSpace + tn('MPEG-1 Audio Layer III');
        case 'FLAC': return ext + smallSpace + nDash + smallSpace + tn('Free Lossless Audio Codec');
        case 'OGG':  return ext + smallSpace + nDash + smallSpace + tn('Ogg Vorbis');
        case 'OPUS': return ext + smallSpace + nDash + smallSpace + tn('Opus Audio');
        case 'WAV':  return ext + smallSpace + nDash + smallSpace + tn('WAVE Audio File');
        case 'WV':   return ext + smallSpace + nDash + smallSpace + tn('WavPack');
        case 'AAC':  return ext + smallSpace + nDash + smallSpace + tn('Advanced Audio Coding');
        case 'MPC':  return ext + smallSpace + nDash + smallSpace + tn('Musepack');
        case 'MP4':  return ext + smallSpace + nDash + smallSpace + tn('MPEG-4');
        case 'APE':  return ext + smallSpace + nDash + smallSpace + tn('Monkey Audio');
        case 'WMA':  return ext + smallSpace + nDash + smallSpace + tn('Windows Media Audio');
        case 'CUE':  return ext + smallSpace + nDash + smallSpace + tn('Cuesheet');
        default:     return ext;
    }
}

/**
 * View specific focus of the search input
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function focusSearch() {
    const searchInput = elGetById(app.id + 'SearchStr');
    if (searchInput !== null) {
        searchInput.focus();
    }
    else {
        appGoto('Search');
    }
}

/**
 * Parses a string to a javascript command object
 * @param {Event} event triggering event
 * @param {string} str string to parse
 * @returns {void}
 */
function parseCmdFromJSON(event, str) {
    const cmd = JSON.parse(str);
    parseCmd(event, cmd);
}

/**
 * Executes a javascript command object
 * @param {Event} event triggering event
 * @param {object} cmd cmd object
 * @returns {void}
 */
function parseCmd(event, cmd) {
    if (event !== null &&
        event !== undefined)
    {
        event.preventDefault();
    }
    const func = getFunctionByName(cmd.cmd);
    if (typeof func === 'function') {
        if (cmd.cmd === 'sendAPI') {
            sendAPI(cmd.options[0].cmd, {}, null, false);
        }
        else {
            // copy - we do not want to modify the original object
            const options = cmd.options.slice();
            for (let i = 0, j = options.length; i < j; i++) {
                if (options[i] === 'event') {
                    options[i] = event;
                }
                else if (options[i] === 'target') {
                    options[i] = event.target;
                }
            }
            // @ts-ignore
            func(... options);
        }
    }
    else {
        logError('Can not execute cmd: ' + cmd.cmd);
    }
}

/**
 * Returns the function by name
 * @param {string} functionName name of the function
 * @returns {Function} the function
 */
 function getFunctionByName(functionName) {
    const namespace = functionName.split('.');
    if (namespace.length === 2) {
        const context = namespace.shift();
        const functionToExecute = namespace.shift();
        return window[context][functionToExecute];
    }
    return window[functionName];
}

/**
 * Checks for support of the media session api
 * @returns {boolean} true if media session api is supported, else false
 */
function checkMediaSessionSupport() {
    if (settings.mediaSession === false ||
        navigator.mediaSession === undefined)
    {
        return false;
    }
    return true;
}

/**
 * Uppercases the first letter of a word
 * @param {string} word word to uppercase first letter
 * @returns {string} changed word
 */
function ucFirst(word) {
    return word.charAt(0).toUpperCase() + word.slice(1);
}

/**
 * Returns the cuesheet name
 * @param {string} uri uri to check
 * @returns {string} the uri part
 */
function cuesheetUri(uri) {
    const cuesheet = uri.match(/^(.*\.cue)\/(track\d+)$/);
    if (cuesheet !== null) {
        return cuesheet[1];
    }
    return uri;
}

/**
 * Returns the cuesheet track name
 * @param {string} uri uri to check
 * @returns {string} the track part
 */
function cuesheetTrack(uri) {
    const cuesheet = uri.match(/^(.*\.cue)\/(track\d+)$/);
    if (cuesheet !== null) {
        return cuesheet[2];
    }
    return '';
}

/**
 * Sets the viewport tag scaling option
 * @returns {void}
 */
function setViewport() {
    document.querySelector("meta[name=viewport]").setAttribute('content', 'width=device-width, initial-scale=' +
        localSettings.scaleRatio + ', maximum-scale=' + localSettings.scaleRatio);
}

/**
 * Sets the height of the container for scrolling
 * @param {HTMLElement} container scrolling container element
 * @returns {void}
 */
function setScrollViewHeight(container) {
    if (userAgentData.isMobile === true) {
        //no scrolling container in the mobile view
        container.parentNode.style.maxHeight = '';
        return;
    }
    const footerHeight = domCache.footer.offsetHeight;
    const tpos = getYpos(container.parentNode);
    const maxHeight = window.innerHeight - tpos - footerHeight;
    container.parentNode.style.maxHeight = maxHeight + 'px';
}

/**
 * Enables the mobile view for specific user agents
 * @returns {void}
 */
function setMobileView() {
    if (userAgentData.isMobile === true) {
        setViewport();
        domCache.body.classList.remove('not-mobile');
        domCache.body.classList.add('mobile');
    }
    else {
        domCache.body.classList.remove('mobile');
        domCache.body.classList.add('not-mobile');
    }
}

/**
 * Generic http get request (async function)
 * @param {string} uri uri for the request
 * @param {Function} callback callback function
 * @param {boolean} json true = parses the response as json, else pass the plain text response
 * @returns {Promise<void>}
 */
async function httpGet(uri, callback, json) {
    let response = null;
    try {
        response = await fetch(uri, {
            method: 'GET',
            mode: 'same-origin',
            credentials: 'same-origin',
            cache: 'no-store',
            redirect: 'follow'
        });
    }
    catch(error) {
        showNotification(tn('API error') + ':\n' + tn('Error accessing %{uri}', {"uri": uri}), 'general', 'error');
        logError('Error posting to ' + uri);
        logError(error);
        callback(null);
        return;
    }

    if (response.redirected === true) {
        logError('Request was redirect, reloading application');
        window.location.reload();
        return;
    }
    if (response.ok === false) {
        showNotification(tn('API error') + '\n' +
            tn('Error accessing %{uri}', {"uri": uri}) + ',\n' +
            tn('Response code: %{code}', {"code": response.status + ' - ' + response.statusText}),
            'general', 'error');
        logError('Error accessing ' + uri + ', code ' + response.status + ' - ' + response.statusText);
        callback(null);
        return;
    }

    let data;
    try {
        data = json === true
            ? await response.json()
            : await response.text();
    }
    catch(error) {
        showNotification(tn('API error') + '\n' + tn('Can not parse response from %{uri}', {"uri": uri}), 'general', 'error');
        logError('Can not parse response from ' + uri);
        logError(error);
        callback(null);
        return;
    }
    callback(data);
}

/**
 * Returns the myMPD uri calculated from the window location
 * @param {string} [proto] protocol to return, allowed: http or ws
 * @returns {string} myMPD uri
 */
function getMyMPDuri(proto) {
    const protocol = proto === 'ws'
        ? window.location.protocol === 'https:'
            ? 'wss:'
            : 'ws:'
        : window.location.protocol;
    return protocol + '//' + window.location.hostname +
        (window.location.port !== '' ? ':' + window.location.port : '') +
        subdir;
}

/**
 * Parses a string to seconds
 * @param {string} value [hh:][mm:]ss value to parse
 * @returns {number} value in seconds
 */
function parseToSeconds(value) {
    let match = value.match(/(\d+):(\d+):(\d+)/);
    if (match) {
        return Number(match[1]) * 60 * 60 +
            Number(match[2]) * 60 +
            Number(match[3]);
    }
    match = value.match(/(\d+):(\d+)/);
    if (match) {
        return Number(match[1]) * 60 +
            Number(match[2]);
    }
    return Number(value);
}

/**
 * Initializes elements with data-href attribute
 * @param {Node} root root of the elements to initialize
 * @returns {void}
 */
function initLinks(root) {
    const hrefs = root.querySelectorAll('[data-href]');
    for (const href of hrefs) {
        if (href.nodeName !== 'A' &&
            href.nodeName !== 'BUTTON' &&
            href.classList.contains('not-clickable') === false)
        {
            href.classList.add('clickable');
        }
        if (href.parentNode.classList.contains('noInitChilds') ||
            href.parentNode.parentNode.classList.contains('noInitChilds'))
        {
            //handler on parentnode
            continue;
        }
        href.addEventListener('click', function(event) {
            parseCmdFromJSON(event, getData(this, 'href'));
        }, false);
    }
}

/**
 * Tries to convert a string to number or bool
 * @param {string} str string to convert
 * @returns {string|number|boolean} parsed string
 */
function convertType(str) {
    if (str === 'true') {
        return true;
    }
    if (str === 'false') {
        return false;
    }
    if (str.match(/^(-)?[\d.]+$/)) {
        return Number(str);
    }
    return str;
}

/**
 * Gets a unix timestamp
 * @returns {number} the unix timestamp
 */
function getTimestamp() {
    return Math.floor(Date.now() / 1000);
}

/**
 * Parses a YYYY-MM-DD string to unix timestamp
 * @param {string} value string to parses
 * @returns {number} unix timestamp
 */
function parseDateFromText(value) {
    const m = value.match(/(\d{4})-(\d{2})-(\d{2})/);
    if (m !== null) {
        return Date.parse(value) / 1000;
    }
    return NaN;
}
