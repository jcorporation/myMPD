"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module utility_js */

/**
 * Ignore keys for inputs
 * @param {KeyboardEvent} event triggering key event
 * @returns {boolean} true if key event should be ignored, else false
 */
function ignoreKeys(event) {
    if (event === undefined ||
        event.key === undefined)
    {
        return true;
    }
    switch (event.key) {
        case 'Escape':
            // @ts-ignore
            event.target.blur();
            return true;
        case 'Backspace':
        case 'Delete':
            // do not ignore some special keys
            return false;
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
    return target === null || target.classList.contains('not-clickable') ? false : true;
}

/**
 * Sets the updating indicator(s) for a view with the given id
 * @param {string} id element id
 * @returns {void}
 */
function setUpdateViewId(id) {
    setUpdateView(document.getElementById(id));
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
    unsetUpdateView(document.getElementById(id));
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
 * Replaces special characters with underscore
 * @param {string} str string to replace
 * @returns {string} result string
 */
function r(str) {
    return str.replace(/[^\w-]/g, '_');
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
 * Joins an array to a comma separated text
 * @param {Array} a array to join
 * @returns {string} joined array
 */
function joinArray(a) {
    return a === undefined ? '' : a.join(', ');
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
 * @returns {string} description of filetype
 */
function filetype(uri) {
    if (uri === undefined) {
        return '';
    }
    const ext = uri.split('.').pop().toUpperCase();
    switch(ext) {
        case 'MP3':  return ext + ' - ' + tn('MPEG-1 Audio Layer III');
        case 'FLAC': return ext + ' - ' + tn('Free Lossless Audio Codec');
        case 'OGG':  return ext + ' - ' + tn('Ogg Vorbis');
        case 'OPUS': return ext + ' - ' + tn('Opus Audio');
        case 'WAV':  return ext + ' - ' + tn('WAVE Audio File');
        case 'WV':   return ext + ' - ' + tn('WavPack');
        case 'AAC':  return ext + ' - ' + tn('Advanced Audio Coding');
        case 'MPC':  return ext + ' - ' + tn('Musepack');
        case 'MP4':  return ext + ' - ' + tn('MPEG-4');
        case 'APE':  return ext + ' - ' + tn('Monkey Audio');
        case 'WMA':  return ext + ' - ' + tn('Windows Media Audio');
        case 'CUE':  return ext + ' - ' + tn('Cuesheet');
        default:     return ext;
    }
}

/**
 * View specific focus of the search input
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function focusSearch() {
    switch(app.id) {
        case 'QueueCurrent':
            document.getElementById('searchQueueStr').focus();
            break;
        case 'QueueLastPlayed':
            document.getElementById('searchQueueLastPlayedStr').focus();
            break;
        case 'QueueJukebox':
            document.getElementById('searchQueueJukeboxStr').focus();
            break;
        case 'BrowseDatabaseList':
            document.getElementById('searchDatabaseStr').focus();
            break;
        case 'BrowseFilesystem':
            document.getElementById('searchFilesystemStr').focus();
            break;
        case 'BrowsePlaylistList':
            document.getElementById('searchPlaylistListStr').focus();
            break;
        case 'BrowsePlaylistsDetail':
            document.getElementById('searchPlaylistsDetailStr').focus();
            break;
        case 'BrowseRadioWebradiodb':
            document.getElementById('BrowseRadioWebradiodbSearchStr').focus();
            break;
        case 'BrowseRadioRadiobrowser':
            document.getElementById('BrowseRadioRadiobrowserSearchStr').focus();
            break;
        case 'Search':
            document.getElementById('searchStr').focus();
            break;
        default:
            appGoto('Search');
    }
}

/**
 * Generates a valid id from string
 * @param {string} str string to generate the id from
 * @returns {string} the generated id
 */
function genId(str) {
    return 'id' + str.replace(/[^\w-]/g, '');
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
 * @param {object} cmd string to parse
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
        for (let i = 0, j = cmd.options.length; i < j; i++) {
            if (cmd.options[i] === 'event') {
                cmd.options[i] = event;
            }
        }
        switch(cmd.cmd) {
            case 'sendAPI':
                sendAPI(cmd.options[0].cmd, {}, null, false);
                break;
            case 'createLocalPlaybackEl':
                // @ts-ignore
                func(event, ... cmd.options);
                break;
            case 'toggleBtn':
            case 'toggleBtnChk':
            case 'toggleBtnGroup':
            case 'toggleBtnGroupCollapse':
            case 'zoomPicture':
            case 'setPlaySettings':
            case 'voteSong':
            case 'toggleAddToPlaylistFrm':
            case 'toggleSaveQueueMode':
                // @ts-ignore
                func(event.target, ... cmd.options);
                break;
            case 'toggleBtnChkCollapse':
                // @ts-ignore
                func(event.target, undefined, ... cmd.options);
                break;
            default:
                // @ts-ignore
                func(... cmd.options);
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
 * Creates the search breadcrumbs from a mpd search expression
 * @param {string} searchStr the search expression
 * @param {HTMLElement} searchEl search input element
 * @param {HTMLElement} crumbEl element to add the crumbs
 * @returns {void}
 */
function createSearchCrumbs(searchStr, searchEl, crumbEl) {
    elClear(crumbEl);
    const elements = searchStr.substring(1, app.current.search.length - 1).split(' AND ');
    //add all but last element to crumbs
    for (let i = 0, j = elements.length - 1; i < j; i++) {
        const fields = elements[i].match(/^\((\w+)\s+(\S+)\s+'(.*)'\)$/);
        if (fields !== null && fields.length === 4) {
            crumbEl.appendChild(createSearchCrumb(fields[1], fields[2], unescapeMPD(fields[3])));
        }
    }
    //check if we should add the last element to the crumbs
    if (searchEl.value === '' &&
        elements.length >= 1)
    {
        const fields = elements[elements.length - 1].match(/^\((\w+)\s+(\S+)\s+'(.*)'\)$/);
        if (fields !== null && fields.length === 4) {
            crumbEl.appendChild(createSearchCrumb(fields[1], fields[2], unescapeMPD(fields[3])));
        }
    }
    crumbEl.childElementCount > 0 ? elShow(crumbEl) : elHide(crumbEl);
}

/**
 * Creates a search crumb element
 * @param {string} filter the tag
 * @param {string} op search operator
 * @param {string} value filter value
 * @returns {HTMLElement} search crumb element
 */
function createSearchCrumb(filter, op, value) {
    const btn = elCreateNodes('button', {"class": ["btn", "btn-dark", "me-2"]}, [
        document.createTextNode(tn(filter) + ' ' + tn(op) + ' \'' + value + '\''),
        elCreateText('span', {"class": ["ml-2", "badge", "bg-secondary"]}, '×')
    ]);
    setData(btn, 'filter-tag', filter);
    setData(btn, 'filter-op', op);
    setData(btn, 'filter-value', value);
    return btn;
}

/**
 * Creates a MPD search expression
 * @param {string} tag tag to search
 * @param {string} op search operator
 * @param {string} value value to search
 * @returns {string} the search expression in parenthesis
 */
function _createSearchExpression(tag, op, value) {
    if (op === 'starts_with' &&
        app.id !== 'BrowseDatabaseList' &&
        features.featStartsWith === false)
    {
        //mpd does not support starts_with, convert it to regex
        if (features.featPcre === true) {
            //regex is supported
            op = '=~';
            value = '^' + value;
        }
        else {
            //best option without starts_with and regex is contains
            op = 'contains';
        }
    }
    return '(' + tag + ' ' + op + ' ' +
        (op === '>=' ? value : '\'' + escapeMPD(value) + '\'') +
        ')';
}

/**
 * Creates the MPD search expression from crumbs and parameters
 * @param {HTMLElement} crumbsEl crumbs container element
 * @param {string} tag tag to search
 * @param {string} op search operator
 * @param {string} value value to search
 * @returns {string} the search expression in parenthesis
 */
function createSearchExpression(crumbsEl, tag, op, value) {
    let expression = '(';
    const crumbs = crumbsEl.children;
    for (let i = 0, j = crumbs.length; i < j; i++) {
        if (i > 0) {
            expression += ' AND ';
        }
        expression += _createSearchExpression(
            getData(crumbs[i], 'filter-tag'),
            getData(crumbs[i], 'filter-op'),
            getData(crumbs[i], 'filter-value')
        );
    }
    if (value !== '') {
        if (expression.length > 1) {
            expression += ' AND ';
        }
        expression += _createSearchExpression(tag, op, value);
    }
    expression += ')';
    if (expression.length <= 2) {
        expression = '';
    }
    return expression;
}

/**
 * Gets a unix timestamp
 * @returns {number} the unix timestamp
 */
function getTimestamp() {
    return Math.floor(Date.now() / 1000);
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
 * Converts a string to a boolean
 * @param {string} str string to parse
 * @returns {boolean} the boolean value
 */
function strToBool(str) {
    return str === 'true';
}

/**
 * Removes the search timer
 * @returns {void}
 */
function clearSearchTimer() {
    if (searchTimer !== null) {
        clearTimeout(searchTimer);
        searchTimer = null;
    }
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
    const response = await fetch(uri, {
        method: 'GET',
        mode: 'same-origin',
        credentials: 'same-origin',
        cache: 'no-store',
        redirect: 'follow'
    });
    if (response.redirected === true) {
        window.location.reload();
        logError('Request was redirect, reloading application');
        return;
    }
    if (response.ok === false) {
        showNotification(tn('API error'),
            tn('Error accessing %{uri}, Response code: %{code}', {"uri": uri, "code": response.status + ' - ' + response.statusText}),
            'general', 'error');
        logError('Error accessing ' + uri + ', code ' + response.status + ' - ' + response.statusText);
        callback(null);
        return;
    }

    try {
        const data = json === true
            ? await response.json()
            : await response.text();
        callback(data);
    }
    catch(error) {
        showNotification(tn('API error'), tn('Can not parse response from %{uri}', {"uri": uri}), 'general', 'error');
        logError('Can not parse response from ' + uri);
        logError(error);
        callback(null);
    }
}
