"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

function removeIsInvalid(el) {
    let els = el.querySelectorAll('.is-invalid');
    for (let i = 0; i < els.length; i++) {
        els[i].classList.remove('is-invalid');
    }
}

function getSelectValue(selectId) {
    let el = document.getElementById(selectId);
    return el.options[el.selectedIndex].value;
}

function alignDropdown(el) {
    const x = getXpos(el.children[0]);
    
    if (x < domCache.body.offsetWidth * 0.66) {
        if (el.id === 'navState') {
            el.classList.remove('dropdown');
            el.classList.add('dropright');
        }
        else {
            el.getElementsByClassName('dropdown-menu')[0].classList.remove('dropdown-menu-right');
        }
    }
    else {
        el.getElementsByClassName('dropdown-menu')[0].classList.add('dropdown-menu-right');
        el.classList.add('dropdown');
        el.classList.remove('dropright');
    }
}

function getXpos(el) {
    var xPos = 0;
    while (el) {
        xPos += (el.offsetLeft - el.scrollLeft + el.clientLeft);
        el = el.offsetParent;
    }
    return xPos;
}

function zeroPad(num, places) {
  var zero = places - num.toString().length + 1;
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
    let ext = uri.split('.').pop().toUpperCase();
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
        case 'APE':  return ext + ' - Monkey Audio ';
        case 'WMA':  return ext + ' - Windows Media Audio';
        default:     return ext;
    }
}

function fileformat(audioformat) {
    return audioformat.bits + t('bits') + ' - ' + audioformat.sampleRate / 1000 + t('kHz');
}

function scrollToPosY(pos) {
    document.body.scrollTop = pos; // For Safari
    document.documentElement.scrollTop = pos; // For Chrome, Firefox, IE and Opera
}

function doSetFilterLetter(x) {
    let af = document.getElementById(x + 'Letters').getElementsByClassName('active')[0];
    if (af) {
        af.classList.remove('active');
    }
    let filter = app.current.filter;
    if (filter === '0') {
        filter = '#';
    }
    
    document.getElementById(x).innerHTML = '<span class="material-icons">filter_list</span>' + (filter !== '-' ? ' ' + filter : '');
    
    if (filter !== '-') {
        let btns = document.getElementById(x + 'Letters').getElementsByTagName('button');
        let btnsLen = btns.length;
        for (let i = 0; i < btnsLen; i++) {
            if (btns[i].innerText === filter) {
                btns[i].classList.add('active');
                break;
            }
        }
    }
}

function addFilterLetter(x) {
    let filter = '<button class="mr-1 mb-1 btn btn-sm btn-secondary material-icons material-icons-small">delete</button>' +
        '<button class="mr-1 mb-1 btn btn-sm btn-secondary">#</button>';
    for (let i = 65; i <= 90; i++) {
        filter += '<button class="mr-1 mb-1 btn-sm btn btn-secondary">' + String.fromCharCode(i) + '</button>';
    }

    let letters = document.getElementById(x);
    letters.innerHTML = filter;
    
    letters.addEventListener('click', function(event) {
        switch (event.target.innerText) {
            case 'delete':
                filter = '-';
                break;
            case '#':
                filter = '0';
                break;
            default:
                filter = event.target.innerText;
        }
        appGoto(app.current.app, app.current.tab, app.current.view, '0/' + filter + '/' + app.current.sort + '/' + 
            app.current.tag + '/' + app.current.search);
    }, false);
}

function selectTag(btnsEl, desc, setTo) {
    let btns = document.getElementById(btnsEl);
    let aBtn = btns.querySelector('.active')
    if (aBtn) {
        aBtn.classList.remove('active');
    }
    aBtn = btns.querySelector('[data-tag=' + setTo + ']');
    if (aBtn) {
        aBtn.classList.add('active');
        if (desc !== undefined) {
            document.getElementById(desc).innerText = aBtn.innerText;
            document.getElementById(desc).setAttribute('data-phrase', aBtn.innerText);
        }
    }
}

function addTagList(el, list) {
    let tagList = '';
    if (list === 'searchtags') {
        if (settings.featTags === true) {
            tagList += '<button type="button" class="btn btn-secondary btn-sm btn-block" data-tag="any">' + t('Any Tag') + '</button>';
        }
        tagList += '<button type="button" class="btn btn-secondary btn-sm btn-block" data-tag="filename">' + t('Filename') + '</button>';
    }
    for (let i = 0; i < settings[list].length; i++) {
        tagList += '<button type="button" class="btn btn-secondary btn-sm btn-block" data-tag="' + settings[list][i] + '">' + t(settings[list][i]) + '</button>';
    }
    if (el === 'BrowseNavFilesystemDropdown' || el === 'BrowseNavPlaylistsDropdown') {
        tagList = '<button type="button" class="btn btn-secondary btn-sm btn-block" data-tag="Database">Database</button>';
    }
    if (el === 'BrowseDatabaseByTagDropdown' || el === 'BrowseNavFilesystemDropdown' || el === 'BrowseNavPlaylistsDropdown') {
        if (el === 'BrowseDatabaseByTagDropdown') {
            tagList += '<div class="dropdown-divider"></div>';
        }
        tagList += '<button type="button" class="btn btn-secondary btn-sm btn-block' + (el === 'BrowseNavPlaylistsDropdown' ? ' active' : '') + '" data-tag="Playlists">' + t('Playlists') + '</button>' +
            '<button type="button" class="btn btn-secondary btn-sm btn-block' + (el === 'BrowseNavFilesystemDropdown' ? ' active' : '') + '" data-tag="Filesystem">' + t('Filesystem') + '</button>'
    }
    else if (el === 'databaseSortTagsList') {
        if (settings.tags.includes('Date')) {
            tagList += '<button type="button" class="btn btn-secondary btn-sm btn-block" data-tag="Date">' + t('Date') + '</button>';
        }
        tagList += '<button type="button" class="btn btn-secondary btn-sm btn-block" data-tag="Last-Modified">' + t('Last modified') + '</button>';
    }
    document.getElementById(el).innerHTML = tagList;
}

function addTagListSelect(el, list) {
    let tagList = '';
    if (el === 'saveSmartPlaylistSort' || el === 'selectSmartplsSort') {
        tagList += '<option value="">' + t('Disabled') + '</option>';
        tagList += '<option value="shuffle">' + t('Shuffle') + '</option>';
        tagList += '<optgroup label="' + t('Sort by tag') + '">';
        tagList += '<option value="filename">' + t('Filename') + '</option>';
    }
    else if (el === 'selectJukeboxUniqueTag' && settings.browsetags.includes('Title') === false) {
        //Title tag should be always in the list
        tagList = '<option value="Title">' + t('Song') + '</option>';
    }
    for (let i = 0; i < settings[list].length; i++) {
        tagList += '<option value="' + settings[list][i] + '">' + t(settings[list][i]) + '</option>';
    }
    if (el === 'saveSmartPlaylistSort' || el === 'selectSmartplsSort') {
        tagList += '</optgroup>';
    }
    document.getElementById(el).innerHTML = tagList;
}

//eslint-disable-next-line no-unused-vars
function openModal(modal) {
    window[modal].show();
}

//eslint-disable-next-line no-unused-vars
function openDropdown(dropdown) {
    window[dropdown].toggle();
}

//eslint-disable-next-line no-unused-vars
function focusSearch() {
    if (app.current.app === 'Queue') {
        document.getElementById('searchqueuestr').focus();
    }
    else if (app.current.app === 'Search') {
        domCache.searchstr.focus();
    }
    else {
        appGoto('Search');
    }
}

function btnWaiting(btn, waiting) {
    if (waiting === true) {
        let spinner = document.createElement('span');
        spinner.classList.add('spinner-border', 'spinner-border-sm', 'mr-2');
        btn.insertBefore(spinner, btn.firstChild);
        btn.setAttribute('disabled', 'disabled');
    }
    else {
        btn.removeAttribute('disabled');
        if (btn.firstChild.nodeName === 'SPAN') {
            btn.firstChild.remove();
        }
    }
}

function toggleBtnGroupValue(btngrp, value) {
    let btns = btngrp.getElementsByTagName('button');
    let b = btns[0];
    let valuestr = value;
    if (isNaN(value) === false) {
        valuestr = value.toString();
    }
    for (let i = 0; i < btns.length; i++) {
        if (btns[i].getAttribute('data-value') === valuestr) {
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
    let activeBtn = toggleBtnGroupValue(btngrp, value);
    if (activeBtn.getAttribute('data-collapse') === 'show') {
        document.getElementById(collapse).classList.add('show');
    }
    else {
        document.getElementById(collapse).classList.remove('show');
    }
}

function toggleBtnGroup(btn) {
    let b = btn;
    if (typeof btn === 'string') {
        b = document.getElementById(btn);
    }
    let btns = b.parentNode.getElementsByTagName('button');
    for (let i = 0; i < btns.length; i++) {
        if (btns[i] === b) {
            btns[i].classList.add('active');
        }
        else {
            btns[i].classList.remove('active');
        }
    }
    return b;
}

function getBtnGroupValue(btnGroup) {
    let activeBtn = document.getElementById(btnGroup).getElementsByClassName('active');
    if (activeBtn.length === 0) {
        activeBtn = document.getElementById(btnGroup).getElementsByTagName('button');    
    }
    return activeBtn[0].getAttribute('data-value');
}

//eslint-disable-next-line no-unused-vars
function toggleBtnGroupCollapse(btn, collapse) {
    let activeBtn = toggleBtnGroup(btn);
    if (activeBtn.getAttribute('data-collapse') === 'show') {
        if (document.getElementById(collapse).classList.contains('show') === false) {
            window[collapse].show();
        }
    }
    else {
        window[collapse].hide();
    }
}

function toggleBtn(btn, state) {
    let b = btn;
    if (typeof btn === 'string') {
        b = document.getElementById(btn);
    }
    if (!b) {
        return;
    }
    if (state === undefined) {
        //toggle state
        state = b.classList.contains('active') ? false : true;
    }

    if (state === true || state === 1) {
        b.classList.add('active');
    }
    else {
        b.classList.remove('active');
    }
}

function toggleBtnChk(btn, state) {
    let b = btn;
    if (typeof btn === 'string') {
        b = document.getElementById(btn);
    }
    if (!b) {
        return;
    }
    if (state === undefined) {
        //toggle state
        state = b.classList.contains('active') ? false : true;
    }

    if (state === true || state === 1) {
        b.classList.add('active');
        b.innerText = 'check';
        return true;
    }
    else {
        b.classList.remove('active');
        b.innerText = 'radio_button_unchecked';
        return false;
    }
}

function toggleBtnChkCollapse(btn, collapse, state) {
    let checked = toggleBtnChk(btn, state);
    if (checked === true) {
        document.getElementById(collapse).classList.add('show');
    }
    else{
        document.getElementById(collapse).classList.remove('show');
    }
}

function setPagination(total, returned) {
    let cat = app.current.app + (app.current.tab === undefined ? '': app.current.tab);
    let totalPages = Math.ceil(total / settings.maxElementsPerPage);
    if (totalPages === 0) {
        totalPages = 1;
    }
    let p = [ document.getElementById(cat + 'PaginationTop'), document.getElementById(cat + 'PaginationBottom') ];
    
    for (let i = 0; i < p.length; i++) {
        let prev = p[i].children[0];
        let page = p[i].children[1].children[0];
        let pages = p[i].children[1].children[1];
        let next = p[i].children[2];
    
        page.innerText = (app.current.page / settings.maxElementsPerPage + 1) + ' / ' + totalPages;
        if (totalPages > 1) {
            page.removeAttribute('disabled');
            let pl = '';
            for (let j = 0; j < totalPages; j++) {
                pl += '<button data-page="' + (j * settings.maxElementsPerPage) + '" type="button" class="mr-1 mb-1 btn-sm btn btn-secondary">' +
                      ( j + 1) + '</button>';
            }
            pages.innerHTML = pl;
            page.classList.remove('nodropdown');
        }
        else if (total === -1) {
            page.setAttribute('disabled', 'disabled');
            page.innerText = (app.current.page / settings.maxElementsPerPage + 1);
            page.classList.add('nodropdown');
        }
        else {
            page.setAttribute('disabled', 'disabled');
            page.classList.add('nodropdown');
        }
        
        if (total > app.current.page + settings.maxElementsPerPage || total === -1 && returned >= settings.maxElementsPerPage) {
            next.removeAttribute('disabled');
            p[i].classList.remove('hide');
            document.getElementById(cat + 'ButtonsBottom').classList.remove('hide');
        }
        else {
            next.setAttribute('disabled', 'disabled');
            p[i].classList.add('hide');
            document.getElementById(cat + 'ButtonsBottom').classList.add('hide');
        }
    
        if (app.current.page > 0) {
            prev.removeAttribute('disabled');
            p[i].classList.remove('hide');
            document.getElementById(cat + 'ButtonsBottom').classList.remove('hide');
        }
        else {
            prev.setAttribute('disabled', 'disabled');
        }
    }
}

function genId(x) {
    return 'id' + x.replace(/[^\w-]/g, '');
}

function parseCmd(event, href) {
    event.preventDefault();
    let cmd = href;
    if (typeof(href) === 'string') {
        cmd = JSON.parse(href);
    }

    if (typeof window[cmd.cmd] === 'function') {
        switch(cmd.cmd) {
            case 'sendAPI':
                sendAPI(cmd.options[0].cmd, {}); 
                break;
            case 'toggleBtn':
            case 'toggleBtnChk':
            case 'toggleBtnGroup':
            case 'toggleBtnGroupCollapse':
            case 'setPlaySettings':
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

function gotoPage(x) {
    switch (x) {
        case 'next':
            app.current.page += settings.maxElementsPerPage;
            break;
        case 'prev':
            app.current.page -= settings.maxElementsPerPage;
            if (app.current.page < 0) {
                app.current.page = 0;
            }
            break;
        default:
            app.current.page = x;
    }
    appGoto(app.current.app, app.current.tab, app.current.view, app.current.page + '/' + app.current.filter + '/' + 
        app.current.sort + '/' + app.current.tag + '/' + app.current.search);
}
