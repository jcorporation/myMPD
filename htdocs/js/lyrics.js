"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module lyrics_js */

/**
 * Gets the lyrics
 * @param {string} uri song uri
 * @param {HTMLElement} el container element to show the lyrics
 * @returns {void}
 */
function getLyrics(uri, el) {
    if (isValidUri(uri) === false ||
        isStreamUri(uri) === true)
    {
        el.textContent = tn('No lyrics found');
        return;
    }
    setUpdateView(el);
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
        unsetUpdateView(el);
    }, true);
}

/**
 * Parses the MYMPD_API_LYRICS_GET jsonrpc response
 * @param {HTMLElement} el container element to show the lyrics
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function createLyricsTabs(el, obj) {
    const lyricsTabs = elCreateEmpty('div', {"class": [ "lyricsTabs", "btn-toolbar"]});
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
        lyricsTabs.appendChild(
            elCreateText('button', {"data-num": i, "class": ["btn", "btn-sm", "btn-outline-secondary", "me-2", "lyricsChangeButton", "text-truncate"],
                "title": (obj.result.data[i].synced === true ? tn('Synced lyrics') : tn('Unsynced lyrics')) + ': ' + ht}, ht)
        );
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
            elCreateText('button', {"data-title-phrase": "Toggle autoscrolling", "class": ["btn", "btn-sm", "me-2", "active", "d-none", "mi"], "id": "lyricsScroll"}, 'autorenew')
        );
        lyricsHeader.appendChild(
            elCreateText('button', {"data-title-phrase": "Resize", "class": ["btn", "btn-sm", "me-2", "active", "mi"], "id": "lyricsResize"}, 'aspect_ratio')
        );
    }
    elClear(el);
    if (obj.result.returnedEntities > 1) {
        //more then one result - show tabs
        lyricsHeader.appendChild(lyricsTabs);
        el.appendChild(lyricsHeader);
        el.appendChild(lyrics);
        el.querySelector('.lyricsTabs').addEventListener('click', function(event) {
            if (event.target.nodeName === 'BUTTON') {
                event.target.parentNode.querySelector('.active').classList.remove('active');
                event.target.classList.add('active');
                const nr = Number(event.target.getAttribute('data-num'));
                const tEls = el.querySelectorAll('.lyricsText');
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
            const ls = elGetById('lyricsScroll');
            if (ls !== null) {
                //synced lyrics scrolling button
                elShow(ls);
                ls.addEventListener('click', function(event) {
                    toggleBtn(event.target, undefined);
                    scrollSyncedLyrics = event.target.classList.contains('active');
                }, false);
                //seek to position on click
                const textEls = el.querySelectorAll('.lyricsSyncedText');
                for (let i = 0, j = textEls.length; i < j; i++) {
                    textEls[i].addEventListener('click', function(event) {
                        const sec = event.target.getAttribute('data-sec');
                        if (sec !== null) {
                            sendAPI("MYMPD_API_PLAYER_SEEK_CURRENT", {
                                "seek": Number(sec),
                                "relative": false
                            }, null, false);
                        }
                    }, false);
                }
            }
        }
        //resize button
        const lr = elGetById('lyricsResize');
        if (lr !== null) {
            lr.addEventListener('click', function(event) {
                toggleBtn(event.target, undefined);
                const mh = event.target.classList.contains('active') ? '16rem' : 'unset';
                const lt = document.querySelectorAll('.lyricsText');
                for (const l of lt) {
                    l.style.maxHeight = mh;
                }
            }, false);
        }
    }
}

/**
 * Parses unsynced lyrics
 * @param {HTMLElement} parent element to append the lyrics
 * @param {string} text the lyrics
 * @returns {void}
 */
function parseUnsyncedLyrics(parent, text) {
    for (const line of text.replace(/\r/g, '').split('\n')) {
        parent.appendChild(
            document.createTextNode(line)
        );
        parent.appendChild(
            elCreateEmpty('br', {})
        );
    }
}

/**
 * Parses synced lyrics (lrc format)
 * @param {HTMLElement} parent element to append the lyrics
 * @param {string} lyrics the lyrics
 * @param {boolean} currentLyrics true = lyrics in playback view, false lyrics in song details modal
 * @returns {void}
 */
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
                const span = elCreateText('span', {"data-sec": ts[i], "title": fmtSongDuration(ts[i])}, text[i]);
                if (currentLyrics === true) {
                    span.classList.add('clickable');
                }
                p.appendChild(span);
            }
            parent.appendChild(p);
        }
    }
}
