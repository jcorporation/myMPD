"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function initSong() {
    document.getElementById('tbodySongDetails').addEventListener('click', function(event) {
        if (event.target.nodeName === 'A') {
            if (event.target.id === 'calcFingerprint') {
                sendAPI("MYMPD_API_DATABASE_FINGERPRINT", {"uri": getData(event.target, 'data-uri')}, parseFingerprint);
                event.preventDefault();
                const spinner = elCreateEmpty('div', {"class": ["spinner-border", "spinner-border-sm"]});
                elHide(event.target);
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
            const cmd = getData(event.target, 'data-href');
            if (cmd !== undefined) {
                parseCmd(event, cmd);
            }
        }
    }, false);
}

function songDetails(uri) {
    sendAPI("MYMPD_API_DATABASE_SONGDETAILS", {
        "uri": uri
    }, parseSongDetails);
    uiElements.modalSongDetails.show();
}

function parseFingerprint(obj) {
    const textarea = elCreateEmpty('textarea', {"class": ["form-control", "text-monospace", "small", "breakAll"], "rows": 5});
    textarea.value = obj.result.fingerprint;
    elReplaceChild(document.getElementById('fingerprint'), textarea);
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
    if (MBentity === '' || value === '-') {
        return elCreateText('span', {}, value);
    }
    else {
        return elCreateText('a', {"title": tn('Lookup at musicbrainz'), "class": ["text-success", "external"], "target": "_musicbrainz",
            "href": "https://musicbrainz.org/" + MBentity + "/" + myEncodeURI(value)}, value);
    }
}

function songDetailsRow(thContent, tdContent) {
    const td = elCreateEmpty('td', {});
    if (typeof tdContent === 'object') {
        td.appendChild(tdContent);
    }
    else {
        td.textContent = tdContent;
    }
    const tr = elCreateNodes('tr', {}, [
        elCreateText('th', {}, tn(thContent)),
        td
    ]);
    return tr;
}

function parseSongDetails(obj) {
    const modal = document.getElementById('modalSongDetails');
    modal.getElementsByClassName('album-cover')[0].style.backgroundImage = 'url("' + 
        subdir + '/albumart/' + myEncodeURI(obj.result.uri) + '"), url("' + subdir + '/assets/coverimage-loading.svg")';

    const elH1s = modal.getElementsByTagName('h1');
    for (let i = 0, j = elH1s.length; i < j; i++) {
        elH1s[i].textContent = obj.result.Title;
    }
    const tbody = document.getElementById('tbodySongDetails');
    elClear(tbody);
    for (let i = 0, j = settings.tagList.length; i < j; i++) {
        if (settings.tagList[i] === 'Title' || obj.result[settings.tagList[i]] === '-') {
            continue;
        }
        const tr = elCreateEmpty('tr', {});
        tr.appendChild(elCreateText('th', {}, tn(settings.tagList[i])));
        const td = elCreateEmpty('td', {});
        setData(td, 'data-tag', settings.tagList[i]);
        setData(td, 'data-name', obj.result[settings.tagList[i]]);
        if (settings.tagList[i] === 'Album' && obj.result[tagAlbumArtist] !== null) {
            setData(td, 'data-albumartist', obj.result[tagAlbumArtist]);
        }
        if (settings.tagListBrowse.includes(settings.tagList[i]) && obj.result[settings.tagList[i]] !== '-') {
            td.appendChild(elCreateText('a', {"class": ["text-success"], "href": "#"}, obj.result[settings.tagList[i]]));
        }
        else if (settings.tagList[i].indexOf('MUSICBRAINZ') === 0) {
            td.appendChild(getMBtagLink(settings.tagList[i], obj.result[settings.tagList[i]]));
        }
        else {
            td.textContent = obj.result[settings.tagList[i]];
        }
        tr.appendChild(td);
        tbody.appendChild(tr);
    }
    tbody.appendChild(songDetailsRow('Duration', beautifyDuration(obj.result.Duration)));
    if (features.featLibrary === true) {
        tbody.appendChild(songDetailsRow('Filename', elCreateText('a', {"class": ["text-break", "text-success", "downdload"], "href": "#", 
            "target": "_blank", "title": tn(obj.result.uri)}, basename(obj.result.uri, false))));
    }
    else {
        tbody.appendChild(songDetailsRow('Filename', elCreateText('span', {"class": ["text-break"], "title": tn(obj.result.uri)},
            basename(obj.result.uri, false))));
    }
    tbody.appendChild(songDetailsRow('AudioFormat', printValue('AudioFormat', obj.result.AudioFormat)));
    tbody.appendChild(songDetailsRow('Filetype', filetype(obj.result.uri)));
    tbody.appendChild(songDetailsRow('LastModified', localeDate(obj.result.LastModified)));
    if (features.featFingerprint === true) {
        const a = elCreateText('a', {"class": "text-success", "id": "calcFingerprint", "href": "#"}, tn('Calculate'));
        setData(a, 'data-uri', obj.result.uri);
        tbody.appendChild(songDetailsRow('Fingerprint', a));
        tbody.lastChild.lastChild.setAttribute('id', 'fingerprint');
    }
    if (obj.result.bookletPath !== '') {
        tbody.appendChild(songDetailsRow('Booklet', elCreateText('a', {"class": ["text-success"], 
            "href": myEncodeURI(subdir + '/browse/music/' + dirname(obj.result.uri) + '/' + settings.bookletName), "target": "_blank"},
            tn('Download'))));
    }
    if (features.featStickers === true) {
        tbody.appendChild(elCreateNode('tr', {}, elCreateNode('th', {"colspan": "2", "class": ["pt-3"]}, elCreateText('h5', {}, tn('Statistics')))));
        for (const sticker of stickerList) {
            if (sticker === 'stickerLike') {
                const thDown = elCreateText('button', {"title": tn('Dislike song'), "class": ["btn", "btn-sm", "btn-secondary", "mi"]}, 'thumb_down');
                setData(thDown, 'data-href', {"cmd": "voteSong", "options": [0]});
                if (obj.result[sticker] === 0) {
                    thDown.classList.add('active');
                }
                const thUp = elCreateText('button', {"title": tn('Dislike song'), "class": ["btn", "btn-sm", "btn-secondary", "mi"]}, 'thumb_up');
                setData(thUp, 'data-href', {"cmd": "voteSong", "options": [2]});
                if (obj.result[sticker] === 2) {
                    thUp.classList.add('active');
                }
                const grp = elCreateNodes('div', {"class": ["btn-group", "btn-group-sm"]},[
                    thDown,
                    thUp
                ]);
                setData(grp, 'data-uri', obj.result.uri);
                tbody.appendChild(
                    elCreateNodes('tr', {}, [
                        elCreateText('th', {}, tn('Like')),
                        elCreateNode('td', {}, grp)
                    ])
                );
            }
            else {
                tbody.appendChild(songDetailsRow(sticker, printValue(sticker, obj.result[sticker])));
            }
        }
    }

    if (features.featLyrics === true) {
        getLyrics(obj.result.uri, document.getElementById('lyricsText'));
    }

    getComments(obj.result.uri, document.getElementById('tbodySongComments'));

    const pictureEls = document.getElementsByClassName('featPictures');
    for (let i = 0, j = pictureEls.length; i < j; i++) {
        elShow(pictureEls[i]);
    }

    //add uri to image list to get embedded albumart
    const images = [ subdir + '/albumart/' + obj.result.uri ];
    //add all but coverfiles to image list
    for (let i = 0, j = obj.result.images.length; i < j; i++) {
        if (isCoverfile(obj.result.images[i]) === false) {
            images.push(subdir + '/browse/music/' + obj.result.images[i]);
        }
    }
    const imgEl = document.getElementById('tabSongPics');
    createImgCarousel(imgEl, 'songPicsCarousel', images);
}

function isCoverfile(uri) {
    const filename = basename(uri).toLowerCase();
    const fileparts = filename.split('.');

    const extensions = ['png', 'jpg', 'jpeg', 'svg', 'webp', 'avif'];
    const coverimageNames = settings.coverimageNames.split(',');
    for (let i = 0, j = coverimageNames.length; i < j; i++) {
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

function getComments(uri, el) {
    el.classList.add('opacity05');
    sendAPI("MYMPD_API_DATABASE_COMMENTS", {
        "uri": uri
    }, function(obj) {
        elClear(el);
        if (obj.result.returnedEntities === 0) {
            el.appendChild(emptyRow(2));
            el.classList.remove('opacity05');
            return false;
        }
        for (const key in obj.result.data) {
            el.appendChild(
                elCreateNodes('tr', {}, [
                    elCreateText('td', {}, key),
                    elCreateText('td', {}, obj.result.data[key])
                ])
            );
        }
        el.classList.remove('opacity05');
    }, false);
}

function getLyrics(uri, el) {
    if (isValidUri(uri) === false || isStreamUri(uri) === true) {
        el.textContent = tn('No lyrics found');
        return;
    }
    el.classList.add('opacity05');
    sendAPI("MYMPD_API_LYRICS_GET", {
        "uri": uri
    }, function(obj) {
        if (obj.error) {
            el.textContent = tn(obj.error.message);
        }
        else if (obj.result.message) {
            el.textContent = tn(obj.result.message);
        }
        else if (obj.result.returnedEntities === 0) {
            el.textContent = tn('No lyrics found');
        }
        else {
            const lyricsTabs = elCreateEmpty('div', {"class": [ "lyricsTabs"]});
            const lyrics = elCreateEmpty('div', {"class": ["lyricsTextContainer", "mt-3"]});
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
                lyricsTabs.appendChild(elCreateText('button', {"data-num": i, "class": ["btn", "btn-sm", "btn-outline-secondary", "me-2", "lyricsChangeButton"],
                    "title": (obj.result.data[i].synced === true ? tn('Synced lyrics') : tn('Unsynced lyrics')) + ': ' + ht}, ht));
                if (i === 0) {
                    lyricsTabs.lastChild.classList.add('active');
                }

                const div = elCreateEmpty('div', {"class": ["lyricsText"]});
                if (i > 0) {
                    div.classList.add('d-none');
                }
                if (obj.result.data[i].synced === true) {
                    div.classList.add('lyricsSyncedText');
                }
                if (clickable === false) {
                    div.classList.add('fullHeight');
                }
                if (obj.result.data[i].synced === true) {
                    parseSyncedLyrics(div, obj.result.data[i].text, clickable);
                }
                else {
                    parseUnsyncedLyrics(div, obj.result.data[i].text);
                }
                lyrics.appendChild(div);

                if (obj.result.data[i].synced === true) {
                    showSyncedLyrics = true;
                }
            }
            const lyricsScroll = elCreateNode('button', {"title": tn('Toggle autoscrolling'), "class": ["btn", "btn-sm", "me-2", "active", "d-none"], "id": "lyricsScroll"}, 
                elCreateText('span', {"class": ["mi", "mi-small"]}, 'autorenew')
            );
            const lyricsResize = elCreateNode('button', {"title": tn('Resize'), "class": ["btn", "btn-sm", "me-2", "active", "d-none"], "id": "lyricsResize"}, 
                elCreateText('span', {"class": ["mi", "mi-small"]}, 'aspect_ratio')
            );
            const lyricsHeader = elCreateEmpty('div', {"class": ["lyricsHeader", "btn-toolbar", "mt-2"]});
            lyricsHeader.appendChild(lyricsScroll);
            lyricsHeader.appendChild(lyricsResize);
            elClear(el);
            if (obj.result.returnedEntities > 1) {
                lyricsHeader.appendChild(lyricsTabs);
                el.appendChild(lyricsHeader);
                el.appendChild(lyrics);
                el.getElementsByClassName('lyricsTabs')[0].addEventListener('click', function(event) {
                    if (event.target.nodeName === 'BUTTON') {
                        event.target.parentNode.getElementsByClassName('active')[0].classList.remove('active');
                        event.target.classList.add('active');
                        const nr = Number(event.target.getAttribute('data-num'));
                        const tEls = el.getElementsByClassName('lyricsText');
                        for (let i = 0, j = tEls.length; i < j; i++) {
                            if (i === nr) {
                                elShow(tEls[i]);
                            }
                            else {
                                elHide(tEls[i]);
                            }
                        }
                    }
                }, false);
            }
            else {
                el.appendChild(lyricsHeader);
                el.appendChild(lyrics);
            }
            const ls = document.getElementById('lyricsScroll');
            if (ls !== null && showSyncedLyrics === true && clickable === true) {
                elShow(ls);
                ls.addEventListener('click', function(event) {
                    const target = event.target.nodeName === 'SPAN' ? event.target.parentNode : event.target;
                    toggleBtn(target);
                    scrollSyncedLyrics = target.classList.contains('active');
                }, false);
                const textEls = el.getElementsByClassName('lyricsSyncedText');
                for (let i = 0, j = textEls.length; i < j; i++) {
                    //seek to songpos in click
                    textEls[i].addEventListener('click', function(event) {
                        const sec = event.target.getAttribute('data-sec');
                        if (sec !== null) {
                            sendAPI("MYMPD_API_PLAYER_SEEK_CURRENT", {
                                "seek": Number(sec),
                                "relative": false
                            });
                        }
                    }, false); 
                }
            }
            const lr = document.getElementById('lyricsResize');
            if (lr !== null) {
                elShow(lr);
                lr.addEventListener('click', function(event) {
                    const target = event.target.nodeName === 'SPAN' ? event.target.parentNode : event.target;
                    toggleBtn(target);
                    const mh = target.classList.contains('active') ? '16rem' : 'unset';
                    const lt = document.getElementsByClassName('lyricsText');
                    for (const l of lt) {
                        l.style.maxHeight = mh;
                    }
                }, false);
            }
        }
        el.classList.remove('opacity05');
    }, true);
}

function parseUnsyncedLyrics(parent, text) {
    for (const line of text.replace('\r').split('\n')) {
        parent.appendChild(document.createTextNode(line));
        parent.appendChild(elCreateEmpty('br', {}));
    }
}

function parseSyncedLyrics(parent, lyrics, clickable) {
    for (const line of lyrics.replace('\r').split('\n')) {
        //line must start with timestamp
        const elements = line.match(/^\[(\d+):(\d+)\.(\d+)\](.*)$/);
        if (elements) {
            //elements[3] are hundreths of a seconde - ignore it for the moment
            const ts = [Number(elements[1]) * 60 + Number(elements[2])];
            const text = [];
            //support of timestamps per word
            const regex = /(.+)(<(\d+):(\d+)\.\d+>)?/g;
            let match;
            while ((match = regex.exec(elements[4])) !== null) {
                text.push(match[1]);
                if (regex[2] !== undefined) {
                    ts.push(Number(match[3]) * 60 + Number(match[4]));
                }
            }
            if (text.length === 0) {
                //handle empty lines
                text.push(' ');
            }
            const p = elCreateEmpty('p', {});
            for (let i = 0, j = ts.length; i < j; i++) {
                const span = elCreateText('span', {"data-sec": ts[i]}, text[i]);
                if (clickable === true) {
                    span.classList.add('clickable');
                }
                p.appendChild(span);
            }
            parent.appendChild(p);
        }
    }
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
    const uri = getData(el.parentNode, 'data-uri');
    sendAPI("MYMPD_API_LIKE", {
        "uri": uri,
        "like": vote
    });
}

//eslint-disable-next-line no-unused-vars
function voteCurrentSong(vote) {
    const uri = getDataId('currentTitle', 'data-uri');
    if (uri === '') {
        return;
    }

    if ((vote === 2 && document.getElementById('btnVoteUp').classList.contains('highlight')) ||
        (vote === 0 && document.getElementById('btnVoteDown').classList.contains('highlight')))
    {
        vote = 1;
    }

    sendAPI("MYMPD_API_LIKE", {
        "uri": uri,
        "like": vote
    });
    setVoteSongBtns(vote, uri);
}

function setVoteSongBtns(vote, uri) {
    if (uri === undefined) {
        uri = '';
    }

    if (isValidUri(uri) === false || isStreamUri(uri) === true) {
        elDisableId('btnVoteUp');
        elDisableId('btnVoteDown');
        document.getElementById('btnVoteUp').classList.remove('highlight');
        document.getElementById('btnVoteDown').classList.remove('highlight');
    }
    else {
        elEnableId('btnVoteUp');
        elEnableId('btnVoteDown');
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
