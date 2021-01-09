"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

function removeIsInvalid(parentEl) {
    const els = parentEl.getElementsByClassName('is-invalid')
    for (let i = 0; i < els.length; i++) {
        els[i].classList.remove('is-invalid');
    }
}

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

function getSelectValue(el) {
    if (typeof el === 'string')	{
        el = document.getElementById(el);
    }
    if (el && el.selectedIndex >= 0) {
        return el.options[el.selectedIndex].value;
    }
    return undefined;
}

function getSelectedOptionAttribute(selectId, attribute) {
    let el = document.getElementById(selectId);
    if (el && el.selectedIndex >= 0) {
        return el.options[el.selectedIndex].getAttribute(attribute);
    }
    return undefined;
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
        if (settings.featTags === true && settings.featAdvsearch === true) {
            tagList = '<button type="button" class="btn btn-secondary btn-sm btn-block" data-tag="Database">' + t('Database') + '</button>';
        }
        else {
            tagList = '';
        }
    }
    if (el === 'BrowseDatabaseByTagDropdown' || el === 'BrowseNavFilesystemDropdown' || el === 'BrowseNavPlaylistsDropdown') {
        if (el === 'BrowseDatabaseByTagDropdown') {
            tagList += '<div class="dropdown-divider"></div>';
        }
        tagList += '<button type="button" class="btn btn-secondary btn-sm btn-block' + (el === 'BrowseNavPlaylistsDropdown' ? ' active' : '') + '" data-tag="Playlists">' + t('Playlists') + '</button>' +
            '<button type="button" class="btn btn-secondary btn-sm btn-block' + (el === 'BrowseNavFilesystemDropdown' ? ' active' : '') + '" data-tag="Filesystem">' + t('Filesystem') + '</button>'
    }
    else if (el === 'databaseSortTagsList') {
        if (settings.tags.includes('Date') === true && settings[list].includes('Date') === false) {
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
    let cat = app.current.app + (app.current.tab === undefined ? '' : app.current.tab);
    let totalPages = app.current.limit > 0 ? Math.ceil(total / app.current.limit) : 1;
    if (totalPages === 0) {
        totalPages = 1;
    }
    let curPage = app.current.limit > 0 ? app.current.offset / app.current.limit + 1 : 1;
    
    const paginationHTML = '<button title="' + t('First page') + '" type="button" class="btn btn-group-prepend btn-secondary material-icons">first_page</button>' +
          '<button title="' + t('Previous page') + '" type="button" class="btn btn-group-prepend btn-secondary material-icons">navigate_before</button>' +
          '<div class="btn-group">' +
            '<button class="btn btn-secondary dropdown-toggle" type="button" data-toggle="dropdown"></button>' +
            '<div class="dropdown-menu bg-lite-dark px-2 pages dropdown-menu-right"></div>' +
          '</div>' +
          '<button title="' + t('Next page') + '" type="button" class="btn btn-secondary btn-group-append material-icons">navigate_next</button>' +
          '<button title="' + t('Last page') + '" type="button" class="btn btn-secondary btn-group-append material-icons">last_page</button>';

    let bottomBarHTML = '<button type="button" class="btn btn-secondary material-icons" title="' + t('To top') + '">keyboard_arrow_up</button>' +
          '<div>' +
          '<select class="form-control custom-select border-secondary" title="' + t('Elements per page') + '">';
    let nrEls = [25, 50, 100, 200, 0];
    for (let i of nrEls) {
        bottomBarHTML += '<option value="' + i + '"' + (app.current.limit === i ? ' selected' : '') + '>' + (i > 0 ? i : t('All')) + '</option>';
    }
    bottomBarHTML += '</select>' +
          '</div>' +
          '<div id="' + cat + 'PaginationBottom" class="btn-group dropup pagination">' +
          paginationHTML +
          '</div>' +
          '</div>';

    const bottomBar = document.getElementById(cat + 'ButtonsBottom');
    bottomBar.innerHTML = bottomBarHTML;
    
    const buttons = bottomBar.getElementsByTagName('button');
    buttons[0].addEventListener('click', function() {
        event.preventDefault();
        scrollToPosY(0);
    }, false);
    
    bottomBar.getElementsByTagName('select')[0].addEventListener('change', function(event) {
        const newLimit = parseInt(getSelectValue(event.target));
        if (app.current.limit !== newLimit) {
            gotoPage(app.current.offset, newLimit);
        }
    }, false);
    
    document.getElementById(cat + 'PaginationTop').innerHTML = paginationHTML;
    
    const offsetLast = app.current.offset + app.current.limit;
    let p = [ document.getElementById(cat + 'PaginationTop'), document.getElementById(cat + 'PaginationBottom') ];
    
    for (let i = 0; i < p.length; i++) {
        const first = p[i].children[0];
        const prev = p[i].children[1];
        const page = p[i].children[2].children[0];
        const pages = p[i].children[2].children[1];
        const next = p[i].children[3];
        const last = p[i].children[4];
    
        page.innerText = curPage + ' / ' + totalPages;
        if (totalPages > 1) {
            page.removeAttribute('disabled');
            let pl = '';
            for (let j = 0; j < totalPages; j++) {
                let o = j * app.current.limit;
                pl += '<button data-offset="' + o + '" type="button" class="btn-sm btn btn-secondary' +
                      ( o === app.current.offset ? ' active' : '') + '">' +
                      ( j + 1) + '</button>';
            }
            pages.innerHTML = pl;
            page.classList.remove('nodropdown');
            pages.addEventListener('click', function(event) {
                if (event.target.nodeName === 'BUTTON') {
                    gotoPage(event.target.getAttribute('data-offset'));
                }
            }, false);
            //eslint-disable-next-line no-unused-vars
            const pagesDropdown = new BSN.Dropdown(page);
            
            let lastPageOffset = (totalPages - 1) * app.current.limit;
            if (lastPageOffset === app.current.offset) {
                last.setAttribute('disabled', 'disabled');
            }
            else {
                last.removeAttribute('disabled');
                last.classList.remove('hide');
                next.classList.remove('rounded-right');
                last.addEventListener('click', function() {
                    event.preventDefault();
                    gotoPage(lastPageOffset);
                }, false);
            }
        }
        else if (total === -1) {
            page.setAttribute('disabled', 'disabled');
            page.innerText = curPage;
            page.classList.add('nodropdown');
            last.setAttribute('disabled', 'disabled');
            last.classList.add('hide');
            next.classList.add('rounded-right');
        }
        else {
            page.setAttribute('disabled', 'disabled');
            page.classList.add('nodropdown');
            last.setAttribute('disabled', 'disabled');
        }
        
        if ((total > offsetLast && offsetLast > 0 ) || (total === -1 && returned >= app.current.limit)) {
            next.removeAttribute('disabled');
            p[i].classList.remove('hide');
            next.addEventListener('click', function() {
                event.preventDefault();
                gotoPage('next');
            }, false);
        }
        else {
            next.setAttribute('disabled', 'disabled');
            if (i === 0) {
                p[i].classList.add('hide');
            }
        }

        if (app.current.offset > 0) {
            prev.removeAttribute('disabled');
            p[i].classList.remove('hide');
            prev.addEventListener('click', function() {
                event.preventDefault();
                gotoPage('prev');
            }, false);
            first.removeAttribute('disabled');
            first.addEventListener('click', function() {
                event.preventDefault();
                gotoPage(0);
            }, false);
        }
        else {
            prev.setAttribute('disabled', 'disabled');
            first.setAttribute('disabled', 'disabled');
        }
    }
    
    //hide bottom pagination bar if returned < limit
    if (returned < app.current.limit) {
        document.getElementById(cat + 'ButtonsBottom').classList.add('hide');
    }
    else {
        document.getElementById(cat + 'ButtonsBottom').classList.remove('hide');
    }
}

function genId(x) {
    return 'id' + x.replace(/[^\w-]/g, '');
}

function parseCmd(event, href) {
    if (event !== null) {
        event.preventDefault();
    }
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
            case 'zoomPicture':
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
        app.current.limit = limit;
        if (app.current.limit === 0) {
            app.current.offset = 0;
        }
        else if (app.current.offset % app.current.limit > 0) {
            app.current.offset = Math.floor(app.current.offset / app.current.limit);
        }
    }
    appGoto(app.current.app, app.current.tab, app.current.view, 
        app.current.offset, app.current.limit, app.current.filter, app.current.sort, app.current.tag, app.current.search, 0);
}
