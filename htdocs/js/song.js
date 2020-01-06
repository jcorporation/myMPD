"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

function songDetails(uri) {
    sendAPI("MPD_API_DATABASE_SONGDETAILS", {"uri": uri}, parseSongDetails);
    modalSongDetails.show();
}

function parseFingerprint(obj) {
    let textarea = document.createElement('textarea');
    textarea.value = obj.result.fingerprint;
    textarea.classList.add('form-control', 'text-monospace', 'small');
    let fpTd = document.getElementById('fingerprint');
    fpTd.innerHTML = '';
    fpTd.appendChild(textarea);
}

function parseSongDetails(obj) {
    let modal = document.getElementById('modalSongDetails');
    modal.getElementsByClassName('album-cover')[0].style.backgroundImage = 'url("' + subdir + '/albumart/' + obj.result.uri + '"), url("' + subdir + '/assets/coverimage-loading.svg")';
    modal.getElementsByTagName('h1')[0].innerText = obj.result.Title;
    
    let songDetails = '';
    for (let i = 0; i < settings.tags.length; i++) {
        if (settings.tags[i] === 'Title') {
            continue;
        }
        songDetails += '<tr><th>' + t(settings.tags[i]) + '</th><td data-tag="' + settings.tags[i] + '" data-name="' + encodeURI(obj.result[settings.tags[i]]) + '">';
        if (settings.browsetags.includes(settings.tags[i])) {
            songDetails += '<a class="text-success" href="#">' + e(obj.result[settings.tags[i]]) + '</a>';
        }
        else {
            songDetails += obj.result[settings.tags[i]];
        }
        songDetails += '</td></tr>';
    }
    songDetails += '<tr><th>' + t('Duration') + '</th><td>' + beautifyDuration(obj.result.Duration) + '</td></tr>';
    if (settings.featLibrary === true && settings.publishLibrary === true) {
        songDetails += '<tr><th>' + t('Filename') + '</th><td><a class="breakAll text-success" href="/library/' + 
            encodeURI(obj.result.uri) + '" download title="' + e(obj.result.uri) + '">' + 
            e(basename(obj.result.uri)) + '</a></td></tr>';
    }
    else {
        songDetails += '<tr><th>' + t('Filename') + '</th><td class="breakAll"><span title="' + e(obj.result.uri) + '">' + 
            e(basename(obj.result.uri)) + '</span></td></tr>';
    }
    songDetails += '<tr><th>' + t('Filetype') + '</th><td>' + filetype(obj.result.uri) + '</td></tr>';
    if (settings.featFingerprint === true) {
        songDetails += '<tr><th>' + t('Fingerprint') + '</th><td class="breakAll" id="fingerprint"><a class="text-success" data-uri="' + 
            encodeURI(obj.result.uri) + '" id="calcFingerprint" href="#">' + t('Calculate') + '</a></td></tr>';
    }
    if (settings.featStickers === true) {
        songDetails += '<tr><th colspan="2" class="pt-3"><h5>' + t('Statistics') + '</h5></th></tr>' +
            '<tr><th>' + t('Play count') + '</th><td>' + obj.result.playCount + '</td></tr>' +
            '<tr><th>' + t('Skip count') + '</th><td>' + obj.result.skipCount + '</td></tr>' +
            '<tr><th>' + t('Last played') + '</th><td>' + (obj.result.lastPlayed === 0 ? t('never') : localeDate(obj.result.lastPlayed)) + '</td></tr>' +
            '<tr><th>' + t('Last skipped') + '</th><td>' + (obj.result.lastSkipped === 0 ? t('never') : localeDate(obj.result.lastSkipped)) + '</td></tr>' +
            '<tr><th>' + t('Like') + '</th><td>' +
              '<div class="btn-group btn-group-sm">' +
                '<button title="' + t('Dislike song') + '" id="btnVoteDown2" data-href=\'{"cmd": "voteSong", "options": [0]}\' class="btn btn-sm btn-light material-icons">thumb_down</button>' +
                '<button title="' + t('Like song') + '" id="btnVoteUp2" data-href=\'{"cmd": "voteSong", "options": [2]}\' class="btn btn-sm btn-light material-icons">thumb_up</button>' +
              '</div>' +
            '</td></tr>';
    }
    
    modal.getElementsByTagName('tbody')[0].innerHTML = songDetails;
    setVoteSongBtns(obj.result.like, obj.result.uri);
}

//eslint-disable-next-line no-unused-vars
function loveSong() {
    sendAPI("MPD_API_LOVE", {});
}

//eslint-disable-next-line no-unused-vars
function voteSong(vote) {
    let uri = decodeURI(domCache.currentTitle.getAttribute('data-uri'));
    if (uri === '') {
        return;
    }
        
    if (vote === 2 && domCache.btnVoteUp.classList.contains('active-fg-green')) {
        vote = 1;
    }
    else if (vote === 0 && domCache.btnVoteDown.classList.contains('active-fg-red')) {
        vote = 1;
    }
    sendAPI("MPD_API_LIKE", {"uri": uri, "like": vote});
    setVoteSongBtns(vote, uri);
}

function setVoteSongBtns(vote, uri) {
    domCache.btnVoteUp2 = document.getElementById('btnVoteUp2');
    domCache.btnVoteDown2 = document.getElementById('btnVoteDown2');

    if (uri === '' || uri.indexOf('://') > -1) {
        domCache.btnVoteUp.setAttribute('disabled', 'disabled');
        domCache.btnVoteDown.setAttribute('disabled', 'disabled');
        if (domCache.btnVoteUp2) {
            domCache.btnVoteUp2.setAttribute('disabled', 'disabled');
            domCache.btnVoteDown2.setAttribute('disabled', 'disabled');
        }
    } else {
        domCache.btnVoteUp.removeAttribute('disabled');
        domCache.btnVoteDown.removeAttribute('disabled');
        if (domCache.btnVoteUp2) {
            domCache.btnVoteUp2.removeAttribute('disabled');
            domCache.btnVoteDown2.removeAttribute('disabled');
        }
    }
    
    if (vote === 0) {
        domCache.btnVoteUp.classList.remove('active-fg-green');
        domCache.btnVoteDown.classList.add('active-fg-red');
        if (domCache.btnVoteUp2) {
            domCache.btnVoteUp2.classList.remove('active-fg-green');
            domCache.btnVoteDown2.classList.add('active-fg-red');
        }
    } else if (vote === 1) {
        domCache.btnVoteUp.classList.remove('active-fg-green');
        domCache.btnVoteDown.classList.remove('active-fg-red');
        if (domCache.btnVoteUp2) {
            domCache.btnVoteUp2.classList.remove('active-fg-green');
            domCache.btnVoteDown2.classList.remove('active-fg-red');
        }
    } else if (vote === 2) {
        domCache.btnVoteUp.classList.add('active-fg-green');
        domCache.btnVoteDown.classList.remove('active-fg-red');
        if (domCache.btnVoteUp2) {
            domCache.btnVoteUp2.classList.add('active-fg-green');
            domCache.btnVoteDown2.classList.remove('active-fg-red');
        }
    }
}
