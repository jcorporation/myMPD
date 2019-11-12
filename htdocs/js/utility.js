"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

function dirname(uri) {
    return uri.replace(/\/[^/]*$/, '');
}

function basename(uri) {
   return uri.split('/').reverse()[0];
}

function filetype(uri) {
    let ext = uri.split('.').pop();
    switch (ext) {
        case 'mp3': return 'MP3 - MPEG-1 Audio Layer III';
        case 'flac': return 'FLAC - Free Lossless Audio Codec';
        case 'ogg': return 'OGG - Ogg Vorbis';
        default: ext.toUpperCase();
    }
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
        document.getElementById(desc).innerText = aBtn.innerText;
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

function toggleBtn(btn, state) {
    let b = document.getElementById(btn);
    if (!b) {
        return;
    }
    if (state === undefined) {
        //toggle state
        state = b.classList.contains('active') ? false : true;
    }
    else if (state === 0 || state === 1) {
        //1 = true, 0 = false
        state = state === 1 ? true : false;
    }

    if (state === true) {
        b.classList.add('active');
    }
    else {
        b.classList.remove('active');
    }
}

function toggleBtnChk(btn, state) {
    let b = document.getElementById(btn);
    if (!b) {
        return;
    }
    if (state === undefined) {
        //toggle state
        state = b.classList.contains('active') ? false : true;
    }
    else if (state === 0 || state === 1) {
        //1 = true, 0 = false
        state = state === 1 ? true : false;
    }

    if (state === true) {
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
    if (totalPages === 0) 
        totalPages = 1;
    let p = ['PaginationTop', 'PaginationBottom'];
    for (let i = 0; i < 2; i++) {
        document.getElementById(cat + p[i] + 'Page').innerText = (app.current.page / settings.maxElementsPerPage + 1) + ' / ' + totalPages;
        if (totalPages > 1) {
            document.getElementById(cat + p[i] + 'Page').removeAttribute('disabled');
            let pl = '';
            for (let j = 0; j < totalPages; j++) {
                pl += '<button data-page="' + (j * settings.maxElementsPerPage) + '" type="button" class="mr-1 mb-1 btn-sm btn btn-secondary">' +
                    ( j + 1) + '</button>';
            }
            document.getElementById(cat + p[i] + 'Pages').innerHTML = pl;
            document.getElementById(cat + p[i] + 'Page').classList.remove('nodropdown');
        }
        else if (total === -1) {
            document.getElementById(cat + p[i] + 'Page').setAttribute('disabled', 'disabled');
            document.getElementById(cat + p[i] + 'Page').innerText = (app.current.page / settings.maxElementsPerPage + 1);
            document.getElementById(cat + p[i] + 'Page').classList.add('nodropdown');
        }
        else {
            document.getElementById(cat + p[i] + 'Page').setAttribute('disabled', 'disabled');
            document.getElementById(cat + p[i] + 'Page').classList.add('nodropdown');
        }
    
        if (total > app.current.page + settings.maxElementsPerPage || total === -1 && returned >= settings.maxElementsPerPage) {
            document.getElementById(cat + p[i] + 'Next').removeAttribute('disabled');
            document.getElementById(cat + p[i]).classList.remove('hide');
            document.getElementById(cat + 'ButtonsBottom').classList.remove('hide');
        }
        else {
            document.getElementById(cat + p[i] + 'Next').setAttribute('disabled', 'disabled');
            document.getElementById(cat + p[i]).classList.add('hide');
            document.getElementById(cat + 'ButtonsBottom').classList.add('hide');
        }
    
        if (app.current.page > 0) {
            document.getElementById(cat + p[i] + 'Prev').removeAttribute('disabled');
            document.getElementById(cat + p[i]).classList.remove('hide');
            document.getElementById(cat + 'ButtonsBottom').classList.remove('hide');
        } else {
            document.getElementById(cat + p[i] + 'Prev').setAttribute('disabled', 'disabled');
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
            if (app.current.page < 0)
                app.current.page = 0;
            break;
        default:
            app.current.page = x;
    }
    appGoto(app.current.app, app.current.tab, app.current.view, app.current.page + '/' + app.current.filter + '/' + app.current.sort + '/' + app.current.search);
}
