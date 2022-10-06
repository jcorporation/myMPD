"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/**
 * Gets the currently opened modal
 * @returns {Element} the opened modal or null if no modal is opened
 */
function getOpenModal() {
    const modals = document.querySelectorAll('.modal');
    for (const modal of modals) {
        if (modal.classList.contains('show')) {
            return modal;
        }
    }
    return null;
}

/**
 * Sets the updating indicator(s) for a view with the given id
 * @param {String} id 
 */
function setUpdateViewId(id) {
    setUpdateView(document.getElementById(id));
}

/**
 * Sets the updating indicator(s) for the element
 * @param {Element} el 
 */
function setUpdateView(el) {
    el.classList.add('opacity05');
    domCache.main.classList.add('border-progress');
}

/**
 * Removes the updating indicator(s) for a view with the given id
 * @param {String} id 
 */
function unsetUpdateViewId(id) {
    unsetUpdateView(document.getElementById(id));
}

/**
 * Removes the updating indicator(s) for the element
 * @param {Element} el 
 */
function unsetUpdateView(el) {
    el.classList.remove('opacity05');
    domCache.main.classList.remove('border-progress');
}

/**
 * Replaces special characters with underscore
 * @param {String} x string to replace
 */
function r(x) {
    return x.replace(/[^\w-]/g, '_');
}

/**
 * Removes all invalid indicators and warning messages from a modal with the given id.
 * @param {String} id id of the modal
 */
function cleanupModalId(id) {
    cleanupModal(document.getElementById(id));
}

/**
 * Removes all invalid indicators and warning messages from a modal pointed by el.
 * @param {Element} el the modal element
 */
function cleanupModal(el) {
    //remove validation warnings
    removeIsInvalid(el);
    //remove enter pin footer
    const enterPinFooter = el.querySelector('.enterPinFooter');
    if (enterPinFooter !== null) {
        removeEnterPinFooter(enterPinFooter);
    }
    //remove error messages
    hideModalAlert(el);
    //remove spinners
    const spinners = el.querySelectorAll('.spinner-border');
    for (let i = spinners.length - 1; i >= 0; i--) {
        spinners[i].remove();
    }
}

/**
 * Shows a confirmation modal
 * @param {String} text text to show (already translated)
 * @param {String} btnText text for the yes button (already translated)
 * @param {Function} callback callback function on confirmation
 */
function showConfirm(text, btnText, callback) {
    document.getElementById('modalConfirmText').textContent = text;
    const yesBtn = elCreateText('button', {"id": "modalConfirmYesBtn", "class": ["btn", "btn-danger"]}, btnText);
    yesBtn.addEventListener('click', function() {
        if (callback !== undefined &&
            typeof(callback) === 'function')
        {
            callback();
        }
        uiElements.modalConfirm.hide();
    }, false);
    document.getElementById('modalConfirmYesBtn').replaceWith(yesBtn);
    uiElements.modalConfirm.show();
}

/**
 * Shows an inline confirmation (for open modals)
 * @param {Element} el parent element to add the confirmation dialog
 * @param {String} text text to show (already translated)
 * @param {String} btnText text for the yes button (already translated)
 * @param {Function} callback callback function on confirmation
 */
function showConfirmInline(el, text, btnText, callback) {
    const confirm = elCreateNode('div', {"class": ["alert", "alert-danger", "mt-2"]},
        elCreateText('p', {}, text)
    );

    const cancelBtn = elCreateTextTn('button', {"class": ["btn", "btn-secondary"]}, 'Cancel');
    cancelBtn.addEventListener('click', function(event) {
        event.stopPropagation();
        this.parentNode.remove();
    }, false);
    confirm.appendChild(cancelBtn);

    const yesBtn = elCreateText('button', {"class": ["btn", "btn-danger", "float-end"]}, btnText);
    yesBtn.addEventListener('click', function(event) {
        event.stopPropagation();
        if (callback !== undefined &&
            typeof(callback) === 'function')
        {
            callback();
        }
        this.parentNode.remove();
    }, false);
    confirm.appendChild(yesBtn);
    el.appendChild(confirm);
}

/**
 * URL-Encodes the path, ignoring the host
 * @param {String} str 
 * @returns encoded string
 */
function myEncodeURIhost(str) {
    const match = str.match(/(https?:\/\/[^/]+)(.*)$/);
    if (match) {
        //encode only non host part of uri
        return match[1] + myEncodeURI(match[2]);
    }
    return myEncodeURI(str);
}

/**
 * Custom encoding function, works like encodeURIComponent but
 * - does not escape /
 * - escapes further reserved characters
 * @param {String} str string to encode
 * @returns the encoded string
 */
function myEncodeURI(str) {
    return encodeURI(str).replace(/[!'()*#?;:,@&=+$~]/g, function(c) {
        return '%' + c.charCodeAt(0).toString(16);
    });
}

/**
 * Custom encoding function, works like encodeURIComponent but
 * - escapes further reserved characters
 * @param {String} str string to encode
 * @returns the encoded string
 */
function myEncodeURIComponent(str) {
    return encodeURIComponent(str).replace(/[!'()*~]/g, function(c) {
        return '%' + c.charCodeAt(0).toString(16);
    });
}

/**
 * Wrapper for decodeURIComponent
 * @param {String} str uri encoded string
 * @returns decoded string
 */
function myDecodeURIComponent(str) {
    return decodeURIComponent(str);
}

/**
 * Joins an array to a comma separated text
 * @param {Array} a 
 * @returns {String} joined array
 */
function joinArray(a) {
    return a === undefined ? '' : a.join(', ');
}

/**
 * Escape a MPD filter value
 * @param {String} x value to escape
 * @returns {String} escaped value
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
 * @param {String} x value to unescape
 * @returns {String} unescaped value
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
 * Aligns a dropdown left or right from the triggering button.
 * @param {Element | EventTarget} el 
 */
function alignDropdown(el) {
    const toggleEl = el.querySelector('.dropdown-toggle');
    const x = getXpos(toggleEl);
    if (x < domCache.body.offsetWidth * 0.66) {
        el.querySelector('.dropdown-menu').classList.remove('dropdown-menu-end');
    }
    else {
        el.querySelector('.dropdown-menu').classList.add('dropdown-menu-end');
    }
}

/**
 * Sets the popover height to 2/3 of screen height
 * @param {Element} el popover element to resize
 */
function popoverHeight(el) {
    const mh = window.innerHeight / 3 * 2;
    el.style.maxHeight = mh + 'px';
    el.style.overflow = 'auto';
}

/**
 * Pads a number with zeros
 * @param {Number} num number to pad
 * @param {Number} places complete width
 * @returns {String} padded number
 */
function zeroPad(num, places) {
    const zero = places - num.toString().length + 1;
    return Array(+(zero > 0 && zero)).join("0") + num;
}

/**
 * Gets the directory from the given uri
 * @param {String} uri 
 * @returns {String}
 */
function dirname(uri) {
    return uri.replace(/\/[^/]*$/, '');
}

/**
 * Gets the filename from the given uri
 * @param {String} uri 
 * @param {Boolean} removeQuery true = remove query string or hash
 * @returns {String}
 */
function basename(uri, removeQuery) {
    if (removeQuery === true) {
        return uri.split('/').reverse()[0].split(/[?#]/)[0];
    }
    return uri.split('/').reverse()[0];
}

/**
 * Splits a string in path + filename and extension
 * @param {String} filename 
 * @returns {Object}
 */
function splitFilename(filename) {
    const parts = filename.match(/^(.*)\.([^.]+)$/);
    return {
        "file": parts[1],
        "ext": parts[2]
    };
 }

/**
 * Checks if the uri is defined as an albumart file
 * @param {*} uri 
 * @returns {Boolean}
 */
function isCoverfile(uri) {
    const filename = basename(uri, true);
    const fileparts = splitFilename(filename);

    const coverimageNames = [...settings.coverimageNames.split(','), ...settings.thumbnailNames.split(',')];
    for (let i = 0, j = coverimageNames.length; i < j; i++) {
        const name = coverimageNames[i].trim();
        if (filename === name) {
            return true;
        }
        if (name === fileparts.file &&
            imageExtensions.includes(fileparts.ext))
        {
            return true;
        }
    }
    return false;
}

/**
 * Checks if the uri is defined as an albumart thumbnail file
 * @param {*} uri 
 * @returns {Boolean}
 */
function isThumbnailfile(uri) {
    const filename = basename(uri, true);
    const fileparts = splitFilename(filename);

    const coverimageNames = settings.thumbnailNames.split(',');
    for (let i = 0, j = coverimageNames.length; i < j; i++) {
        const name = coverimageNames[i].trim();
        if (filename === name) {
            return true;
        }
        if (name === fileparts.file &&
            imageExtensions.includes(fileparts.ext))
        {
            return true;
        }
    }
    return false;
}

/**
 * Returns a description of the filetype from uri
 * @param {String} uri 
 * @returns {String} description of filetype
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
 * Gets the y-scrolling position
 * @returns {Number}
 */
function getScrollPosY() {
    if (userAgentData.isMobile === true) {
        return document.body.scrollTop ? document.body.scrollTop : document.documentElement.scrollTop;
    }
    else {
        const container = document.getElementById(app.id + 'List');
        if (container) {
            return container.parentNode.scrollTop;
        }
        else {
            return 0;
        }
    }
}

/**
 * Scrolls the container to the y-position
 * @param {Element | ParentNode} container or null
 * @param {Number} pos position to scroll
 */
function scrollToPosY(container, pos) {
    if (userAgentData.isMobile === true ||
        container === null)
    {
        // For Safari
        document.body.scrollTop = pos;
        // For Chrome, Firefox, IE and Opera
        document.documentElement.scrollTop = pos;
    }
    else {
        container.scrollTop = pos;
    }
}

/**
 * Marks a tag from a tag dropdown as active and sets the element with descId to its phrase
 * @param {String} containerId container id (dropdown)
 * @param {String} descId id of the descriptive element
 * @param {String} setTo tag to select
 */
function selectTag(containerId, descId, setTo) {
    const btns = document.getElementById(containerId);
    let aBtn = btns.querySelector('.active');
    if (aBtn !== null) {
        aBtn.classList.remove('active');
    }
    aBtn = btns.querySelector('[data-tag=' + setTo + ']');
    if (aBtn !== null) {
        aBtn.classList.add('active');
        if (descId !== undefined) {
            const descEl = document.getElementById(descId);
            if (descEl !== null) {
                descEl.textContent = aBtn.textContent;
                descEl.setAttribute('data-phrase', aBtn.getAttribute('data-phrase'));
            }
        }
    }
}

/**
 * Populates a container with buttons for tags
 * @param {String} elId id of the element to populate
 * @param {*} list taglist
 */
function addTagList(elId, list) {
    const stack = elCreateEmpty('div', {"class": ["d-grid", "gap-2"]});
    if (list === 'tagListSearch') {
        if (features.featTags === true) {
            stack.appendChild(
                elCreateTextTn('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": "any"}, 'Any Tag')
            );
        }
        stack.appendChild(
            elCreateTextTn('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": "filename"}, 'Filename')
        );
    }
    if (elId === 'searchDatabaseTags') {
        stack.appendChild(
            elCreateTextTn('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": "any"}, 'Any Tag')
        );
    }
    for (let i = 0, j = settings[list].length; i < j; i++) {
        stack.appendChild(
            elCreateTextTn('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": settings[list][i]}, settings[list][i])
        );
    }
    if (elId === 'BrowseNavFilesystemDropdown' ||
        elId === 'BrowseNavPlaylistsDropdown' ||
        elId === 'BrowseNavRadioFavoritesDropdown' ||
        elId === 'BrowseNavWebradiodbDropdown' ||
        elId === 'BrowseNavRadiobrowserDropdown')
    {
        if (features.featTags === true) {
            elClear(stack);
            stack.appendChild(
                elCreateTextTn('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": "Database"}, 'Database')
            );
        }
    }
    if (elId === 'BrowseDatabaseByTagDropdown' ||
        elId === 'BrowseNavFilesystemDropdown' ||
        elId === 'BrowseNavPlaylistsDropdown' ||
        elId === 'BrowseNavRadioFavoritesDropdown' ||
        elId === 'BrowseNavWebradiodbDropdown' ||
        elId === 'BrowseNavRadiobrowserDropdown')
    {
        if (elId === 'BrowseDatabaseByTagDropdown') {
            stack.appendChild(
                elCreateEmpty('div', {"class": ["dropdown-divider"]})
            );
        }
        stack.appendChild(
            elCreateTextTn('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": "Playlists"}, 'Playlists')
        );
        if (elId === 'BrowseNavPlaylistsDropdown') {
            stack.lastChild.classList.add('active');
        }
        stack.appendChild(
            elCreateTextTn('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": "Filesystem"}, 'Filesystem')
        );
        if (elId === 'BrowseNavFilesystemDropdown') {
            stack.lastChild.classList.add('active');
        }
        stack.appendChild(
            elCreateTextTn('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": "Radio"}, 'Webradios')
        );
        if (elId === 'BrowseNavRadioFavoritesDropdown' ||
            elId === 'BrowseNavWebradiodbDropdown' ||
            elId === 'BrowseNavRadiobrowserDropdown')
        {
            stack.lastChild.classList.add('active');
        }
    }
    else if (elId === 'databaseSortTagsList') {
        if (settings.tagList.includes('Date') === true &&
            settings[list].includes('Date') === false)
        {
            stack.appendChild(
                elCreateTextTn('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": "Date"}, 'Date')
            );
        }
        stack.appendChild(
            elCreateTextTn('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": "LastModified"}, 'Last modified')
        );
    }
    else if (elId === 'searchQueueTags') {
        if (features.featAdvqueue === true)
        {
            stack.appendChild(
                elCreateTextTn('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": "prio"}, 'Priority')
            );
        }
    }
    const el = document.getElementById(elId);
    elReplaceChild(el, stack);
}

/**
 * Populates a select element with options for tags
 * @param {String} elId id of the select to populate
 * @param {*} list taglist
 */
function addTagListSelect(elId, list) {
    const select = document.getElementById(elId);
    elClear(select);
    if (elId === 'saveSmartPlaylistSort' ||
        elId === 'selectSmartplsSort')
    {
        select.appendChild(
            elCreateTextTn('option', {"value": ""}, 'Disabled')
        );
        select.appendChild(
            elCreateTextTn('option', {"value": "shuffle"}, 'Shuffle')
        );
        const optGroup = elCreateEmpty('optgroup', {"label": tn('Sort by tag'), "data-label-phrase": "Sort by tag"});
        optGroup.appendChild(
            elCreateTextTn('option', {"value": "filename"}, 'Filename')
        );
        for (let i = 0, j = settings[list].length; i < j; i++) {
            optGroup.appendChild(
                elCreateTextTn('option', {"value": settings[list][i]}, settings[list][i])
            );
        }
        select.appendChild(optGroup);
    }
    else if (elId === 'selectJukeboxUniqueTag' &&
        settings.tagListBrowse.includes('Title') === false)
    {
        //Title tag should be always in the list
        select.appendChild(
            elCreateTextTn('option', {"value": "Title"}, 'Song')
        );
        for (let i = 0, j = settings[list].length; i < j; i++) {
            select.appendChild(
                elCreateTextTn('option', {"value": settings[list][i]}, settings[list][i])
            );
        }
    }
}

/**
 * Opens a modal
 * @param {String} modal 
 */
//eslint-disable-next-line no-unused-vars
function openModal(modal) {
    uiElements[modal].show();
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
 * @param {String} x
 * @returns {String}
 */
function genId(x) {
    return 'id' + x.replace(/[^\w-]/g, '');
}

/**
 * Parses a string to a javascript command object
 * @param {Event} event triggering event
 * @param {Object} str string to parse
 */
function parseCmdFromJSON(event, str) {
    const cmd = JSON.parse(str);
    parseCmd(event, cmd);
}

/**
 * Executes a javascript command object
 * @param {Event} event triggering event
 * @param {Object} cmd string to parse
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
    if (typeof window[cmd.cmd] === 'function') {
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
                window[cmd.cmd](event, ... cmd.options);
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
                window[cmd.cmd](event.target, ... cmd.options);
                break;
            case 'toggleBtnChkCollapse':
                // @ts-ignore
                window[cmd.cmd](event.target, undefined, ... cmd.options);
                break;
            default:
                // @ts-ignore
                window[cmd.cmd](... cmd.options);
        }
    }
    else {
        logError('Can not execute cmd: ' + cmd);
    }
}

/**
 * Creates the search breadcrumbs
 * @param {String} searchStr 
 * @param {*} searchEl se
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
 * @param {String} filter 
 * @param {String} op 
 * @param {String} value 
 * @returns {HTMLElement}
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
 * @param {String} tag tag to search
 * @param {String} op search operator
 * @param {String} value value to search
 * @returns {String} the search expression in parenthesis
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
 * @param {*} crumbsEl crumbs container element
 * @param {*} tag tag to search
 * @param {*} op search operator
 * @param {*} value value to search
 * @returns the search expression in parenthesis
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
 * Appends a link to the browse views to the element
 * @param {HTMLElement} el element to append the link
 * @param {String} tag mpd tag
 * @param {*} values tag values
 */
function printBrowseLink(el, tag, values) {
    if (settings.tagListBrowse.includes(tag)) {
        for (const value of values) {
            const link = elCreateText('a', {"href": "#"}, value);
            setData(link, 'tag', tag);
            setData(link, 'name', value);
            link.addEventListener('click', function(event) {
                event.preventDefault();
                gotoBrowse(event);
            }, false);
            el.appendChild(link);
            el.appendChild(
                elCreateEmpty('br', {})
            );
        }
    }
    else {
        el.appendChild(printValue(tag, values));
    }
}

/**
 * Returns a tag value as dom element
 * @param {String} key 
 * @param {*} value 
 * @returns {Node}
 */
function printValue(key, value) {
    if (value === undefined || value === null || value === '') {
        return document.createTextNode('-');
    }
    switch(key) {
        case 'Type':
            switch(value) {
                case 'song':     return elCreateText('span', {"class": ["mi"]}, 'music_note');
                case 'smartpls': return elCreateText('span', {"class": ["mi"]}, 'queue_music');
                case 'plist':    return elCreateText('span', {"class": ["mi"]}, 'list');
                case 'dir':      return elCreateText('span', {"class": ["mi"]}, 'folder_open');
                case 'stream':	 return elCreateText('span', {"class": ["mi"]}, 'stream');
                case 'webradio': return elCreateText('span', {"class": ["mi"]}, 'radio');
                default:         return elCreateText('span', {"class": ["mi"]}, 'radio_button_unchecked');
            }
        case 'Duration':
            return document.createTextNode(beautifySongDuration(value));
        case 'AudioFormat':
            return document.createTextNode(value.bits + tn('bits') + smallSpace + nDash + smallSpace + value.sampleRate / 1000 + tn('kHz'));
        case 'Pos':
            //mpd is 0-indexed but humans wants 1-indexed lists
            return document.createTextNode(value + 1);
        case 'LastModified':
        case 'LastPlayed':
        case 'stickerLastPlayed':
        case 'stickerLastSkipped':
            return document.createTextNode(value === 0 ? tn('never') : localeDate(value));
        case 'stickerLike':
            return elCreateText('span', {"class": ["mi"]},
                value === 0 ? 'thumb_down' : value === 1 ? 'radio_button_unchecked' : 'thumb_up');
        case 'stickerElapsed':
            return document.createTextNode(beautifySongDuration(value));
        case 'Artist':
        case 'ArtistSort':
        case 'AlbumArtist':
        case 'AlbumArtistSort':
        case 'Composer':
        case 'ComposerSort':
        case 'Performer':
        case 'Conductor':
        case 'Ensemble':
        case 'MUSICBRAINZ_ARTISTID':
        case 'MUSICBRAINZ_ALBUMARTISTID': {
            //multi value tags - print one line per value
            const span = elCreateEmpty('span', {});
            for (let i = 0, j = value.length; i < j; i++) {
                if (i > 0) {
                    span.appendChild(
                        elCreateEmpty('br', {})
                    );
                }
                if (key.indexOf('MUSICBRAINZ') === 0) {
                    span.appendChild(
                        getMBtagLink(key, value[i])
                    );
                }
                else {
                    span.appendChild(
                        document.createTextNode(value[i])
                    );
                }
            }
            return span;
        }
        case 'Genre':
            //multi value tags - print comma separated
            if (typeof value === 'string') {
                return document.createTextNode(value);
            }
            return document.createTextNode(
                value.join(', ')
            );
        case 'tags':
            //radiobrowser.info
            return document.createTextNode(
                value.replace(/,(\S)/g, ', $1')
            );
        case 'homepage':
        case 'Homepage':
            //webradios
            if (value === '') {
                return document.createTextNode(value);
            }
            return elCreateText('a', {"class": ["text-success", "external"],
                "href": myEncodeURIhost(value), "target": "_blank"}, value);
        case 'lastcheckok':
            //radiobrowser.info
            return elCreateText('span', {"class": ["mi"]},
                (value === 1 ? 'check_circle' : 'error')
            );
        case 'Bitrate':
            return document.createTextNode(value + ' ' + tn('kbit'));
        default:
            if (key.indexOf('MUSICBRAINZ') === 0) {
                return getMBtagLink(key, value);
            }
            else {
                return document.createTextNode(value);
            }
    }
}

/**
 * Gets a unix timestamp
 * @returns {Number}
 */
function getTimestamp() {
    return Math.floor(Date.now() / 1000);
}

/**
 * Toggles the collapse indicator
 * @param {HTMLElement} el 
 */
function toggleCollapseArrow(el) {
    const icon = el.querySelector('span');
    icon.textContent = icon.textContent === 'keyboard_arrow_right' ? 'keyboard_arrow_down' : 'keyboard_arrow_right';
}

/**
 * Uppercases the first letter
 * @param {String} str 
 * @returns {String}
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
 * Clears the covercache
 */
//eslint-disable-next-line no-unused-vars
function clearCovercache() {
    sendAPI("MYMPD_API_COVERCACHE_CLEAR", {}, null, false);
}

/**
 * Crops the covercache
 */
//eslint-disable-next-line no-unused-vars
function cropCovercache() {
    sendAPI("MYMPD_API_COVERCACHE_CROP", {}, null, false);
}

/**
 * Opens the picture modal
 * @param {HTMLElement} el 
 */
//eslint-disable-next-line no-unused-vars
function zoomPicture(el) {
    if (el.classList.contains('booklet')) {
        window.open(getData(el, 'href'));
        return;
    }

    if (el.classList.contains('carousel')) {
        let images;
        let embeddedImageCount;
        const dataImages = getData(el, 'images');
        if (dataImages !== undefined) {
            images = dataImages.slice();
            embeddedImageCount = getData(el, 'embeddedImageCount');
        }
        else if (currentSongObj.images) {
            images = currentSongObj.images.slice();
            embeddedImageCount = currentSongObj.embeddedImageCount;
        }
        else {
            return;
        }

        const uri = getData(el, 'uri');
        const imgEl = document.getElementById('modalPictureImg');
        imgEl.style.paddingTop = 0;
        createImgCarousel(imgEl, 'picsCarousel', uri, images, embeddedImageCount);
        elHideId('modalPictureZoom');
        uiElements.modalPicture.show();
        return;
    }

    if (el.style.backgroundImage !== '') {
        const imgEl = document.getElementById('modalPictureImg');
        elClear(imgEl);
        imgEl.style.paddingTop = '100%';
        imgEl.style.backgroundImage = el.style.backgroundImage;
        elShowId('modalPictureZoom');
        uiElements.modalPicture.show();
    }
}

/**
 * Opens the picture in a new window
 */
//eslint-disable-next-line no-unused-vars
function zoomZoomPicture() {
    window.open(document.getElementById('modalPictureImg').style.backgroundImage.match(/^url\(["']?([^"']*)["']?\)/)[1]);
}

/**
 * Creates the array of images and creates the image carousel
 * @param {HTMLElement} imgEl 
 * @param {String} name name to construct the image carousel id from
 * @param {*} uri uri of the image
 * @param {Object} images array of additional images
 * @param {Number} embeddedImageCount 
 */
function createImgCarousel(imgEl, name, uri, images, embeddedImageCount) {
    //embedded albumart
    if (embeddedImageCount === 0) {
        //enforce first coverimage
        embeddedImageCount++;
    }
    const aImages = [];
    for (let i = 0; i < embeddedImageCount; i++) {
        aImages.push(subdir + '/albumart?offset=' + i + '&uri=' + myEncodeURIComponent(uri));
    }
    //add all but coverfiles to image list
    for (let i = 0, j = images.length; i < j; i++) {
        if (isCoverfile(images[i]) === false) {
            aImages.push(subdir + myEncodeURI(images[i]));
        }
    }
    _createImgCarousel(imgEl, name, aImages);
}

/**
 * Creates the image carousel
 * @param {HTMLElement} imgEl 
 * @param {String} name 
 * @param {Object} images array of all images to display
 */
function _createImgCarousel(imgEl, name, images) {
    const nrImages = images.length;
    const carousel = elCreateEmpty('div', {"id": name, "class": ["carousel", "slide"], "data-bs-ride": "carousel"});
    if (nrImages > 1) {
        const carouselIndicators = elCreateEmpty('div', {"class": ["carousel-indicators"]});
        for (let i = 0; i < nrImages; i++) {
            carouselIndicators.appendChild(
                elCreateEmpty('button', {"type": "button", "data-bs-target": "#" + name, "data-bs-slide-to": i})
            );
            if (i === 0) {
                carouselIndicators.lastChild.classList.add('active');
            }
        }
        carousel.appendChild(carouselIndicators);
    }
    const carouselInner = elCreateEmpty('div', {"class": ["carousel-inner"]});
    for (let i = 0; i < nrImages; i++) {
        const carouselItem = elCreateNode('div', {"class": ["carousel-item"]},
            elCreateEmpty('div', {})
        );

        carouselItem.style.backgroundImage = 'url("' + images[i] + '")';
        carouselInner.appendChild(carouselItem);
        if (i === 0) {
            carouselItem.classList.add('active');
        }
    }
    carousel.appendChild(carouselInner);
    if (nrImages > 1) {
        carousel.appendChild(
            elCreateNode('a', {"href": "#" + name, "data-bs-slide": "prev", "class": ["carousel-control-prev"]},
                elCreateEmpty('span', {"class": ["carousel-control-prev-icon"]})
            )
        );
        carousel.appendChild(
            elCreateNode('a', {"href": "#" + name, "data-bs-slide": "next", "class": ["carousel-control-next"]},
                elCreateEmpty('span', {"class": ["carousel-control-next-icon"]})
            )
        );
    }

    elClear(imgEl);
    imgEl.appendChild(carousel);
    uiElements.albumartCarousel = new BSN.Carousel(carousel, {
        interval: false,
        pause: false
    });
}

/**
 * Opens a modal
 * @param {String} modal 
 */
//eslint-disable-next-line no-unused-vars
function showModal(modal) {
    uiElements[modal].show();
}

/**
 * Checks for support of the media session api
 * @returns {Boolean} true if media session api is supported, else false
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
 * Checks if tag matches the value
 * @param {String | Object} tag 
 * @param {String} value 
 * @returns {Boolean} true if tag matches value, else false
 */
function checkTagValue(tag, value) {
    if (typeof tag === 'string') {
        return tag === value;
    }
    return tag[0] === value;
}

/**
 * Converts a string to a boolean
 * @param {String} str 
 * @returns {Boolean}
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
 * @param {String} uri 
 * @returns {String}
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
 * @param {String} uri 
 * @returns {String}
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
 * @param {HTMLElement} container 
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
    }
    else {
        domCache.body.classList.add('not-mobile');
    }
}

/**
 * Generic http get request
 * @param {String} uri 
 * @param {Function} callback 
 * @param {Boolean} json true = parses the response as json, else pass the plain text response
 */
function httpGet(uri, callback, json) {
    const ajaxRequest = new XMLHttpRequest();
    ajaxRequest.open('GET', uri, true);
    ajaxRequest.onreadystatechange = function() {
        if (ajaxRequest.readyState === 4) {
            if (json === true) {
                try {
                    const obj = JSON.parse(ajaxRequest.responseText);
                    callback(obj);
                }
                catch(error) {
                    showNotification(tn('Can not parse response from %{uri} to json object', {"uri": uri}), '', 'general', 'error');
                    logError('Can not parse response from ' + uri + ' to json object.');
                    logError(error);
                }
            }
            else {
                callback(ajaxRequest.responseText);
            }
        }
    };
    ajaxRequest.send();
}
