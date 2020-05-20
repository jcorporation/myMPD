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
    
    let elH1s = modal.getElementsByTagName('h1');
    for (let i = 0; i < elH1s.length; i++) {
        elH1s[i].innerText = obj.result.Title;
    }
    
    let songDetailsHTML = '';
    for (let i = 0; i < settings.tags.length; i++) {
        if (settings.tags[i] === 'Title' || obj.result[settings.tags[i]] === '-') {
            continue;
        }
        songDetailsHTML += '<tr><th>' + t(settings.tags[i]) + '</th><td data-tag="' + settings.tags[i] + '" data-name="' + encodeURI(obj.result[settings.tags[i]]) + '">';
        if (settings.browsetags.includes(settings.tags[i]) && obj.result[settings.tags[i]] !== '-') {
            songDetailsHTML += '<a class="text-success" href="#">' + e(obj.result[settings.tags[i]]) + '</a>';
        }
        else {
            songDetailsHTML += obj.result[settings.tags[i]];
        }
        songDetailsHTML += '</td></tr>';
    }
    songDetailsHTML += '<tr><th>' + t('Duration') + '</th><td>' + beautifyDuration(obj.result.Duration) + '</td></tr>';
    if (settings.featLibrary === true && settings.publish === true) {
        songDetailsHTML += '<tr><th>' + t('Filename') + '</th><td><a class="breakAll text-success" href="/browse/music/' + 
            encodeURI(obj.result.uri) + '" target="_blank" title="' + e(obj.result.uri) + '">' + 
            e(basename(obj.result.uri)) + '</a></td></tr>';
    }
    else {
        songDetailsHTML += '<tr><th>' + t('Filename') + '</th><td class="breakAll"><span title="' + e(obj.result.uri) + '">' + 
            e(basename(obj.result.uri)) + '</span></td></tr>';
    }
    songDetailsHTML += '<tr><th>' + t('Filetype') + '</th><td>' + filetype(obj.result.uri) + '</td></tr>';
    songDetailsHTML += '<tr><th>' + t('LastModified') + '</th><td>' + localeDate(obj.result.LastModified) + '</td></tr>';
    if (settings.featFingerprint === true) {
        songDetailsHTML += '<tr><th>' + t('Fingerprint') + '</th><td class="breakAll" id="fingerprint"><a class="text-success" data-uri="' + 
            encodeURI(obj.result.uri) + '" id="calcFingerprint" href="#">' + t('Calculate') + '</a></td></tr>';
    }
    if (obj.result.booklet === true && settings.publish === true) {
        songDetailsHTML += '<tr><th>' + t('Booklet') + '</th><td><a class="text-success" href="/browse/music/' + dirname(obj.result.uri) + '/' + settings.bookletName + '" target="_blank">' + t('Download') + '</a></td></tr>';
    }
    if (settings.featStickers === true) {
        songDetailsHTML += '<tr><th colspan="2" class="pt-3"><h5>' + t('Statistics') + '</h5></th></tr>' +
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
    
    document.getElementById('tbodySongDetails').innerHTML = songDetailsHTML;
    setVoteSongBtns(obj.result.like, obj.result.uri);
    
    if (settings.featLyrics === true) {
        getLyrics(obj.result.uri, document.getElementById('lyricsText'));
    }

    let showPictures = false;
    if (obj.result.images.length > 0 && settings.featLibrary === true && settings.publish === true) {
        showPictures = true;
    }
    else if (settings.coverimage === true) {
        showPictures = true;
    }
    
    let pictureEls = document.getElementsByClassName('featPictures');
    for (let i = 0; i < pictureEls.length; i++) {
        if (showPictures === true) {
            pictureEls[i].classList.remove('hide');
        }
        else {
            pictureEls[i].classList.add('hide');
        }
    }
    
    if (showPictures === true) {
        //add uri to image list to get embedded albumart
        let images = [ subdir + '/albumart/' + obj.result.uri ];
        //add all but coverfiles to image list
        for (let i = 0; i < obj.result.images.length; i++) {
            if (isCoverfile(obj.result.images[i]) === false) {
                images.push(subdir + '/browse/music/' + obj.result.images[i]);
            }
        }
    
        let carousel = '<div id="songPicsCarousel" class="carousel slide" data-ride="carousel">' +
            '<ol class="carousel-indicators">';
        for (let i = 0; i < images.length; i++) {
            carousel += '<li data-target="#songPicsCarousel" data-slide-to="' + i + '"' +
                (i === 0 ? ' class="active"' : '') + '></li>';
        }    
        carousel += '</ol>' +
            '<div class="carousel-inner" role="listbox">';
        for (let i = 0; i < images.length; i++) {
            carousel += '<div class="carousel-item' + (i === 0 ? ' active' : '') + '"><div></div></div>';
        }
        carousel += '</div>' +
            '<a class="carousel-control-prev" href="#songPicsCarousel" data-slide="prev">' +
                '<span class="carousel-control-prev-icon"></span>' +
            '</a>' +
            '<a class="carousel-control-next" href="#songPicsCarousel" data-slide="next">' +
                '<span class="carousel-control-next-icon"></span>' +
            '</a>' +
            '</div>';
    
        document.getElementById('tabSongPics').innerHTML = carousel;
        let carouselItems = document.getElementById('tabSongPics').getElementsByClassName('carousel-item');
        for (let i = 0; i < carouselItems.length; i++) {
            carouselItems[i].children[0].style.backgroundImage = 'url("' + encodeURI(images[i]) + '")';
        }
        let myCarousel = document.getElementById('songPicsCarousel');
        //eslint-disable-next-line no-undef, no-unused-vars
        let myCarouselInit = new Carousel(myCarousel, {
            interval: false,
            pause: false
        });
    }
    else {
        document.getElementById('tabSongPics').innerText = '';
    }
}

function isCoverfile(uri) {
    let filename = basename(uri).toLowerCase();
    let fileparts = filename.split('.');
    
    let extensions = ['png', 'jpg', 'jpeg', 'svg', 'webp', 'tiff', 'bmp'];
    let coverimageNames = settings.coverimageName.split(',');
    for (let i = 0; i < coverimageNames.length; i++) {
        let name = coverimageNames[i].trim();
        if (filename === name) {
            return true;
        }
        if (fileparts[1]) {
            if (name === fileparts[0] && extensions.includes(fileparts[1])) {
                return true;
            }
        }
    }
    return false;
}

function getLyrics(uri, el) {
    el.classList.add('opacity05');
    let ajaxRequest=new XMLHttpRequest();
    
    ajaxRequest.open('GET', subdir + '/lyrics/' + uri, true);
    ajaxRequest.onreadystatechange = function() {
        if (ajaxRequest.readyState === 4) {
            el.innerText = ajaxRequest.responseText === 'No lyrics found' ? t(ajaxRequest.responseText) : ajaxRequest.responseText;
            el.classList.remove('opacity05');
        }
    };
    ajaxRequest.send();
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
        
    if (vote === 2 && domCache.btnVoteUp.classList.contains('highlight')) {
        vote = 1;
    }
    else if (vote === 0 && domCache.btnVoteDown.classList.contains('highlight')) {
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
        domCache.btnVoteUp.classList.remove('highlight');
        domCache.btnVoteDown.classList.add('highlight');
        if (domCache.btnVoteUp2) {
            domCache.btnVoteUp2.classList.remove('highlight');
            domCache.btnVoteDown2.classList.add('highlight');
        }
    } else if (vote === 1) {
        domCache.btnVoteUp.classList.remove('highlight');
        domCache.btnVoteDown.classList.remove('highlight');
        if (domCache.btnVoteUp2) {
            domCache.btnVoteUp2.classList.remove('highlight');
            domCache.btnVoteDown2.classList.remove('highlight');
        }
    } else if (vote === 2) {
        domCache.btnVoteUp.classList.add('highlight');
        domCache.btnVoteDown.classList.remove('highlight');
        if (domCache.btnVoteUp2) {
            domCache.btnVoteUp2.classList.add('highlight');
            domCache.btnVoteDown2.classList.remove('highlight');
        }
    }
}
