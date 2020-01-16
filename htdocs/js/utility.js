"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

function zeroPad(num, places) {
  var zero = places - num.toString().length + 1;
  return Array(+(zero > 0 && zero)).join("0") + num;
}

function dirname(uri) {
    return uri.replace(/\/[^/]*$/, '');
}

function basename(uri) {
   return uri.split('/').reverse()[0];
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
        appGoto(app.current.app, app.current.tab, app.current.view, '0/' + filter + '/' + app.current.sort + '/' + app.current.search);
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
    if (el === 'searchCovergridSortTagsList' && settings.tags.includes('Date')) {
        tagList += '<button type="button" class="btn btn-secondary btn-sm btn-block" data-tag="Date">' + t('Date') + '</button>';
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

function toggleBtnGroupValue(btngrp, value) {
    let btns = btngrp.getElementsByTagName('button');
    for (let i = 0; i < btns.length; i++) {
        if (btns[i].getAttribute('data-value') == value) {
            btns[i].classList.add('active');
        }
        else {
            btns[i].classList.remove('active');
        }
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
    }
    else {
        b.classList.remove('active');
        b.innerText = 'radio_button_unchecked';
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
            case 'setPlaySettings':
                window[cmd.cmd](event.target, ... cmd.options);
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
    appGoto(app.current.app, app.current.tab, app.current.view, app.current.page + '/' + app.current.filter + '/' + app.current.sort + '/' + app.current.search);
}
