"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function initSong() {
    document.getElementById('tbodySongDetails').addEventListener('click', function(event) {
        if (event.target.nodeName === 'A') {
            if (event.target.id === 'calcFingerprint') {
                sendAPI("MYMPD_API_DATABASE_FINGERPRINT", {
                    "uri": getData(event.target, 'uri')
                }, parseFingerprint, true);
                event.preventDefault();
                const spinner = elCreateEmpty('div', {"class": ["spinner-border", "spinner-border-sm"]});
                elHide(event.target);
                event.target.parentNode.appendChild(spinner);
            }
            else if (event.target.classList.contains('external') === true)
            {
                //do nothing, link opens in new browser window
            }
            else if (getData(event.target.parentNode, 'tag') !== undefined) {
                uiElements.modalSongDetails.hide();
                event.preventDefault();
                gotoBrowse(event);
            }
        }
        else if (event.target.nodeName === 'BUTTON') {
            switch(event.target.id) {
                case 'gotoContainingFolder': {
                    uiElements.modalSongDetails.hide();
                    event.preventDefault();
                    appGoto('Browse', 'Filesystem', undefined, 0, undefined, '-', '-', '-', getData(event.target, 'folder'), 0);
                    break;
                }
                case 'downloadSong': {
                    const href = getData(event.target, 'href');
                    window.open(href, '_blank');
                    break;
                }
                default: {
                    //song vote buttons
                    const cmd = getData(event.target.parentNode, 'href');
                    if (cmd !== undefined) {
                        parseCmd(event, cmd);
                    }
                }
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
    if (obj.error) {
        elReplaceChildId('fingerprint',
            elCreateText('div', {"class": ["alert", "alert-danger"]}, tn(obj.error.message, obj.error.data))
        );
        return;
    }
    const textarea = elCreateEmpty('textarea', {"class": ["form-control", "text-monospace", "small", "breakAll"], "rows": 5});
    textarea.value = obj.result.fingerprint;
    elReplaceChildId('fingerprint', textarea);
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
    if (MBentity === '' ||
        value === '-')
    {
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
        subdir + '/albumart?offset=0&uri=' + myEncodeURIComponent(obj.result.uri) + '"), url("' + subdir + '/assets/coverimage-loading.svg")';

    const elH1s = modal.getElementsByTagName('h1');
    for (let i = 0, j = elH1s.length; i < j; i++) {
        elH1s[i].textContent = obj.result.Title;
    }
    const tbody = document.getElementById('tbodySongDetails');
    elClear(tbody);
    for (let i = 0, j = settings.tagList.length; i < j; i++) {
        if (settings.tagList[i] === 'Title' ||
            obj.result[settings.tagList[i]] === '-')
        {
            continue;
        }
        const tr = elCreateEmpty('tr', {});
        tr.appendChild(elCreateText('th', {}, tn(settings.tagList[i])));
        const td = elCreateEmpty('td', {});
        setData(td, 'tag', settings.tagList[i]);
        setData(td, 'name', obj.result[settings.tagList[i]]);
        if (settings.tagList[i] === 'Album' && obj.result[tagAlbumArtist] !== null) {
            setData(td, 'AlbumArtist', obj.result[tagAlbumArtist]);
        }
        if (settings.tagListBrowse.includes(settings.tagList[i]) &&
            checkTagValue(obj.result[settings.tagList[i]], '-') === false)
        {
            if (typeof obj.result[settings.tagList[i]] === 'string') {
                td.appendChild(elCreateText('a', {"class": ["text-success"], "href": "#"}, obj.result[settings.tagList[i]]));
            }
            else {
                td.appendChild(elCreateText('a', {"class": ["text-success"], "href": "#"}, obj.result[settings.tagList[i]].join(', ')));
            }
        }
        else if (settings.tagList[i].indexOf('MUSICBRAINZ') === 0) {
            td.appendChild(printValue(settings.tagList[i], obj.result[settings.tagList[i]]));
        }
        else {
            td.textContent = obj.result[settings.tagList[i]];
        }
        tr.appendChild(td);
        tbody.appendChild(tr);
    }
    tbody.appendChild(songDetailsRow('Duration', beautifyDuration(obj.result.Duration)));
    //resolves cuesheet virtual tracks
    const rUri = cuesheetUri(obj.result.uri);
    let isCuesheet = false;
    if (rUri !== obj.result.uri) {
        isCuesheet = true;
    }

    const shortName = basename(rUri, false) + (isCuesheet === true ? ' (' + cuesheetTrack(obj.result.uri) + ')' : '');
    const openFolderBtn = elCreateText('button', {"id": "gotoContainingFolder", "class": ["btn", "btn-secondary", "mi"],
        "title": tn("Open folder")}, 'folder_open');
    setData(openFolderBtn, 'folder', dirname(obj.result.uri));
    let downloadBtn = null;
    if (features.featLibrary === true) {
        downloadBtn = elCreateText('button', {"id": "downloadSong","class": ["btn", "btn-secondary", "mi"],
            "title": tn("Download")}, 'file_download');
        setData(downloadBtn, 'href', myEncodeURI(subdir + '/browse/music/' + rUri));
    }
    tbody.appendChild(
        songDetailsRow('Filename',
            elCreateNodes('div', {}, [
                elCreateText('p', {"class": ["text-break", "mb-1"], "title": rUri}, shortName),
                elCreateNodes('div', {"class": ["input-group", "mb-1"]}, [
                    elCreateEmpty('input', {"class": ["form-control"], "value": rUri}),
                    openFolderBtn,
                    downloadBtn
                ])
            ])
        )
    );

    tbody.appendChild(songDetailsRow('AudioFormat', printValue('AudioFormat', obj.result.AudioFormat)));
    tbody.appendChild(songDetailsRow('Filetype', filetype(rUri)));
    tbody.appendChild(songDetailsRow('LastModified', localeDate(obj.result.LastModified)));
    //fingerprint command is not supported for cuesheet virtual tracks
    if (features.featFingerprint === true &&
        isCuesheet === false)
    {
        const a = elCreateText('a', {"class": ["text-success"], "id": "calcFingerprint", "href": "#"}, tn('Calculate'));
        setData(a, 'uri', obj.result.uri);
        tbody.appendChild(songDetailsRow('Fingerprint', a));
        tbody.lastChild.lastChild.setAttribute('id', 'fingerprint');
    }
    if (obj.result.bookletPath !== '') {
        tbody.appendChild(songDetailsRow('Booklet', elCreateText('a', {"class": ["text-success"],
            "href": myEncodeURI(subdir + obj.result.bookletPath), "target": "_blank"},
            tn('Download'))));
    }
    if (features.featStickers === true) {
        tbody.appendChild(
            elCreateNode('tr', {},
                elCreateNode('th', {"colspan": "2", "class": ["pt-3"]},
                    elCreateText('h5', {}, tn('Statistics'))
                )
            )
        );
        for (const sticker of stickerList) {
            if (sticker === 'stickerLike') {
                const thDown = elCreateText('button', {"data-vote": "0", "title": tn('Hate song'), "class": ["btn", "btn-sm", "btn-secondary", "mi"]}, 'thumb_down');
                if (obj.result[sticker] === 0) {
                    thDown.classList.add('active');
                }
                const thUp = elCreateText('button', {"data-vote": "2", "title": tn('Love song'), "class": ["btn", "btn-sm", "btn-secondary", "mi"]}, 'thumb_up');
                if (obj.result[sticker] === 2) {
                    thUp.classList.add('active');
                }
                const grp = elCreateNodes('div', {"class": ["btn-group", "btn-group-sm"]}, [
                    thDown,
                    thUp
                ]);
                setData(grp, 'href', {"cmd": "voteSong", "options": []});
                setData(grp, 'uri', obj.result.uri);
                tbody.appendChild(
                    elCreateNodes('tr', {}, [
                        elCreateText('th', {}, tn('Like')),
                        elCreateNode('td', {}, grp)
                    ])
                );
            }
            else {
                tbody.appendChild(
                    songDetailsRow(sticker, printValue(sticker, obj.result[sticker]))
                );
            }
        }
    }
    //populate other tabs
    if (features.featLyrics === true) {
        getLyrics(obj.result.uri, document.getElementById('lyricsText'));
    }
    getComments(obj.result.uri, document.getElementById('tbodySongComments'));
    const imgEl = document.getElementById('tabSongPics');
    createImgCarousel(imgEl, 'songPicsCarousel', obj.result.uri, obj.result.images, obj.result.embeddedImageCount);
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
    if (isValidUri(uri) === false ||
        isStreamUri(uri) === true)
    {
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
            createLyricsTabs(el, obj);
        }
        el.classList.remove('opacity05');
    }, true);
}

function createLyricsTabs(el, obj) {
    const lyricsTabs = elCreateEmpty('div', {"class": [ "lyricsTabs"]});
    const lyrics = elCreateEmpty('div', {"class": ["lyricsTextContainer", "mt-3"]});
    const currentLyrics = el.parentNode.getAttribute('id') === 'currentLyrics' ? true : false;
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
        if (currentLyrics === false) {
            //full height for lyrics in song details modal
            div.classList.add('fullHeight');
        }
        if (obj.result.data[i].synced === true) {
            div.classList.add('lyricsSyncedText');
            parseSyncedLyrics(div, obj.result.data[i].text, currentLyrics);
        }
        else {
            parseUnsyncedLyrics(div, obj.result.data[i].text);
        }
        lyrics.appendChild(div);

        if (obj.result.data[i].synced === true) {
            showSyncedLyrics = true;
        }
    }
    const lyricsHeader = elCreateEmpty('div', {"class": ["lyricsHeader", "btn-toolbar", "mt-2"]});
    if (currentLyrics === true) {
        //buttons for lyris in playback view
        lyricsHeader.appendChild(
            elCreateNode('button', {"title": tn('Toggle autoscrolling'), "class": ["btn", "btn-sm", "me-2", "active", "d-none"], "id": "lyricsScroll"},
                elCreateText('span', {"class": ["mi", "mi-small"]}, 'autorenew')
            )
        );
        lyricsHeader.appendChild(
            elCreateNode('button', {"title": tn('Resize'), "class": ["btn", "btn-sm", "me-2", "active"], "id": "lyricsResize"},
                elCreateText('span', {"class": ["mi", "mi-small"]}, 'aspect_ratio')
            )
        );
    }
    elClear(el);
    if (obj.result.returnedEntities > 1) {
        //more then one result - show tabs
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
    if (currentLyrics === true) {
        if (showSyncedLyrics === true) {
            const ls = document.getElementById('lyricsScroll');
            if (ls !== null) {
                //synced lyrics scrolling button
                elShow(ls);
                ls.addEventListener('click', function(event) {
                    const target = event.target.nodeName === 'SPAN' ? event.target.parentNode : event.target;
                    toggleBtn(target);
                    scrollSyncedLyrics = target.classList.contains('active');
                }, false);
                //seek to songpos on click
                const textEls = el.getElementsByClassName('lyricsSyncedText');
                for (let i = 0, j = textEls.length; i < j; i++) {
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
        }
        //resize button
        const lr = document.getElementById('lyricsResize');
        if (lr !== null) {
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
}

function parseUnsyncedLyrics(parent, text) {
    for (const line of text.replace(/\r/g, '').split('\n')) {
        parent.appendChild(document.createTextNode(line));
        parent.appendChild(elCreateEmpty('br', {}));
    }
}

function parseSyncedLyrics(parent, lyrics, currentLyrics) {
    for (const line of lyrics.replace(/\r/g, '').split('\n')) {
        //line must start with timestamp
        const elements = line.match(/^\[(\d+):(\d+)\.\d+\](.*)$/);
        if (elements) {
            const ts = [Number(elements[1]) * 60 + Number(elements[2])];
            const text = [];
            //support of timestamps per word
            const words = elements[3].split(/(<\d+:\d+\.\d+>)/);
            for (const word of words) {
                let timestamp;
                if ((timestamp = word.match(/^<(\d+):(\d+)\.\d+>$/)) !== null) {
                    ts.push(Number(timestamp[1]) * 60 + Number(timestamp[2]));
                }
                else {
                    text.push(word);
                }
            }
            if (text.length === 0) {
                //handle empty lines
                text.push(' ');
            }
            const p = elCreateEmpty('p', {});
            for (let i = 0, j = ts.length; i < j; i++) {
                const span = elCreateText('span', {"data-sec": ts[i], "title": beautifySongDuration(ts[i])}, text[i]);
                if (currentLyrics === true) {
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
function voteSong(el) {
    if (el.nodeName === 'SPAN') {
        el = el.parentNode;
    }
    if (el.nodeName === 'DIV') {
        return;
    }
    let vote = Number(el.getAttribute('data-vote'));
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
    if (vote === 0 ||
        vote === 2)
    {
        el.classList.add('active');
    }
    let uri = getData(el.parentNode, 'uri');
    if (uri === undefined) {
        //fallback to current song
        uri = getDataId('currentTitle', 'uri');
    }
    sendAPI("MYMPD_API_LIKE", {
        "uri": uri,
        "like": vote
    });
}

function setVoteSongBtns(vote, uri) {
    if (uri === undefined) {
        uri = '';
    }

    const btnVoteUp = document.getElementById('btnVoteUp');
    const btnVoteDown = document.getElementById('btnVoteDown');

    if (isValidUri(uri) === false ||
        isStreamUri(uri) === true)
    {
        elDisable(btnVoteUp);
        elDisable(btnVoteDown);
        elDisable(btnVoteUp.parentNode);
        btnVoteUp.classList.remove('active');
        btnVoteDown.classList.remove('active');
    }
    else {
        elEnable(btnVoteUp);
        elEnable(btnVoteDown);
        elEnable(btnVoteUp.parentNode);
    }

    switch(vote) {
        case 0:
            btnVoteUp.classList.remove('active');
            btnVoteDown.classList.add('active');
            break;
        case 2:
            btnVoteUp.classList.add('active');
            btnVoteDown.classList.remove('active');
            break;
        default:
            btnVoteUp.classList.remove('active');
            btnVoteDown.classList.remove('active');
            break;
    }
}
