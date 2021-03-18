"use strict";
// SPDX-License-Identifier: GPL-2.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function initSong() {
    document.getElementById('tbodySongDetails').addEventListener('click', function(event) {
        if (event.target.nodeName === 'A') {
            if (event.target.id === 'calcFingerprint') {
                sendAPI("MPD_API_DATABASE_FINGERPRINT", {"uri": getAttDec(event.target, 'data-uri')}, parseFingerprint);
                event.preventDefault();
                const spinner = document.createElement('div');
                spinner.classList.add('spinner-border', 'spinner-border-sm');
                event.target.classList.add('hide');
                event.target.parentNode.appendChild(spinner);
            }
            else if (event.target.classList.contains('external')) {
                //do nothing, link opens in new browser window
            }
            else if (event.target.parentNode.getAttribute('data-tag') !== null) {
                uiElements.modalSongDetails.hide();
                event.preventDefault();
                gotoBrowse(event);
            } 
        }
        else if (event.target.nodeName === 'BUTTON') { 
            if (event.target.getAttribute('data-href')) {
                parseCmd(event, event.target.getAttribute('data-href'));
            }
        }
    }, false);
}

function songDetails(uri) {
    sendAPI("MPD_API_DATABASE_SONGDETAILS", {"uri": uri}, parseSongDetails);
    uiElements.modalSongDetails.show();
}

function parseFingerprint(obj) {
    const textarea = document.createElement('textarea');
    textarea.value = obj.result.fingerprint;
    textarea.classList.add('form-control', 'text-monospace', 'small');
    const fpTd = document.getElementById('fingerprint');
    fpTd.innerHTML = '';
    fpTd.appendChild(textarea);
}

function getMBtagLink(tag, value) {
    let MBentity = '';
    switch (tag) {
        case 'MUSICBRAINZ_ALBUMARTISTID':
        case 'MUSICBRAINZ_ARTISTID':
            MBentity = 'artist';
            break;
        case 'MUSICBRAINZ_ALBUMID':
            MBentity = 'release';
            break;
        case 'MUSICBRAINZ_RELEASETRACKID':
            MBentity = 'track';
            break;
        case 'MUSICBRAINZ_TRACKID':
            MBentity = 'recording';
            break;
    }
    if (MBentity === '') {
        return e(value);
    }
    else {
        return '<a title="' + t('Lookup at musicbrainz') + '" class="text-success external" target="_musicbrainz" href="https://musicbrainz.org/' + MBentity + '/' + encodeURI(value) + '">' +
            '<span class="mi">open_in_browser</span>&nbsp;' + value + '</a>';
    }
}

function parseSongDetails(obj) {
    const modal = document.getElementById('modalSongDetails');
    modal.getElementsByClassName('album-cover')[0].style.backgroundImage = 'url("' + subdir + '/albumart/' + obj.result.uri + '"), url("' + subdir + '/assets/coverimage-loading.svg")';
    
    const elH1s = modal.getElementsByTagName('h1');
    for (let i = 0; i < elH1s.length; i++) {
        elH1s[i].innerText = obj.result.Title;
    }
    
    let songDetailsHTML = '';
    for (let i = 0; i < settings.tags.length; i++) {
        if (settings.tags[i] === 'Title' || obj.result[settings.tags[i]] === '-') {
            continue;
        }
        songDetailsHTML += '<tr><th>' + t(settings.tags[i]) + '</th><td data-tag="' + settings.tags[i] + '" data-name="' + encodeURI(obj.result[settings.tags[i]]) + '"';
        if (settings.tags[i] === 'Album' && obj.result[tagAlbumArtist] !== null) {
            songDetailsHTML += ' data-albumartist="' + encodeURI(obj.result[tagAlbumArtist]) + '"';
        }
        songDetailsHTML += '>';
        if (settings.browsetags.includes(settings.tags[i]) && obj.result[settings.tags[i]] !== '-') {
            songDetailsHTML += '<a class="text-success" href="#">' + e(obj.result[settings.tags[i]]) + '</a>';
        }
        else if (settings.tags[i].indexOf('MUSICBRAINZ') === 0) {
            songDetailsHTML += getMBtagLink(settings.tags[i], obj.result[settings.tags[i]]);
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
            e(basename(obj.result.uri, true)) + '</a></td></tr>';
    }
    else {
        songDetailsHTML += '<tr><th>' + t('Filename') + '</th><td class="breakAll"><span title="' + e(obj.result.uri) + '">' + 
            e(basename(obj.result.uri, true)) + '</span></td></tr>';
    }
    songDetailsHTML += '<tr><th>' + t('Filetype') + '</th><td>' + filetype(obj.result.uri) + '</td></tr>';
    songDetailsHTML += '<tr><th>' + t('LastModified') + '</th><td>' + localeDate(obj.result.LastModified) + '</td></tr>';
    if (settings.featFingerprint === true) {
        songDetailsHTML += '<tr><th>' + t('Fingerprint') + '</th><td class="breakAll" id="fingerprint"><a class="text-success" data-uri="' + 
            encodeURI(obj.result.uri) + '" id="calcFingerprint" href="#">' + t('Calculate') + '</a></td></tr>';
    }
    if (obj.result.bookletPath !== '' && settings.publish === true) {
        songDetailsHTML += '<tr><th>' + t('Booklet') + '</th><td><a class="text-success" href="' + encodeURI(subdir + '/browse/music/' + dirname(obj.result.uri) + '/' + settings.bookletName) + '" target="_blank">' + t('Download') + '</a></td></tr>';
    }
    if (settings.featStickers === true) {
        songDetailsHTML += '<tr><th colspan="2" class="pt-3"><h5>' + t('Statistics') + '</h5></th></tr>';
        for (const sticker of stickerList) {
            if (sticker === 'stickerLike') {
                songDetailsHTML += '<tr><th>' + t('Like') + '</th><td><div class="btn-group btn-group-sm" data-uri="' + encodeURI(obj.result.uri) + '">' +
                    '<button title="' + t('Dislike song') + '" data-href=\'{"cmd": "voteSong", "options": [0]}\' class="btn btn-sm btn-secondary mi' + (obj.result[sticker] === 0 ? ' active' : '') + '">thumb_down</button>' +
                    '<button title="' + t('Like song') + '" data-href=\'{"cmd": "voteSong", "options": [2]}\' class="btn btn-sm btn-secondary mi' + (obj.result[sticker] === 2 ? ' active' : '') + '">thumb_up</button>' +
                    '</div></td></tr>';
            }
            else {
                songDetailsHTML += '<tr><th>' + t(sticker) + '</th><td>' + printValue(sticker,  obj.result[sticker]) + '</td></tr>';
            }
        }
    }
    
    document.getElementById('tbodySongDetails').innerHTML = songDetailsHTML;
    
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
    
    const pictureEls = document.getElementsByClassName('featPictures');
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
        const images = [ subdir + '/albumart/' + obj.result.uri ];
        //add all but coverfiles to image list
        if (settings.publish === true) {
            for (let i = 0; i < obj.result.images.length; i++) {
                if (isCoverfile(obj.result.images[i]) === false) {
                    images.push(subdir + '/browse/music/' + obj.result.images[i]);
                }
            }
        }
        const imgEl = document.getElementById('tabSongPics');
        createImgCarousel(imgEl, 'songPicsCarousel', images);
    }
    else {
        document.getElementById('tabSongPics').innerText = '';
    }
}

function isCoverfile(uri) {
    const filename = basename(uri).toLowerCase();
    const fileparts = filename.split('.');
    
    const extensions = ['png', 'jpg', 'jpeg', 'svg', 'webp', 'tiff', 'bmp'];
    const coverimageNames = settings.coverimageName.split(',');
    for (let i = 0; i < coverimageNames.length; i++) {
        const name = coverimageNames[i].trim();
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
    if (isValidUri(uri) === false || isStreamUri(uri) === true) {
        el.innerHTML = t('No lyrics found');
        return;
    }
    el.classList.add('opacity05');
    sendAPI("MPD_API_LYRICS_GET", {"uri": uri}, function(obj) {
        if (obj.error) {
            el.innerText = t(obj.error.message);
        }
        else if (obj.result.message) {
            el.innerText = t(obj.result.message);
        }
        else if (obj.result.returnedEntities === 0) {
            el.innerHTML = t('No lyrics found');
        }
        else {
            let lyricsHeader = '<span class="lyricsHeader" class="btn-group-toggle" data-toggle="buttons">';
            let lyrics = '<div class="lyricsTextContainer">';
            const clickable = el.parentNode.getAttribute('id') === 'currentLyrics' ? true : false;
            showSyncedLyrics = false;
            for (let i = 0; i < obj.result.returnedEntities; i++) {
                let ht = obj.result.data[i].desc;
                if (ht !== '' && obj.result.data[i].lang !== '') {
                    ht += ' (' + obj.result.data[i].lang + ')';
                }
                else if (obj.result.data[i].lang !== '') {
                    ht = obj.result.data[i].lang;
                }
                else {
                    ht = i;
                }
                lyricsHeader += '<label data-num="' + i + '" class="btn btn-sm btn-outline-secondary mr-2 lyricsChangeButton' + (i === 0 ? ' active' : '') + '" title="' + 
                    (obj.result.data[i].synced === true ? t('Synced lyrics') : t('Unsynced lyrics')) + ': ' + e(ht) + '">' + e(ht) + '</label>';
                lyrics += '<div class="lyricsText ' + (i > 0 ? 'hide' : '') + (obj.result.data[i].synced === true ? 'lyricsSyncedText' : '') + 
                    (clickable === true ? '' : ' fullHeight') + '">' +
                    (obj.result.data[i].synced === true ? parseSyncedLyrics(obj.result.data[i].text, clickable) : e(obj.result.data[i].text).replace(/\n/g, "<br/>")) + 
                    '</div>';
                if (obj.result.data[i].synced === true) {
                    showSyncedLyrics = true;
                }
            }
            lyricsHeader += '</span>';
            lyrics += '</div>';
            const lyricsScroll = showSyncedLyrics === false || clickable === false ? '' :
                '<button class="btn btn-sm mi mr-2 active" id="lyricsScroll">autorenew</button>';
            if (obj.result.returnedEntities > 1) {
                el.innerHTML = lyricsScroll + lyricsHeader + lyrics;
                el.getElementsByClassName('lyricsHeader')[0].addEventListener('click', function(event) {
                    if (event.target.nodeName === 'LABEL') {
                        event.target.parentNode.getElementsByClassName('active')[0].classList.remove('active');
                        event.target.classList.add('active');
                        const nr = parseInt(event.target.getAttribute('data-num'));
                        const tEls = el.getElementsByClassName('lyricsText');
                        for (let i = 0; i < tEls.length; i++) {
                            if (i === nr) {
                                tEls[i].classList.remove('hide');
                            }
                            else {
                                tEls[i].classList.add('hide');
                            }
                        }
                    }
                }, false);
            }
            else {
                el.innerHTML = lyricsScroll + lyrics;
            }
            if (showSyncedLyrics === true && clickable === true) {
                document.getElementById('lyricsScroll').addEventListener('click', function(event) {
                    toggleBtn(event.target);
                    scrollSyncedLyrics = event.target.classList.contains('active');
                }, false);
                const textEls = el.getElementsByClassName('lyricsSyncedText');
                for (let i = 0; i < textEls.length; i++) {
                    //seek to songpos in click
                    textEls[i].addEventListener('click', function(event) {
                        const sec = event.target.getAttribute('data-sec');
                        if (sec !== null) {
                            sendAPI("MPD_API_PLAYER_SEEK", {"songid": currentSong.currentSongId, "seek": parseInt(sec)});
                        }
                    }, false); 
                }
            }
        }
        el.classList.remove('opacity05');
    }, true);
}

function parseSyncedLyrics(text, clickable) {
    let html = '';
    const lines = text.replace(/\r/g, '').split('\n');
    for (let i = 0; i < lines.length; i++) {
        //line must start with timestamp
        const line = lines[i].match(/^\[(\d+):(\d+)\.(\d+)\](.*)$/);
        if (line) {
            const sec = parseInt(line[1]) * 60 + parseInt(line[2]);
            //line[3] are hundreths of a seconde - ignore it for the moment
            html += '<p><span class="' + (clickable === true ? 'clickable' : '') + '" data-sec="' + sec + '">';
            if (line[4].match(/^\s+$/)) {
                html += '&nbsp;';
            }
            else {
                //support of extended lrc format - timestamps for words
                html += line[4].replace(/<(\d+):(\d+)\.\d+>/g, function(m0, m1, m2) {
                    //hundreths of a secondes are ignored
                    const wsec = parseInt(m1) * 60 + parseInt(m2);
                    return '</span><span class="' + (clickable === true ? 'clickable' : '') + '" data-sec="' + wsec + '">';
                });
            }
            html += '</span></p>';
        }
    }
    html += '';
    return html;
}

//eslint-disable-next-line no-unused-vars
function loveSong() {
    sendAPI("MPD_API_LOVE", {});
}

//used in songdetails modal
//eslint-disable-next-line no-unused-vars
function voteSong(el, vote) {
    if (vote === 0 && el.classList.contains('active')) {
        vote = 1;
        el.classList.remove('active');
    }
    else if (vote === 2 && el.classList.contains('active')) {
        vote = 1;
        el.classList.remove('active');
    }
    const aEl = el.parentNode.getElementsByClassName('active')[0];
    if (aEl !== undefined) {
        aEl.classList.remove('active');
    }
    if (vote === 0 || vote === 2) {
        el.classList.add('active');
    }
    const uri = getAttDec(el.parentNode, 'data-uri');
    sendAPI("MPD_API_LIKE", {"uri": uri, "like": vote});
}

//eslint-disable-next-line no-unused-vars
function voteCurrentSong(vote) {
    const uri = getAttDec(document.getElementById('currentTitle'), 'data-uri');
    if (uri === '') {
        return;
    }
        
    if (vote === 2 && document.getElementById('btnVoteUp').classList.contains('highlight')) {
        vote = 1;
    }
    else if (vote === 0 && document.getElementById('btnVoteDown').classList.contains('highlight')) {
        vote = 1;
    }
    sendAPI("MPD_API_LIKE", {"uri": uri, "like": vote});
    setVoteSongBtns(vote, uri);
}

function setVoteSongBtns(vote, uri) {
    if (uri === undefined) {
        uri = '';
    }

    if (isValidUri(uri) === false || isStreamUri(uri) === true) {
        disableEl('btnVoteUp');
        disableEl('btnVoteDown');
        document.getElementById('btnVoteUp').classList.remove('highlight');
        document.getElementById('btnVoteDown').classList.remove('highlight');
    }
    else {
        enableEl('btnVoteUp');
        enableEl('btnVoteDown');
    }
    
    if (vote === 0) {
        document.getElementById('btnVoteUp').classList.remove('highlight');
        document.getElementById('btnVoteDown').classList.add('highlight');
    }
    else if (vote === 1) {
        document.getElementById('btnVoteUp').classList.remove('highlight');
        document.getElementById('btnVoteDown').classList.remove('highlight');
    }
    else if (vote === 2) {
        document.getElementById('btnVoteUp').classList.add('highlight');
        document.getElementById('btnVoteDown').classList.remove('highlight');
    }
}
