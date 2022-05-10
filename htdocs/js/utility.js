"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

//element handling shortcuts
function elCreateText(tagName, attributes, text) {
    const tag = elCreateEmpty(tagName, attributes);
    tag.textContent = text;
    return tag;
}

function elCreateNode(tagName, attributes, node) {
    const tag = elCreateEmpty(tagName, attributes);
    tag.appendChild(node);
    return tag;
}

function elCreateNodes(tagName, attributes, nodes) {
    const tag = elCreateEmpty(tagName, attributes);
    for (const node of nodes) {
        tag.appendChild(node);
    }
    return tag;
}

function elCreateEmpty(tagName, attributes) {
    const tag = document.createElement(tagName);
    for (const key in attributes) {
        switch(key) {
            case 'class':
                tag.classList.add(...attributes[key]);
                break;
            case 'is':
                tag.setAttribute('data-is', attributes[key]);
                break;
            default:
                tag.setAttribute(key, attributes[key]);
        }
    }
    return tag;
}

function elReplaceChildId(id, child) {
    elReplaceChild(document.getElementById(id), child);
}

function elReplaceChild(el, child) {
    elClear(el);
    el.appendChild(child);
}

function elHideId(id) {
    document.getElementById(id).classList.add('d-none');
}

function elShowId(id) {
    document.getElementById(id).classList.remove('d-none');
}

function elClearId(id) {
    document.getElementById(id).textContent = '';
}

function elHide(el) {
    el.classList.add('d-none');
}

function elShow(el) {
    el.classList.remove('d-none');
}

function elClear(el) {
    el.textContent = '';
}

function elDisableId(el) {
    document.getElementById(el).setAttribute('disabled', 'disabled');
}

function elDisable(el) {
    el.setAttribute('disabled', 'disabled');
    //manually disabled, remove disabled class
    el.classList.remove('disabled');
    el.classList.replace('clickable', 'not-clickable');
}

function elEnableId(el) {
    document.getElementById(el).removeAttribute('disabled');
}

function elEnable(el) {
    el.removeAttribute('disabled');
    el.classList.replace('not-clickable', 'clickable');
}

function elReflow(el) {
    return el.offsetHeight;
}

function getOpenModal() {
    const modals = document.getElementsByClassName('modal');
    for (const modal of modals) {
        if (modal.classList.contains('show')) {
            return modal;
        }
    }
    return null;
}

function setFocusId(id) {
    setFocus(document.getElementById(id));
}

function setFocus(el) {
    if (userAgentData.isMobile === false) {
        el.focus();
    }
}

//replaces special characters with underscore
function r(x) {
    return x.replace(/[^\w-]/g, '_');
}

function cleanupModalId(id) {
    cleanupModal(document.getElementById(id));
}

function cleanupModal(el) {
    //remove validation warnings
    removeIsInvalid(el);
    //remove enter pin footer
    const enterPinFooter = el.getElementsByClassName('enterPinFooter');
    if (enterPinFooter.length > 0) {
        removeEnterPinFooter(enterPinFooter[0]);
    }
    //remove error messages
    hideModalAlert(el);
    //remove spinners
    const spinners = el.getElementsByClassName('spinner-border');
    for (let i = spinners.length - 1; i >= 0; i--) {
        spinners[i].remove();
    }
}

//confirmation dialogs
function showConfirm(text, btnText, callback) {
    document.getElementById('modalConfirmText').textContent = text;
    const yesBtn = elCreateText('button', {"id": "modalConfirmYesBtn", "class": ["btn", "btn-danger"]}, btnText);
    yesBtn.addEventListener('click', function() {
        if (callback !== undefined && typeof(callback) === 'function') {
            callback();
        }
        uiElements.modalConfirm.hide();
    }, false);
    document.getElementById('modalConfirmYesBtn').replaceWith(yesBtn);
    uiElements.modalConfirm.show();
}

function showConfirmInline(el, text, btnText, callback) {
    const confirm = elCreateNode('div', {"class": ["alert", "alert-danger", "mt-2"]},
        elCreateText('p', {}, text)
    );

    const cancelBtn = elCreateText('button', {"class": ["btn", "btn-secondary"]}, tn('Cancel'));
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

function myEncodeURIhost(str) {
    const match = str.match(/(https?:\/\/[^/]+)(.*)$/);
    if (match) {
        //encode only non host part of uri
        return match[1] + myEncodeURI(match[2]);
    }
    return myEncodeURI(str);
}

//custom encoding function
//works like encodeURIComponent but
//- does not escape /
//- escapes further reserved characters
function myEncodeURI(str) {
    return encodeURI(str).replace(/[!'()*#?;:,@&=+$~]/g, function(c) {
        return '%' + c.charCodeAt(0).toString(16);
    });
}

function myEncodeURIComponent(str) {
    return encodeURIComponent(str).replace(/[!'()*~]/g, function(c) {
        return '%' + c.charCodeAt(0).toString(16);
    });
}

function myDecodeURIComponent(str) {
    return decodeURIComponent(str);
}

function joinArray(a) {
    return a === undefined ? '' : a.join(', ');
}

//functions to execute default actions
function clickQuickRemove(target) {
    switch(app.id) {
        case 'QueueCurrent': {
            const songId = getData(target.parentNode.parentNode, 'songid');
            removeFromQueue('single', songId);
            break;
        }
        case 'BrowsePlaylistsDetail': {
            const pos = getData(target.parentNode.parentNode, 'songpos');
            const plist = getDataId('BrowsePlaylistsDetailList', 'uri');
            removeFromPlaylist('single', plist, pos);
            break;
        }
    }
}

function clickQuickPlay(target) {
    const type = getData(target.parentNode.parentNode, 'type');
    let uri = getData(target.parentNode.parentNode, 'uri');
    if (type === 'webradio') {
        uri = getRadioFavoriteUri(uri);
    }
    switch(settings.webuiSettings.clickQuickPlay) {
        case 'append': return appendQueue(type, uri);
        case 'appendPlay': return appendPlayQueue(type, uri);
        case 'insertAfterCurrent': return insertAfterCurrentQueue(type, uri);
        case 'insertPlayAfterCurrent': return insertPlayAfterCurrentQueue(type, uri);
        case 'replace': return replaceQueue(type, uri);
        case 'replacePlay': return replacePlayQueue(type, uri);
    }
}

function clickAlbumPlay(albumArtist, album) {
    switch(settings.webuiSettings.clickQuickPlay) {
        case 'append': return _addAlbum('appendQueue', albumArtist, album);
        case 'appendPlay': return _addAlbum('appendPlayQueue', albumArtist, album);
        case 'insertAfterCurrent': return _addAlbum('insertAfterCurrentQueue', albumArtist, album);
        case 'insertPlayAfterCurrent': return _addAlbum('insertPlayAfterCurrentQueue', albumArtist, album);
        case 'replace': return _addAlbum('replaceQueue', albumArtist, album);
        case 'replacePlay': return _addAlbum('replacePlayQueue', albumArtist, album);
    }
}

function clickSong(uri) {
    switch (settings.webuiSettings.clickSong) {
        case 'append': return appendQueue('song', uri);
        case 'appendPlay': return appendPlayQueue('song', uri);
        case 'insertAfterCurrent': return insertAfterCurrentQueue('song', uri);
        case 'insertPlayAfterCurrent': return insertPlayAfterCurrentQueue('song', uri);
        case 'replace': return replaceQueue('song', uri);
        case 'replacePlay': return replacePlayQueue('song', uri);
        case 'view': return songDetails(uri);
    }
}

function clickRadiobrowser(uri, uuid) {
    switch (settings.webuiSettings.clickRadiobrowser) {
        case 'append': return appendQueue('song', uri);
        case 'appendPlay': return appendPlayQueue('song', uri);
        case 'insertAfterCurrent': return insertAfterCurrentQueue('song', uri);
        case 'insertPlayAfterCurrent': return insertPlayAfterCurrentQueue('song', uri);
        case 'replace': return replaceQueue('song', uri);
        case 'replacePlay': return replacePlayQueue('song', uri);
    }
    countClickRadiobrowser(uuid);
}

function clickWebradiodb(uri) {
    switch (settings.webuiSettings.clickRadiobrowser) {
        case 'append': return appendQueue('song', uri);
        case 'appendPlay': return appendPlayQueue('song', uri);
        case 'insertAfterCurrent': return insertAfterCurrentQueue('song', uri);
        case 'insertPlayAfterCurrent': return insertPlayAfterCurrentQueue('song', uri);
        case 'replace': return replaceQueue('song', uri);
        case 'replacePlay': return replacePlayQueue('song', uri);
    }
}

function clickRadioFavorites(uri) {
    const fullUri = getRadioFavoriteUri(uri);
    switch(settings.webuiSettings.clickRadioFavorites) {
        case 'append': return appendQueue('plist', fullUri);
        case 'appendPlay': return appendPlayQueue('plist', fullUri);
        case 'insertAfterCurrent': return insertAfterCurrentQueue('plist', fullUri);
        case 'insertPlayAfterCurrent': return insertPlayAfterCurrentQueue('plist', fullUri);
        case 'replace': return replaceQueue('plist', fullUri);
        case 'replacePlay': return replacePlayQueue('plist', fullUri);
        case 'edit': return editRadioFavorite(uri);
    }
}

function clickQueueSong(songid, uri) {
    switch(settings.webuiSettings.clickQueueSong) {
        case 'play':
            if (songid === null) {
                return;
            }
            sendAPI("MYMPD_API_PLAYER_PLAY_SONG", {
                "songId": songid
            });
            break;
        case 'view':
            if (uri === null) {
                return;
            }
            return songDetails(uri);
    }
}

function clickPlaylist(uri) {
    switch(settings.webuiSettings.clickPlaylist) {
        case 'append': return appendQueue('plist', uri);
        case 'appendPlay': return appendPlayQueue('plist', uri);
        case 'insertAfterCurrent': return insertAfterCurrentQueue('plist', uri);
        case 'insertPlayAfterCurrent': return insertPlayAfterCurrentQueue('plist', uri);
        case 'replace': return replaceQueue('plist', uri);
        case 'replacePlay': return replacePlayQueue('plist', uri);
        case 'view': return playlistDetails(uri);
    }
}

function clickFilesystemPlaylist(uri) {
    switch(settings.webuiSettings.clickFilesystemPlaylist) {
        case 'append': return appendQueue('plist', uri);
        case 'appendPlay': return appendPlayQueue('plist', uri);
        case 'insertAfterCurrent': return insertAfterCurrentQueue('plist', uri);
        case 'insertPlayAfterCurrent': return insertPlayAfterCurrentQueue('plist', uri);
        case 'replace': return replaceQueue('plist', uri);
        case 'replacePlay': return replacePlayQueue('plist', uri);
        case 'view':
            //remember offset for current browse uri
            browseFilesystemHistory[app.current.search] = {
                "offset": app.current.offset,
                "scrollPos": document.body.scrollTop ? document.body.scrollTop : document.documentElement.scrollTop
            };
            //reset filter and show playlist
            app.current.filter = '-';
            appGoto('Browse', 'Filesystem', undefined, 0, app.current.limit, app.current.filter, app.current.sort, 'plist', uri);
    }
}

function clickFolder(uri) {
    //remember offset for current browse uri
    browseFilesystemHistory[app.current.search] = {
        "offset": app.current.offset,
        "scrollPos": getScrollPosY()
    };
    //reset filter and open folder
    app.current.filter = '-';
    appGoto('Browse', 'Filesystem', undefined, 0, app.current.limit, app.current.filter, app.current.sort, 'dir', uri);
}

function seekRelativeForward() {
    seekRelative(5);
}

function seekRelativeBackward() {
    seekRelative(-5);
}

function seekRelative(offset) {
    sendAPI("MYMPD_API_PLAYER_SEEK_CURRENT", {
        "seek": offset,
        "relative": true
    });
}

//eslint-disable-next-line no-unused-vars
function clickPlay() {
    switch(currentState.state) {
        case 'play':
            if (settings.webuiSettings.uiFooterPlaybackControls === 'stop' ||
                isStreamUri(currentSongObj.uri) === true)
            {
                //always stop streams
                sendAPI("MYMPD_API_PLAYER_STOP", {});
            }
            else {
                sendAPI("MYMPD_API_PLAYER_PAUSE", {});
            }
            break;
        case 'pause':
            sendAPI("MYMPD_API_PLAYER_RESUME", {});
            break;
        default:
            //fallback if playstate is stop or unknown
            sendAPI("MYMPD_API_PLAYER_PLAY", {});
    }
}

//eslint-disable-next-line no-unused-vars
function clickStop() {
    sendAPI("MYMPD_API_PLAYER_STOP", {});
}

//eslint-disable-next-line no-unused-vars
function clickPrev() {
    sendAPI("MYMPD_API_PLAYER_PREV", {});
}

//eslint-disable-next-line no-unused-vars
function clickNext() {
    sendAPI("MYMPD_API_PLAYER_NEXT", {});
}

//eslint-disable-next-line no-unused-vars
function clickSingle(mode) {
    //mode: 0 = off, 1 = single, 2 = single one shot
    sendAPI("MYMPD_API_PLAYER_OPTIONS_SET", {
        "single": mode
    });
}

//escape and unescape MPD filter values
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

//get and set custom dom properties
//replaces data-* attributes
function setDataId(id, attribute, value) {
    document.getElementById(id)['myMPD-' + attribute] = value;
}

function setData(el, attribute, value) {
    el['myMPD-' + attribute] = value;
}

function getDataId(id, attribute) {
    return getData(document.getElementById(id), attribute);
}

function getData(el, attribute) {
    let value = el['myMPD-' + attribute];
    if (value === undefined) {
        //fallback to attribute
        value = el.getAttribute('data-' + attribute);
        if (value === null) {
            //return undefined if attribute is null
            value = undefined;
        }
    }
    logDebug('getData: "' + attribute + '":"' + value + '"');
    return value;
}

//utility functions
function getSelectValueId(id) {
    return getSelectValue(document.getElementById(id));
}

function getSelectValue(el) {
    if (el && el.selectedIndex >= 0) {
        return el.options[el.selectedIndex].getAttribute('value');
    }
    return undefined;
}

function getSelectedOptionDataId(id, attribute) {
    return getSelectedOptionData(document.getElementById(id), attribute)
}

function getSelectedOptionData(el, attribute) {
    if (el && el.selectedIndex >= 0) {
        return getData(el.options[el.selectedIndex], attribute);
    }
    return undefined;
}

function getRadioBoxValueId(id) {
    return getRadioBoxValue(document.getElementById(id));
}

function getRadioBoxValue(el) {
    const radiobuttons = el.getElementsByClassName('form-check-input');
    for(const button of radiobuttons) {
        if (button.checked === true){
            return button.value;
        }
    }
}

function alignDropdown(el) {
    const toggleEl = el.getElementsByClassName('dropdown-toggle')[0];
    const x = getXpos(toggleEl);
    if (x < domCache.body.offsetWidth * 0.66) {
        el.getElementsByClassName('dropdown-menu')[0].classList.remove('dropdown-menu-end');
    }
    else {
        el.getElementsByClassName('dropdown-menu')[0].classList.add('dropdown-menu-end');
    }
}

function getXpos(el) {
    let xPos = 0;
    while (el) {
        xPos += (el.offsetLeft - el.scrollLeft + el.clientLeft);
        el = el.offsetParent;
    }
    return xPos;
}

function getYpos(el) {
    let yPos = 0;
    while (el) {
        yPos += (el.offsetTop + el.clientTop);
        el = el.offsetParent;
    }
    return yPos;
}

function zeroPad(num, places) {
    const zero = places - num.toString().length + 1;
    return Array(+(zero > 0 && zero)).join("0") + num;
}

function dirname(uri) {
    return uri.replace(/\/[^/]*$/, '');
}

function basename(uri, removeQuery) {
    if (removeQuery === true) {
        return uri.split('/').reverse()[0].split(/[?#]/)[0];
    }
    return uri.split('/').reverse()[0];
}

 function splitFilename(filename) {
     const parts = filename.match(/^(.*)\.([^.]+)$/);
     return {
        "file": parts[1],
        "ext": parts[2]
     };
 }

function isCoverfile(uri) {
    const filename = basename(uri);
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

function isThumbnailfile(uri) {
    const filename = basename(uri);
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

function filetype(uri) {
    if (uri === undefined) {
        return '';
    }
    const ext = uri.split('.').pop().toUpperCase();
    switch(ext) {
        case 'MP3':  return ext + ' - MPEG-1 Audio Layer III';
        case 'FLAC': return ext + ' - Free Lossless Audio Codec';
        case 'OGG':  return ext + ' - Ogg Vorbis';
        case 'OPUS': return ext + ' - Opus Audio';
        case 'WAV':  return ext + ' - WAVE Audio File';
        case 'WV':   return ext + ' - WavPack';
        case 'AAC':  return ext + ' - Advancded Audio Coding';
        case 'MPC':  return ext + ' - Musepack';
        case 'MP4':  return ext + ' - MPEG-4';
        case 'APE':  return ext + ' - Monkey Audio';
        case 'WMA':  return ext + ' - Windows Media Audio';
        case 'CUE':  return ext + ' - Cuesheet';
        default:     return ext;
    }
}

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

function scrollToPosY(container, pos) {
    if (userAgentData.isMobile === true) {
        // For Safari
        document.body.scrollTop = pos;
        // For Chrome, Firefox, IE and Opera
        document.documentElement.scrollTop = pos;
    }
    else {
        container.scrollTop = pos;
    }
}

function selectTag(btnsId, desc, setTo) {
    const btns = document.getElementById(btnsId);
    let aBtn = btns.querySelector('.active');
    if (aBtn) {
        aBtn.classList.remove('active');
    }
    aBtn = btns.querySelector('[data-tag=' + setTo + ']');
    if (aBtn) {
        aBtn.classList.add('active');
        if (desc !== undefined) {
            const descEl = document.getElementById(desc);
            if (descEl !== null) {
                descEl.textContent = aBtn.textContent;
                descEl.setAttribute('data-phrase', aBtn.textContent);
            }
        }
    }
}

function addTagList(elId, list) {
    const stack = elCreateEmpty('div', {"class": ["d-grid", "gap-2"]});
    if (list === 'tagListSearch') {
        if (features.featTags === true) {
            stack.appendChild(elCreateText('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": "any"}, tn('Any Tag')));
        }
        stack.appendChild(elCreateText('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": "filename"}, tn('Filename')));
    }
    if (elId === 'searchDatabaseTags') {
        stack.appendChild(elCreateText('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": "any"}, tn('Any Tag')));
    }
    for (let i = 0, j = settings[list].length; i < j; i++) {
        stack.appendChild(elCreateText('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": settings[list][i]}, tn(settings[list][i])));
    }
    if (elId === 'BrowseNavFilesystemDropdown' ||
        elId === 'BrowseNavPlaylistsDropdown' ||
        elId === 'BrowseNavRadioFavoritesDropdown' ||
        elId === 'BrowseNavWebradiodbDropdown' ||
        elId === 'BrowseNavRadiobrowserDropdown')
    {
        if (features.featTags === true && features.featAdvsearch === true) {
            elClear(stack);
            stack.appendChild(elCreateText('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": "Database"}, tn('Database')));
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
            stack.appendChild(elCreateEmpty('div', {"class": ["dropdown-divider"]}));
        }
        stack.appendChild(elCreateText('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": "Playlists"}, tn('Playlists')));
        if (elId === 'BrowseNavPlaylistsDropdown') {
            stack.lastChild.classList.add('active');
        }
        stack.appendChild(elCreateText('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": "Filesystem"}, tn('Filesystem')));
        if (elId === 'BrowseNavFilesystemDropdown') {
            stack.lastChild.classList.add('active');
        }
        stack.appendChild(elCreateText('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": "Radio"}, tn('Webradios')));
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
            stack.appendChild(elCreateText('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": "Date"}, tn('Date')));
        }
        stack.appendChild(elCreateText('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": "LastModified"}, tn('Last modified')));
    }
    else if (elId === 'searchQueueTags') {
        if (features.featAdvqueue === true)
        {
            stack.appendChild(elCreateText('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": "prio"}, tn('Priority')));
        }
    }
    const el = document.getElementById(elId);
    elReplaceChild(el, stack);
}

function addTagListSelect(elId, list) {
    const select = document.getElementById(elId);
    elClear(select);
    if (elId === 'saveSmartPlaylistSort' || elId === 'selectSmartplsSort') {
        select.appendChild(elCreateText('option', {"value": ""}, tn('Disabled')));
        select.appendChild(elCreateText('option', {"value": "shuffle"}, tn('Shuffle')));
        const optGroup = elCreateEmpty('optgroup', {"label": tn('Sort by tag')});
        optGroup.appendChild(elCreateText('option', {"value": "filename"}, tn('Filename')));
        for (let i = 0, j = settings[list].length; i < j; i++) {
            optGroup.appendChild(elCreateText('option', {"value": settings[list][i]}, tn(settings[list][i])));
        }
        select.appendChild(optGroup);
    }
    else if (elId === 'selectJukeboxUniqueTag' && settings.tagListBrowse.includes('Title') === false) {
        //Title tag should be always in the list
        select.appendChild(elCreateText('option', {"value": "Title"}, tn('Song')));
        for (let i = 0, j = settings[list].length; i < j; i++) {
            select.appendChild(elCreateText('option', {"value": settings[list][i]}, tn(settings[list][i])));
        }
    }
}

//eslint-disable-next-line no-unused-vars
function openModal(modal) {
    uiElements[modal].show();
}

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

function btnWaitingId(id, waiting) {
    btnWaiting(document.getElementById(id), waiting);
}

function btnWaiting(btn, waiting) {
    if (waiting === true) {
        const spinner = elCreateEmpty('span', {"class": ["spinner-border", "spinner-border-sm", "me-2"]});
        btn.insertBefore(spinner, btn.firstChild);
        elDisable(btn);
    }
    else {
        //add a small delay, user should notice the change
        setTimeout(function() {
            elEnable(btn);
            if (btn.firstChild.nodeName === 'SPAN' &&
                btn.firstChild.classList.contains('spinner-border'))
            {
                btn.firstChild.remove();
            }
        }, 100);
    }
}

function toggleBtnGroupValueId(id, value) {
    return toggleBtnGroupValue(document.getElementById(id), value)
}

function toggleBtnGroupValue(btngrp, value) {
    const btns = btngrp.getElementsByTagName('button');
    let b = btns[0];
    let valuestr = value;
    if (isNaN(value) === false) {
        valuestr = value.toString();
    }
    for (let i = 0, j = btns.length; i < j; i++) {
        if (getData(btns[i], 'value') === valuestr) {
            btns[i].classList.add('active');
            b = btns[i];
        }
        else {
            btns[i].classList.remove('active');
        }
    }
    return b;
}

function toggleBtnGroupValueCollapse(btngrp, collapse, value) {
    const activeBtn = toggleBtnGroupValue(btngrp, value);
    if (activeBtn.getAttribute('data-collapse') === 'show') {
        document.getElementById(collapse).classList.add('show');
    }
    else {
        document.getElementById(collapse).classList.remove('show');
    }
}

//eslint-disable-next-line no-unused-vars
function toggleBtnGroupId(id) {
    return toggleBtnGroup(document.getElementById(id));
}

function toggleBtnGroup(btn) {
    const btns = btn.parentNode.getElementsByTagName('button');
    for (let i = 0, j = btns.length; i < j; i++) {
        if (btns[i] === btn) {
            btns[i].classList.add('active');
        }
        else {
            btns[i].classList.remove('active');
        }
    }
    return btn;
}

function getBtnGroupValueId(id) {
    let activeBtn = document.getElementById(id).getElementsByClassName('active');
    if (activeBtn.length === 0) {
        activeBtn = document.getElementById(id).getElementsByTagName('button');
    }
    return getData(activeBtn[0], 'value');
}

//eslint-disable-next-line no-unused-vars
function toggleBtnGroupCollapse(el, collapse) {
    const activeBtn = toggleBtnGroup(el);
    if (activeBtn.getAttribute('data-collapse') === 'show') {
        if (document.getElementById(collapse).classList.contains('show') === false) {
            uiElements[collapse].show();
        }
    }
    else {
        uiElements[collapse].hide();
    }
}

//eslint-disable-next-line no-unused-vars
function toggleBtnId(id, state) {
    toggleBtn(document.getElementById(id), state);
}

function toggleBtn(btn, state) {
    if (state === undefined) {
        //toggle state
        state = btn.classList.contains('active') ? false : true;
    }

    if (state === true ||
        state === 1)
    {
        btn.classList.add('active');
    }
    else {
        btn.classList.remove('active');
    }
}

function toggleBtnChkId(id, state) {
    toggleBtnChk(document.getElementById(id), state);
}

function toggleBtnChk(btn, state) {
    if (state === undefined) {
        //toggle state
        state = btn.classList.contains('active') ? false : true;
    }

    if (state === true ||
        state === 1)
    {
        btn.classList.add('active');
        btn.textContent = 'check';
        return true;
    }
    else {
        btn.classList.remove('active');
        btn.textContent = 'radio_button_unchecked';
        return false;
    }
}

function toggleBtnChkCollapseId(id, collapse, state) {
    toggleBtnChkCollapse(document.getElementById(id), collapse, state);
}

function toggleBtnChkCollapse(btn, collapse, state) {
    const checked = toggleBtnChk(btn, state);
    if (checked === true) {
        document.getElementById(collapse).classList.add('show');
    }
    else {
        document.getElementById(collapse).classList.remove('show');
    }
}

function setPagination(total, returned) {
    const curPaginationTop = document.getElementById(app.id + 'PaginationTop');
    if (curPaginationTop === null) {
        return;
    }

    if (app.current.limit === 0) {
        app.current.limit = 500;
    }

    let totalPages = total < app.current.limit ?
        total === -1 ? -1 : 1 : Math.ceil(total / app.current.limit);
    const curPage = Math.ceil(app.current.offset / app.current.limit) + 1;
    if (app.current.limit > returned) {
        totalPages = curPage;
    }

    //toolbar
    const paginationTop = createPaginationEls(totalPages, curPage);
    paginationTop.classList.add('me-2');
    paginationTop.setAttribute('id', curPaginationTop.id);
    curPaginationTop.replaceWith(paginationTop);

    //bottom
    const bottomBar = document.getElementById(app.id + 'ButtonsBottom');
    elClear(bottomBar);
    if (domCache.body.classList.contains('not-mobile') ||
        returned < 25)
    {
        elHide(bottomBar);
        return;
    }
    const toTop = elCreateText('button', {"class": ["btn", "btn-secondary", "mi"], "title": tn('To top')}, 'keyboard_arrow_up');
    toTop.addEventListener('click', function(event) {
        event.preventDefault();
        scrollToPosY(0);
    }, false);
    bottomBar.appendChild(toTop);
    const paginationBottom = createPaginationEls(totalPages, curPage);
    paginationBottom.classList.add('dropup');
    bottomBar.appendChild(paginationBottom);
    elShow(bottomBar);
}

function createPaginationEls(totalPages, curPage) {
    const prev = elCreateNode('button', {"title": tn('Previous page'), "type": "button", "class": ["btn", "btn-secondary"]},
        elCreateText('span', {"class": ["mi"]}, 'navigate_before'));
    if (curPage === 1) {
        elDisable(prev);
    }
    else {
        prev.addEventListener('click', function(event) {
            event.preventDefault();
            gotoPage('prev');
        }, false);
    }

    const pageDropdownBtn = elCreateText('button', {"type": "button", "data-bs-toggle": "dropdown",
        "class": ["square-end", "btn", "btn-secondary", "dropdown-toggle", "px-2"]}, curPage);
    pageDropdownBtn.addEventListener('show.bs.dropdown', function () {
        alignDropdown(this);
    });
    const pageDropdownMenu = elCreateEmpty('div', {"class": ["dropdown-menu", "bg-lite-dark", "px-2", "page-dropdown", "dropdown-menu-dark"]});

    const row = elCreateNodes('div', {"class": ["row"]}, [
        elCreateText('label', {"class": ["col-sm-8", "col-form-label"]}, tn('Elements per page')),
        elCreateEmpty('div', {"class": ["col-sm-4"]})
    ]);

    const elPerPage = elCreateEmpty('select', {"class": ["form-control", "form-select", "border-secondary"]});
    for (const i in webuiSettingsDefault.uiMaxElementsPerPage.validValues) {
        elPerPage.appendChild(elCreateText('option', {"value": i}, i));
        if (Number(i) === app.current.limit) {
            elPerPage.lastChild.setAttribute('selected', 'selected');
        }
    }
    elPerPage.addEventListener('click', function(event) {
        event.stopPropagation();
    }, false);
    elPerPage.addEventListener('change', function(event) {
        const newLimit = Number(getSelectValue(event.target));
        if (app.current.limit !== newLimit) {
            BSN.Dropdown.getInstance(event.target.parentNode.parentNode.parentNode.previousElementSibling).hide();
            gotoPage(app.current.offset, newLimit);
        }
    }, false);
    row.lastChild.appendChild(elPerPage);

    const pageGrp = elCreateEmpty('div', {"class": ["btn-group"]});

    let start = curPage - 3;
    if (start < 1) {
        start = 1;
    }
    let end = start + 5;
    if (end >= totalPages) {
        end = totalPages - 1;
        start = end - 6 > 1 ? end - 6 : 1;
    }

    const first = elCreateEmpty('button', {"title": tn('First page'), "type": "button", "class": ["btn", "btn-secondary"]});
    if (start === 1) {
        first.textContent = '1';
    }
    else {
        first.appendChild(elCreateText('span', {"class": ["mi"]}, 'first_page'));
    }
    if (curPage === 1) {
        elDisable(first);
        first.classList.add('active');
    }
    else {
        first.addEventListener('click', function(event) {
            event.preventDefault();
            gotoPage(0);
        }, false);
    }
    pageGrp.appendChild(first);

    for (let i = start; i < end; i++) {
        pageGrp.appendChild(elCreateText('button', {"class": ["btn", "btn-secondary"]}, i + 1));
        if (i + 1 === curPage) {
            pageGrp.lastChild.classList.add('active');
        }
        if (totalPages === -1) {
            elDisable(pageGrp.lastChild);
        }
        else {
            pageGrp.lastChild.addEventListener('click', function(event) {
                event.preventDefault();
                gotoPage(i * app.current.limit);
            }, false);
        }
    }

    const last = elCreateEmpty('button', {"title": tn('Last page'), "type": "button", "class": ["btn", "btn-secondary"]});
    if (totalPages === end + 1) {
        last.textContent = end + 1;
    }
    else {
        last.appendChild(elCreateText('span', {"class": ["mi"]}, 'last_page'));
    }
    if (totalPages === -1) {
        elDisable(last);
    }
    else if (totalPages === curPage) {
        if (curPage !== 1) {
            last.classList.add('active');
        }
        elDisable(last);
    }
    else {
        last.addEventListener('click', function(event) {
            event.preventDefault();
            gotoPage(totalPages * app.current.limit - app.current.limit);
        }, false);
    }
    pageGrp.appendChild(last);

    pageDropdownMenu.appendChild(
        elCreateNode('div', {"class": ["row", "mb-3"]}, pageGrp)
    );
    pageDropdownMenu.appendChild(row);

    const next = elCreateEmpty('button', {"title": tn('Next page'), "type": "button", "class": ["btn", "btn-secondary"]});
    next.appendChild(elCreateText('span', {"class": ["mi"]}, 'navigate_next'));
    if (totalPages !== -1 && totalPages === curPage) {
        elDisable(next);
    }
    else {
        next.addEventListener('click', function(event) {
            event.preventDefault();
            gotoPage('next');
        }, false);
    }

    const outer = elCreateNodes('div', {"class": ["btn-group", "pagination"]}, [
        prev,
        elCreateNodes('div', {"class": ["btn-group"]}, [
            pageDropdownBtn,
            pageDropdownMenu
        ]),
        next
    ]);
    new BSN.Dropdown(pageDropdownBtn);
    pageDropdownBtn.parentNode.addEventListener('show.bs.dropdown', function (event) {
        alignDropdown(event.target);
    });
    return outer;
}

function genId(x) {
    return 'id' + x.replace(/[^\w-]/g, '');
}

function parseCmd(event, href) {
    if (event !== null && event !== undefined) {
        event.preventDefault();
    }
    let cmd = href;
    if (typeof(href) === 'string') {
        cmd = JSON.parse(href);
    }

    if (typeof window[cmd.cmd] === 'function') {
        for (let i = 0, j = cmd.options.length; i < j; i++) {
            if (cmd.options[i] === 'event') {
                cmd.options[i] = event;
            }
        }
        switch(cmd.cmd) {
            case 'sendAPI':
                sendAPI(cmd.options[0].cmd, {});
                break;
            case 'createLocalPlaybackEl':
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
                window[cmd.cmd](event.target, ... cmd.options);
                break;
            case 'toggleBtnChkCollapse':
                window[cmd.cmd](event.target, undefined, ... cmd.options);
                break;
            default:
                window[cmd.cmd](... cmd.options);
        }
    }
    else {
        logError('Can not execute cmd: ' + cmd);
    }
}

function gotoPage(x, limit) {
    switch (x) {
        case 'next':
            app.current.offset = app.current.offset + app.current.limit;
            break;
        case 'prev':
            app.current.offset = app.current.offset - app.current.limit;
            if (app.current.offset < 0) {
                app.current.offset = 0;
            }
            break;
        default:
            app.current.offset = x;
    }
    if (limit !== undefined) {
        if (limit === 0) {
            limit = 500;
        }
        app.current.limit = limit;
        if (app.current.offset % app.current.limit > 0) {
            app.current.offset = Math.floor(app.current.offset / app.current.limit);
        }
    }
    appGoto(app.current.card, app.current.tab, app.current.view,
        app.current.offset, app.current.limit, app.current.filter, app.current.sort, app.current.tag, app.current.search, 0);
}

function createSearchCrumbs(searchStr, searchEl, crumbEl) {
    elClear(crumbEl);
    const elements = searchStr.substring(1, app.current.search.length - 1).split(' AND ');
    for (let i = 0, j = elements.length - 1; i < j; i++) {
        const expression = elements[i].substring(1, elements[i].length - 1);
        const fields = expression.match(/^(\w+)\s+(\S+)\s+'(.*)'$/);
        if (fields !== null && fields.length === 4) {
            crumbEl.appendChild(createSearchCrumb(fields[1], fields[2], unescapeMPD(fields[3])));
        }
    }
    if (searchEl.value === '' && elements.length >= 1) {
        const lastEl = elements[elements.length - 1].substring(1, elements[elements.length - 1].length - 1);
        const lastElValue = lastEl.substring(lastEl.indexOf('\'') + 1, lastEl.length - 1);
        if (searchEl.value !== lastElValue) {
            const fields = lastEl.match(/^(\w+)\s+(\S+)\s+'(.*)'$/);
            if (fields !== null && fields.length === 4) {
                crumbEl.appendChild(createSearchCrumb(fields[1], fields[2], unescapeMPD(fields[3])));
            }
        }
    }
    crumbEl.childElementCount > 0 ? elShow(crumbEl) : elHide(crumbEl);
}

function createSearchCrumb(filter, op, value) {
    const btn = elCreateNodes('button', {"class": ["btn", "btn-secondary", "bg-gray-800", "me-2"]}, [
        document.createTextNode(filter + ' ' + op + ' \'' + value + '\''),
        elCreateText('span', {"class": ["ml-2", "badge", "bg-secondary"]}, '×')
    ]);
    setData(btn, 'filter-tag', filter);
    setData(btn, 'filter-op', op);
    setData(btn, 'filter-value', value);
    return btn;
}

function _createSearchExpression(tag, op, value) {
    if (op === 'starts_with' &&
        app.id !== 'BrowseDatabaseList')
    {
        //mpd does not support starts_with, convert it to regex
        op = '=~';
        value = '^' + value;
    }
    return '(' + tag + ' ' + op + ' ' +
        (op === '>=' ? value : '\'' + escapeMPD(value) + '\'') +
        ')';
}

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
        case 'Artist':
        case 'ArtistSort':
        case 'AlbumArtist':
        case 'AlbumArtistSort':
        case 'Composer':
        case 'Performer':
        case 'Conductor':
        case 'Ensemble':
        case 'MUSICBRAINZ_ARTISTID':
        case 'MUSICBRAINZ_ALBUMARTISTID': {
            //multi value tags - print one line per value
            const span = elCreateEmpty('span', {});
            for (let i = 0, j = value.length; i < j; i++) {
                if (i > 0) {
                    span.appendChild(elCreateEmpty('br', {}));
                }
                if (key.indexOf('MUSICBRAINZ') === 0) {
                    span.appendChild(getMBtagLink(key, value[i]));
                }
                else {
                    span.appendChild(document.createTextNode(value[i]));
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
                        "href": myEncodeURIhost(value),
                        "target": "_blank"}, value);
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

function getTimestamp() {
    return Math.floor(Date.now() / 1000);
}

function setScrollViewHeight(container) {
    if (userAgentData.isMobile === true) {
        container.parentNode.style.maxHeight = '';
        return;
    }
    const footerHeight = document.getElementsByTagName('footer')[0].offsetHeight;
    const tpos = getYpos(container.parentNode);
    const maxHeight = window.innerHeight - tpos - footerHeight;
    container.parentNode.style.maxHeight = maxHeight + 'px';
}

function toggleCollapseArrow(el) {
    const icon = el.getElementsByTagName('span')[0];
    icon.textContent = icon.textContent === 'keyboard_arrow_right' ? 'keyboard_arrow_down' : 'keyboard_arrow_right';
}

function ucFirst(string) {
    return string[0].toUpperCase() + string.slice(1);
}

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

function setViewport() {
    document.querySelector("meta[name=viewport]").setAttribute('content', 'width=device-width, initial-scale=' +
        localSettings.scaleRatio + ', maximum-scale=' + localSettings.scaleRatio);
}

//eslint-disable-next-line no-unused-vars
function clearCovercache() {
    sendAPI("MYMPD_API_COVERCACHE_CLEAR", {});
}

//eslint-disable-next-line no-unused-vars
function cropCovercache() {
    sendAPI("MYMPD_API_COVERCACHE_CROP", {});
}

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

//eslint-disable-next-line no-unused-vars
function zoomZoomPicture() {
    window.open(document.getElementById('modalPictureImg').style.backgroundImage.match(/^url\(["']?([^"']*)["']?\)/)[1]);
}

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

function _createImgCarousel(imgEl, name, images) {
    const nrImages = images.length;
    const carousel = elCreateEmpty('div', {"id": name, "class": ["carousel", "slide"], "data-bs-ride": "carousel"});
    if (nrImages > 1) {
        const carouselIndicators = elCreateEmpty('div', {"class": ["carousel-indicators"]});
        for (let i = 0; i < nrImages; i++) {
            carouselIndicators.appendChild(elCreateEmpty('button', {"type": "button", "data-bs-target": "#" + name, "data-bs-slide-to": i}));
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

//eslint-disable-next-line no-unused-vars
function showModal(modal) {
    uiElements[modal].show();
}

function checkMediaSessionSupport() {
    if (settings.mediaSession === false ||
        navigator.mediaSession === undefined)
    {
        return false;
    }
    return true;
}

function checkTagValue(tag, value) {
    if (typeof tag === 'string') {
        return tag === value;
    }
    return tag[0] === value;
}

function strToBool(str) {
    return str === 'true';
}

function getParent(el, nodeName) {
    let target = el;
    let i = 0;
    while (target.nodeName !== nodeName) {
        i++;
        if (i > 10) {
            return null;
        }
        target = target.parentNode;
    }
    return target;
}

function clearSearchTimer() {
    if (searchTimer !== null) {
        clearTimeout(searchTimer);
        searchTimer = null;
    }
}

//cuesheet handling
function cuesheetUri(uri) {
    const cuesheet = uri.match(/^(.*\.cue)\/(track\d+)$/);
    if (cuesheet !== null) {
        return cuesheet[1];
    }
    return uri;
}

function cuesheetTrack(uri) {
    const cuesheet = uri.match(/^(.*\.cue)\/(track\d+)$/);
    if (cuesheet !== null) {
        return cuesheet[2];
    }
    return '';
}

function setMobileView() {
    if (userAgentData.isMobile === true) {
        setViewport();
        domCache.body.classList.remove('not-mobile');
    }
    else {
        domCache.body.classList.add('not-mobile');
    }
}
