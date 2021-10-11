"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

//element handling shortcuts
function elCreate(tagName, attributes, textContent) {
    const tag = document.createElement(tagName);
    for (const key in attributes) {
        switch(key) {
            case "class":
                tag.classList.add(...attributes[key]);
                break;
            default:
                tag.setAttribute(key, attributes[key]);
        }
    }
    tag.textContent = textContent;
    return tag;
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
}

function elEnableId(el) {
    document.getElementById(el).removeAttribute('disabled');
}

function elEnable(el) {
    el.removeAttribute('disabled');
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

//escapes html characters to avoid xss
function e(x) {
    if (isNaN(x)) {
        return x.replace(/([<>"'])/g, function(m0, m1) {
            if (m1 === '<') return '&lt;';
            else if (m1 === '>') return '&gt;';
            else if (m1 === '"') return '&quot;';
            else if (m1 === '\'') return '&apos;';
        }).replace(/\\u(003C|003E|0022|0027)/gi, function(m0, m1) {
            if (m1 === '003C') return '&lt;';
            else if (m1 === '003E') return '&gt;';
            else if (m1 === '0022') return '&quot;';
            else if (m1 === '0027') return '&apos;';
        }).replace(/\[\[(\w+)\]\]/g, function(m0, m1) {
            return '<span class="mi">' + m1 + '</span>';
        });
    }
    return x;
}

//removes special characters
function r(x) {
    return x.replace(/[^\w-]/g, '_');
}

//confirmation dialogs
function showConfirm(text, btnText, callback) {
    document.getElementById('modalConfirmText').textContent = text;
    const yesBtn = elCreate('button', {"id": "modalConfirmYesBtn", "class": ["btn", "btn-danger"]}, btnText);
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
    const confirm = elCreate('div', {"class": ["alert", "alert-danger", "mt-2"]}, '');
    const p = elCreate('p', {}, text);
    confirm.appendChild(p);

    const cancelBtn = elCreate('button', {"class": ["btn", "btn-secondary"]}, t('Cancel'));
    cancelBtn.addEventListener('click', function() {
        this.parentNode.remove();
    }, false);
    confirm.appendChild(cancelBtn);

    const yesBtn = elCreate('button', {"class": ["btn", "btn-danger", "float-end"]}, btnText);
    yesBtn.addEventListener('click', function() {
        if (callback !== undefined && typeof(callback) === 'function') {
            callback();
        }
        this.parentNode.remove();
    }, false);
    confirm.appendChild(yesBtn);
    el.appendChild(confirm);
}

function myEncodeURI(x) {
    return encodeURI(x).replace(/([#])/g, function(m0, m1) {
            if (m1 === '#') return '%23';
    });
}

//eslint-disable-next-line no-unused-vars
function myDecodeURI(x) {
    return decodeURI(x).replace(/(%23)/g, function(m0, m1) {
            if (m1 === '%23') return '#';
    });
}

//functions to get custom actions
function clickAlbumPlay(albumArtist, album) {
    switch (settings.webuiSettings.clickAlbumPlay) {
        case 'append': return _addAlbum('appendQueue', albumArtist, album);
        case 'replace': return _addAlbum('replaceQueue', albumArtist, album);
    }
}

function clickSong(uri, name) {
    switch (settings.webuiSettings.clickSong) {
        case 'append': return appendQueue('song', uri, name);
        case 'replace': return replaceQueue('song', uri, name);
        case 'view': return songDetails(uri);
    }
}

function clickQueueSong(songid, uri) {
    switch (settings.webuiSettings.clickQueueSong) {
        case 'play':
            if (songid === null) {
                return;
            }
            sendAPI("MYMPD_API_PLAYER_PLAY_SONG", {"songId": songid});
            break;
        case 'view': 
            if (uri === null) {
                return;
            }
            return songDetails(uri);
    }
}

function clickPlaylist(uri, name) {
    switch (settings.webuiSettings.clickPlaylist) {
        case 'append': return appendQueue('plist', uri, name);
        case 'replace': return replaceQueue('plist', uri, name);
        case 'view': return playlistDetails(uri);
    }
}

function clickFolder(uri, name) {
    switch (settings.webuiSettings.clickFolder) {
        case 'append': return appendQueue('dir', uri, name);
        case 'replace': return replaceQueue('dir', uri, name);
        case 'view':
            //remember offset for current browse uri
            browseFilesystemHistory[app.current.search] = {
                "offset":  app.current.offset,
                "scrollPos": document.body.scrollTop ? document.body.scrollTop : document.documentElement.scrollTop
            };
            //reset filter and open folder
            app.current.filter = '-';
            appGoto('Browse', 'Filesystem', undefined, '0', app.current.limit, app.current.filter, app.current.sort, '-', uri);
            break;
    }
}

//escape and unescape MPD filter values
function escapeMPD(x) {
    return x.replace(/(["'])/g, function(m0, m1) {
        if (m1 === '"') return '\\"';
        else if (m1 === '\'') return '\\\'';
        else if (m1 === '\\') return '\\\\';
    });
}

function unescapeMPD(x) {
    return x.replace(/(\\'|\\"|\\\\)/g, function(m0, m1) {
        if (m1 === '\\"') return '"';
        else if (m1 === '\\\'') return '\'';
        else if (m1 === '\\\\') return '\\';
    });
}

//get and set custom dom properties
//replaces data-* attributes
function setCustomDomPropertyId(id, attribute, value) {
    document.getElementById(id)['myMPD-' + attribute] = value;
}

function setCustomDomProperty(el, attribute, value) {
    el['myMPD-' + attribute] = value;
}

function getCustomDomPropertyId(id, attribute) {
    return getCustomDomProperty(document.getElementById(id), attribute);
}

function getCustomDomProperty(el, attribute) {
    let value = el['myMPD-' + attribute];
    if (value === undefined) {
        //fallback to attribute
        const encValue = el.getAttribute(attribute);
        value = encValue !== null ? decodeURI(encValue) : null;
    }
    return value;
}

//utility functions
function getSelectValueId(id) {
    return getSelectValue(document.getElementById(id));
}

function getSelectValue(el) {
    if (el && el.selectedIndex >= 0) {
        return getCustomDomProperty(el.options[el.selectedIndex], 'value');
    }
    return undefined;
}

function getSelectedOptionAttribute(selectId, attribute) {
    const el = document.getElementById(selectId);
    if (el && el.selectedIndex >= 0) {
        return getCustomDomProperty(el.options[el.selectedIndex], attribute);
    }
    return undefined;
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
    else {
        return uri.split('/').reverse()[0];
    }
}

function filetype(uri) {
    if (uri === undefined) {
        return '';
    }
    const ext = uri.split('.').pop().toUpperCase();
    switch (ext) {
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
        default:     return ext;
    }
}

function fileformat(audioformat) {
    return audioformat.bits + t('bits') + ' - ' + audioformat.sampleRate / 1000 + t('kHz');
}

function scrollToPosY(pos) {
    // For Safari
    document.body.scrollTop = pos;
    // For Chrome, Firefox, IE and Opera
    document.documentElement.scrollTop = pos;
}

function selectTag(btnsEl, desc, setTo) {
    const btns = document.getElementById(btnsEl);
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
    const stack = elCreate('div', {"class": ["d-grid", "gap-2"]}, '');
    if (list === 'tagListSearch') {
        if (features.featTags === true) {
            stack.appendChild(elCreate('button', {"class": ["btn", "btn-secondary", "btn-sm", "btn-block"], "data-tag": "any"}, tn('Any Tag')));
        }
        stack.appendChild(elCreate('button', {"class": ["btn", "btn-secondary", "btn-sm", "btn-block"], "data-tag": "filename"}, tn('Filename')));
    }
    if (elId === 'searchDatabaseTags') {
        stack.appendChild(elCreate('button', {"class": ["btn", "btn-secondary", "btn-sm", "btn-block"], "data-tag": "any"}, tn('Any Tag')));
    }
    for (let i = 0, j = settings[list].length; i < j; i++) {
        stack.appendChild(elCreate('button', {"class": ["btn", "btn-secondary", "btn-sm", "btn-block"], "data-tag": settings[list][i]}, tn(settings[list][i])));
    }
    if (elId === 'BrowseNavFilesystemDropdown' || elId === 'BrowseNavPlaylistsDropdown') {
        if (features.featTags === true && features.featAdvsearch === true) {
            elClear(stack);
            stack.appendChild(elCreate('button', {"class": ["btn", "btn-secondary", "btn-sm", "btn-block"], "data-tag": "Database"}, tn('Database')));
        }
    }
    if (elId === 'BrowseDatabaseByTagDropdown' || elId === 'BrowseNavFilesystemDropdown' || elId === 'BrowseNavPlaylistsDropdown') {
        if (elId === 'BrowseDatabaseByTagDropdown') {
            stack.appendChild(elCreate('div', {"class": ["dropdown-divider"]}, ''));
        }
        stack.appendChild(elCreate('button', {"class": ["btn", "btn-secondary", "btn-sm", "btn-block"], "data-tag": "Playlists"}, tn('Playlists')));
        if (elId === 'BrowseNavPlaylistsDropdown') {
            stack.lastChild.classList.add('active');
        }
        stack.appendChild(elCreate('button', {"class": ["btn", "btn-secondary", "btn-sm", "btn-block"], "data-tag": "Filesystem"}, tn('Filesystem')));
        if (elId === 'BrowseNavFilesystemDropdown') {
            stack.lastChild.classList.add('active');
        }
    }
    else if (elId === 'databaseSortTagsList') {
        if (settings.tagList.includes('Date') === true && settings[list].includes('Date') === false) {
            stack.appendChild(elCreate('button', {"class": ["btn", "btn-secondary", "btn-sm", "btn-block"], "data-tag": "Date"}, tn('Date')));
        }
        stack.appendChild(elCreate('button', {"class": ["btn", "btn-secondary", "btn-sm", "btn-block"], "data-tag": "Last-Modified"}, tn('Last modified')));
    }
    const el = document.getElementById(elId);
    elClear(el);
    el.appendChild(stack);
}

function addTagListSelect(elId, list) {
    let tagList = '';
    if (elId === 'saveSmartPlaylistSort' || elId === 'selectSmartplsSort') {
        tagList += '<option value="">' + t('Disabled') + '</option>';
        tagList += '<option value="shuffle">' + t('Shuffle') + '</option>';
        tagList += '<optgroup label="' + t('Sort by tag') + '">';
        tagList += '<option value="filename">' + t('Filename') + '</option>';
    }
    else if (elId === 'selectJukeboxUniqueTag' && settings.tagListBrowse.includes('Title') === false) {
        //Title tag should be always in the list
        tagList = '<option value="Title">' + t('Song') + '</option>';
    }
    for (let i = 0, j = settings[list].length; i < j; i++) {
        tagList += '<option value="' + settings[list][i] + '">' + t(settings[list][i]) + '</option>';
    }
    if (elId === 'saveSmartPlaylistSort' || elId === 'selectSmartplsSort') {
        tagList += '</optgroup>';
    }
    document.getElementById(elId).innerHTML = tagList;
}

//eslint-disable-next-line no-unused-vars
function openModal(modal) {
    uiElements[modal].show();
}

//eslint-disable-next-line no-unused-vars
function openDropdown(dropdown) {
    uiElements[dropdown].toggle();
}

//eslint-disable-next-line no-unused-vars
function focusSearch() {
    if (app.current.app === 'Queue' && app.current.tab === 'Current') {
        document.getElementById('searchqueuestr').focus();
    }
    else if (app.current.app === 'Browse' && app.current.tab === 'Database' && app.current.view === 'List') {
        document.getElementById('searchDatabaseStr').focus();
    }
    else if (app.current.app === 'Browse' && app.current.tab === 'Filesystem') {
        document.getElementById('searchFilesystemStr').focus();
    }
    else if (app.current.app === 'Browse' && app.current.tab === 'Playlists' && app.current.view === 'List') {
        document.getElementById('searchPlaylistsListStr').focus();
    }
    else if (app.current.app === 'Browse' && app.current.tab === 'Playlists' && app.current.view === 'Detail') {
        document.getElementById('searchPlaylistsDetailStr').focus();
    }
    else if (app.current.app === 'Search') {
        document.getElementById('searchstr').focus();
    }
    else {
        appGoto('Search');
    }
}

function btnWaiting(btn, waiting) {
    if (waiting === true) {
        const spinner = document.createElement('span');
        spinner.classList.add('spinner-border', 'spinner-border-sm', 'mr-2');
        btn.insertBefore(spinner, btn.firstChild);
        elDisable(btn);
    }
    else {
        elEnable(btn);
        if (btn.firstChild.nodeName === 'SPAN') {
            btn.firstChild.remove();
        }
    }
}

function toggleBtnGroupValue(btngrp, value) {
    const btns = btngrp.getElementsByTagName('button');
    let b = btns[0];
    let valuestr = value;
    if (isNaN(value) === false) {
        valuestr = value.toString();
    }
    for (let i = 0, j = btns.length; i < j; i++) {
        if (getCustomDomProperty(btns[i], 'data-value') === valuestr) {
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
    return getCustomDomProperty(activeBtn[0], 'data-value');
}

//eslint-disable-next-line no-unused-vars
function toggleBtnGroupCollapse(id, collapse) {
    const activeBtn = toggleBtnGroupId(id);
    if (activeBtn.getAttribute('data-collapse') === 'show') {
        if (document.getElementById(collapse).classList.contains('show') === false) {
            uiElements[collapse].show();
        }
    }
    else {
        uiElements[collapse].hide();
    }
}

function toggleBtnId(id, state) {
    toggleBtn(document.getElementById(id), state);
}

function toggleBtn(btn, state) {
    if (state === undefined) {
        //toggle state
        state = btn.classList.contains('active') ? false : true;
    }

    if (state === true || state === 1) {
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

    if (state === true || state === 1) {
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
        total === -1 ? -1 : 1 :  Math.ceil(total / app.current.limit);
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
    if (returned < 25) {
        elHide(bottomBar);
        return;
    }
    elClear(bottomBar);
    const toTop = elCreate('button', {"class": ["btn", "btn-secondary", "mi"], "title": tn('To top')}, 'keyboard_arrow_up');
    toTop.addEventListener('click', function() {
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
    const prev = elCreate('button', {"title": tn('Previous page'), "type": "button", "class": ["btn", "btn-secondary"]}, '');
    prev.appendChild(elCreate('span', {"class": ["mi"]}, 'navigate_before'));
    if (curPage === 1) {
        elDisable(prev);
    }
    else {
        prev.addEventListener('click', function(event) {
            event.preventDefault();
            gotoPage('prev');
        }, false);
    }
    
    const pageDropdownBtn = elCreate('button', {"type": "button", "data-bs-toggle": "dropdown", "class": ["square-end", "btn", "btn-secondary", "dropdown-toggle", "px-2"]}, curPage);
    const pageDropdownMenu = elCreate('div', {"class": ["dropdown-menu", "bg-lite-dark", "px-2", "pages", "dropdown-menu-right"]}, '');
    
    const row = elCreate('div', {"class": ["row"]}, '');
    row.appendChild(elCreate('label', {"class": ["col-sm-8", "col-form-label"]}, tn('Elements per page')));
    row.appendChild(elCreate('div', {"class": ["col-sm-4"]}, ''));
    const elPerPage = elCreate('select', {"class": ["form-control", "form-select", "border-secondary"]}, '');
    for (const i in webuiSettingsDefault.uiMaxElementsPerPage.validValues) {
        elPerPage.appendChild(elCreate('option', {"value": i}, i));
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
            event.target.parentNode.parentNode.parentNode.previousElementSibling.Dropdown.hide();
            gotoPage(app.current.offset, newLimit);
        }
    }, false);
    row.lastChild.appendChild(elPerPage);
    
    const pageList = elCreate('div', {"class": ["row", "mb-3"]}, '');
    const pageGrp = elCreate('div', {"class": ["btn-group"]}, '');
    
    const first = elCreate('button', {"title": tn('First page'), "type": "button", "class": ["btn", "btn-secondary"]}, '');
    first.appendChild(elCreate('span', {"class": ["mi"]}, 'first_page'));
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
    
    let start = curPage - 3;
    if (start < 1) {
        start = 1;
    }
    let end = start + 5;
    if (end >= totalPages) {
        end = totalPages - 1;
        start = end - 6 > 1 ? end - 6 : 1;
    }

    for (let i = start; i < end; i++) {
        pageGrp.appendChild(elCreate('button', {"class": ["btn", "btn-secondary"]}, i + 1));
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
    
    const last = elCreate('button', {"title": tn('Last page'), "type": "button", "class": ["btn", "btn-secondary"]}, '');
    last.appendChild(elCreate('span', {"class": ["mi"]}, 'last_page'));
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
    pageList.appendChild(pageGrp);
    
    pageDropdownMenu.appendChild(pageList);
    pageDropdownMenu.appendChild(row);
    
    const next = elCreate('button', {"title": tn('Next page'), "type": "button", "class": ["btn", "btn-secondary"]}, '');
    next.appendChild(elCreate('span', {"class": ["mi"]}, 'navigate_next'));
    if (totalPages !== -1 && totalPages === curPage) {
        elDisable(next);
    }
    else {
        next.addEventListener('click', function(event) {
            event.preventDefault();
            gotoPage('next');
        }, false);
    }

    const outer = elCreate('div', {"class": ["btn-group", "pagination"]}, '');
    outer.appendChild(prev);
    const btnGrp = elCreate('div', {"class": ["btn-group"]}, '');
    btnGrp.appendChild(pageDropdownBtn);
    btnGrp.appendChild(pageDropdownMenu);
    outer.appendChild(btnGrp);
    outer.appendChild(next);
    new BSN.Dropdown(pageDropdownBtn);
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
            case 'toggleBtn':
            case 'toggleBtnChk':
            case 'toggleBtnGroup':
            case 'toggleBtnGroupCollapse':
            case 'zoomPicture':
            case 'setPlaySettings':
            case 'resetToDefault':
            case 'voteSong':
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
    appGoto(app.current.app, app.current.tab, app.current.view, 
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
    const btn = elCreate('button', {"class": ["btn", "btn-light", "mr-2"]}, filter + ' ' + op + ' \'' + value + '\'');
    setCustomDomProperty(btn, 'data-filter-tag', filter);
    setCustomDomProperty(btn, 'data-filter-op', op);
    setCustomDomProperty(btn, 'data-filter-value', value);
    const badge = elCreate('span', {"class": ["ml-2", "badge", "bg-secondary"]}, '×');
    btn.appendChild(badge);
    return btn;
}

function createSearchExpression(crumbsEl, tag, op, value) {
    let expression = '(';
    const crumbs = crumbsEl.children;
    for (let i = 0, j = crumbs.length; i < j; i++) {
        if (i > 0) {
            expression += ' AND ';
        }
        let crumbOp = getCustomDomProperty(crumbs[i], 'data-filter-op');
        let crumbValue = getCustomDomProperty(crumbs[i], 'data-filter-value');
        if (app.current.app === 'Search' && crumbOp === 'starts_with') {
            crumbOp = '=~';
            crumbValue = '^' + crumbValue;
        }
        expression += '(' + getCustomDomProperty(crumbs[i], 'data-filter-tag') + ' ' + 
            crumbOp + ' \'' + escapeMPD(crumbValue) + '\')';
    }
    if (value !== '') {
        if (expression.length > 1) {
            expression += ' AND ';
        }
        if (app.current.app === 'Search' && op === 'starts_with') {
            //mpd does not support starts_with, convert it to regex
            op = '=~';
            value = '^' + value;
        }
        expression += '(' + tag + ' ' + op + ' \'' + escapeMPD(value) +'\')';
    }
    expression += ')';
    if (expression.length <= 2) {
        expression = '';
    }
    return expression;
}

function printValue(key, value) {
    if (value === undefined || value === null) {
        return '-';
    }
    switch (key) {
        case 'Type':
            if (value === 'song') { return '<span class="mi">music_note</span>'; }
            if (value === 'smartpls') { return '<span class="mi">queue_music</span>'; }
            if (value === 'plist') { return '<span class="mi">list</span>'; }
            if (value === 'dir') { return '<span class="mi">folder_open</span>'; }
            return '<span class="mi">radio_button_unchecked</span>';
        case 'Duration':
            return beautifySongDuration(value);
        case 'LastModified': 
        case 'LastPlayed':
        case 'stickerLastPlayed':
        case 'stickerLastSkipped':
            return value === 0 ? t('never') : localeDate(value);
        case 'stickerLike':
            return '<span class="mi mi-small">'+
                (value === 0 ? 'thumb_down' : value === 1 ? 'radio_button_unchecked' : 'thumb_up') +
                '</span>';
        default:
            if (key.indexOf('MUSICBRAINZ') === 0) {
                return getMBtagLink(key, value);
            }
            return e(value);
    }
}

function addIconLine(el, ligature, text) {
    const icon = elCreate('span', {"class": ["mi", "me-2"]}, ligature);
    const span = elCreate('span', {}, text);
    el.appendChild(icon);
    el.appendChild(span);
}

function getTimestamp() {
    return Math.floor(Date.now() / 1000);
}
