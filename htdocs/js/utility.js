"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module utility_js */

/**
 * Sets the updating indicator(s) for a view with the given id
 * @param {string} id element id
 */
function setUpdateViewId(id) {
    setUpdateView(document.getElementById(id));
}

/**
 * Sets the updating indicator(s) for the element
 * @param {Element} el element
 */
function setUpdateView(el) {
    el.classList.add('opacity05');
    domCache.main.classList.add('border-progress');
}

/**
 * Removes the updating indicator(s) for a view with the given id
 * @param {string} id element id
 */
function unsetUpdateViewId(id) {
    unsetUpdateView(document.getElementById(id));
}

/**
 * Removes the updating indicator(s) for the element
 * @param {Element | ParentNode} el element
 */
function unsetUpdateView(el) {
    el.classList.remove('opacity05');
    domCache.main.classList.remove('border-progress');
}

/**
 * Replaces special characters with underscore
 * @param {string} x string to replace
 * @returns {string} result string
 */
function r(x) {
    return x.replace(/[^\w-]/g, '_');
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
 * @param {string} x value to escape
 * @returns {string} escaped value
 */
function escapeMPD(x) {
    if (typeof x === 'number') {
        return x;
    }
    return x.replace(/(["'])/g, function(m0, m1) {
        switch(m1) {
            case '"':  return '\\"';
            case '\'': return '\\\'';
            case '\\': return '\\\\';
        }
    });
}

/**
 * Unescape a MPD filter value
 * @param {string} x value to unescape
 * @returns {string} unescaped value
 */
function unescapeMPD(x) {
    if (typeof x === 'number') {
        return x;
    }
    return x.replace(/(\\'|\\"|\\\\)/g, function(m0, m1) {
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
        case 'BrowsePlaylistsList':
            document.getElementById('searchPlaylistsListStr').focus();
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
 * @param {object} str string to parse
 */
function parseCmdFromJSON(event, str) {
    const cmd = JSON.parse(str);
    parseCmd(event, cmd);
}

/**
 * Executes a javascript command object
 * @param {Event} event triggering event
 * @param {object} cmd string to parse
 */
function parseCmd(event, cmd) {
    if (typeof cmd === 'string') {
        //TODO: remove
        logError('Invalid type of cmd');
        parseCmdFromJSON(event, cmd);
        return;
    }
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
        document.createTextNode(filter + ' ' + op + ' \'' + value + '\''),
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
 * Toggles the collapse indicator
 * @param {HTMLElement} el arrow element to toggle
 */
function toggleCollapseArrow(el) {
    const icon = el.querySelector('span');
    icon.textContent = icon.textContent === 'keyboard_arrow_right' ? 'keyboard_arrow_down' : 'keyboard_arrow_right';
}

/**
 * Uppercases the first letter
 * @param {string} str string to change
 * @returns {string} changed string
 */
function ucFirst(str) {
    return str[0].toUpperCase() + str.slice(1);
}

/**
 * Go's into fullscreen mode
 */
//eslint-disable-next-line no-unused-vars
function openFullscreen() {
    const elem = document.documentElement;
    if (elem.requestFullscreen) {
        elem.requestFullscreen();
    }
    else if (elem.mozRequestFullScreen) {
        //Firefox
        elem.mozRequestFullScreen();
    }
    else if (elem.webkitRequestFullscreen) {
        //Chrome, Safari and Opera
        elem.webkitRequestFullscreen();
    }
    else if (elem.msRequestFullscreen) {
        //IE and Edge
        elem.msRequestFullscreen();
    }
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
 */
function setViewport() {
    document.querySelector("meta[name=viewport]").setAttribute('content', 'width=device-width, initial-scale=' +
        localSettings.scaleRatio + ', maximum-scale=' + localSettings.scaleRatio);
}

/**
 * Sets the height of the container for scrolling
 * @param {HTMLElement} container scrolling container element
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
 * Generic http get request
 * @param {string} uri uri for the request
 * @param {Function} callback callback function
 * @param {boolean} json true = parses the response as json, else pass the plain text response
 */
function httpGet(uri, callback, json) {
    const ajaxRequest = new XMLHttpRequest();
    ajaxRequest.open('GET', uri, true);
    ajaxRequest.onreadystatechange = function() {
        if (ajaxRequest.readyState === 4) {
            if (json === true) {
                let obj = {};
                try {
                    obj = JSON.parse(ajaxRequest.responseText);
                }
                catch(error) {
                    showNotification(tn('Can not parse response from %{uri} to json object', {"uri": uri}), '', 'general', 'error');
                    logError('Can not parse response from ' + uri + ' to json object.');
                    logError(error);
                }
                callback(obj);
            }
            else {
                callback(ajaxRequest.responseText);
            }
        }
    };
    ajaxRequest.send();
}
