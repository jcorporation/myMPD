"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalSongDetails_js */

/**
 * Initialization function for song elements
 * @returns {void}
 */
function initModalSongDetails() {
    elGetById('modalSongDetailsTagsList').addEventListener('click', function(event) {
        if (event.target.nodeName === 'A') {
            if (event.target.id === 'calcFingerprint') {
                sendAPI("MYMPD_API_SONG_FINGERPRINT", {
                    "uri": getData(event.target, 'uri')
                }, parseFingerprint, true);
                event.preventDefault();
                const spinner = elCreateEmpty('div', {"class": ["spinner-border", "spinner-border-sm"]});
                elHide(event.target);
                event.target.parentNode.appendChild(spinner);
            }
            else if (event.target.classList.contains('external') === true) {
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
                    appGoto('Browse', 'Filesystem', undefined, 0, undefined, getData(event.target, 'folder'), {'tag': '', 'desc': false}, '', '', 0);
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

/**
 * Shows the song details modal
 * @param {string} uri song uri
 * @returns {void}
 */
function songDetails(uri) {
    setUpdateViewId('modalSongDetailsTagsList');
    sendAPI("MYMPD_API_SONG_DETAILS", {
        "uri": uri
    }, parseSongDetails, true);
    uiElements.modalSongDetails.show();
}

/**
 * Parses the MYMPD_API_SONG_FINGERPRINT jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parseFingerprint(obj) {
    if (obj.error) {
        elReplaceChildId('fingerprint',
            elCreateTextTn('div', {"class": ["alert", "alert-danger"]}, obj.error.message, obj.error.data)
        );
        return;
    }
    const textarea = elCreateEmpty('textarea', {"class": ["form-control", "font-monospace", "breakAll"], "rows": 5});
    textarea.value = obj.result.fingerprint;
    elReplaceChildId('fingerprint', textarea);
}

/**
 * Adds a row to the song details modal
 * @param {string} thContent text for th
 * @param {HTMLElement | Node | string} tdContent content element fot td
 * @returns {HTMLElement} created row
 */
function songDetailsRow(thContent, tdContent) {
    const td = elCreateEmpty('td', {});
    if (typeof tdContent === 'object') {
        td.appendChild(tdContent);
    }
    else {
        td.textContent = tdContent;
    }
    const tr = elCreateNodes('tr', {}, [
        elCreateTextTn('th', {}, thContent),
        td
    ]);
    return tr;
}

/**
 * Parses the MYMPD_API_SONG_DETAILS jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parseSongDetails(obj) {
    const modal = elGetById('modalSongDetails');
    modal.querySelector('.album-cover').style.backgroundImage = getCssImageUri('/albumart?offset=0&uri=' + myEncodeURIComponent(obj.result.uri));

    const elH1s = modal.querySelectorAll('h1');
    for (let i = 0, j = elH1s.length; i < j; i++) {
        elH1s[i].textContent = obj.result.Title;
    }
    const table = elGetById('modalSongDetailsTagsList');
    const tbody = table.querySelector('tbody');
    elClear(tbody);
    if (checkResult(obj, table, 'table') === false) {
        return;
    }
    for (let i = 0, j = settings.tagList.length; i < j; i++) {
        if (settings.tagList[i] === 'Title' ||
            isEmptyTag(obj.result[settings.tagList[i]]) === true)
        {
            continue;
        }
        const tr = elCreateEmpty('tr', {});
        tr.appendChild(
            elCreateTextTn('th', {}, settings.tagList[i])
        );
        const td = elCreateEmpty('td', {});
        setData(td, 'tag', settings.tagList[i]);
        setData(td, 'name', obj.result[settings.tagList[i]]);
        if (settings.tagList[i] === 'Album') {
            setData(td, 'AlbumId', obj.result.AlbumId);
        }
        if (settings.tagListBrowse.includes(settings.tagList[i]) &&
            isEmptyTag(obj.result[settings.tagList[i]]) === false)
        {
            if (typeof obj.result[settings.tagList[i]] === 'string') {
                td.appendChild(
                    elCreateText('a', {"class": ["text-success"], "href": "#"}, obj.result[settings.tagList[i]])
                );
            }
            else {
                td.appendChild(
                    elCreateText('a', {"class": ["text-success"], "href": "#"}, obj.result[settings.tagList[i]].join(', '))
                );
            }
        }
        else if (settings.tagList[i].indexOf('MUSICBRAINZ') === 0) {
            td.appendChild(
                printValue(settings.tagList[i], obj.result[settings.tagList[i]])
            );
        }
        else {
            td.textContent = obj.result[settings.tagList[i]];
        }
        tr.appendChild(td);
        tbody.appendChild(tr);
    }
    tbody.appendChild(songDetailsRow('Duration', fmtDuration(obj.result.Duration)));
    //resolves cuesheet virtual tracks
    const rUri = cuesheetUri(obj.result.uri);
    let isCuesheet = false;
    if (rUri !== obj.result.uri) {
        isCuesheet = true;
    }

    const shortName = basename(rUri, false) + (isCuesheet === true ? ' (' + cuesheetTrack(obj.result.uri) + ')' : '');
    const openFolderBtn = elCreateText('button', {"id": "gotoContainingFolder", "class": ["btn", "btn-secondary", "mi"],
        "data-title-phrase": "Open folder"}, 'folder_open');
    setData(openFolderBtn, 'folder', dirname(obj.result.uri));
    let downloadBtn = null;
    if (features.featLibrary === true) {
        downloadBtn = elCreateText('button', {"id": "downloadSong","class": ["btn", "btn-secondary", "mi"],
            "data-title-phrase": "Download"}, 'file_download');
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
    tbody.appendChild(songDetailsRow('Filetype', filetype(rUri, true)));
    tbody.appendChild(songDetailsRow('Last-Modified', printValue('Last-Modified', obj.result['Last-Modified'])));
    if (features.featDbAdded === true) {
        tbody.appendChild(songDetailsRow('Added', printValue('Added', obj.result.Added)));
    }
    //fingerprint command is not supported for cuesheet virtual tracks
    if (features.featFingerprint === true &&
        isCuesheet === false)
    {
        const a = elCreateTextTn('a', {"class": ["text-success"], "id": "calcFingerprint", "href": "#"}, 'Calculate');
        setData(a, 'uri', obj.result.uri);
        tbody.appendChild(songDetailsRow('Fingerprint', a));
        tbody.lastChild.lastChild.setAttribute('id', 'fingerprint');
    }
    if (obj.result.bookletPath !== '') {
        tbody.appendChild(
            songDetailsRow('Booklet',
                elCreateTextTn('a', {"class": ["text-success"],
                    "href": myEncodeURI(subdir + obj.result.bookletPath), "target": "_blank"}, 'Download')
            )
        );
    }
    if (obj.result.infoTxtPath !== '') {
        const infoTxtEl = elCreateTextTn('span', {"class": ["text-success", "clickable"]}, 'Show');
        setData(infoTxtEl, 'uri', obj.result.infoTxtPath);
        infoTxtEl.addEventListener('click', function(event) {
            showInfoTxt(event.target);
        }, false);
        tbody.appendChild(
            songDetailsRow('Album info', infoTxtEl)
        );
    }
    if (features.featStickers === true) {
        tbody.appendChild(
            elCreateNode('tr', {},
                elCreateNode('th', {"colspan": "2", "class": ["pt-3"]},
                    elCreateTextTn('h5', {}, 'Statistics')
                )
            )
        );
        for (const sticker of stickerListSongs) {
            if (sticker === 'like' ||
                sticker === 'rating')
            {
                if (sticker === 'like' &&
                    features.featLike === true)
                {
                    const grp = createLike(obj.result.like, "song");
                    setData(grp, 'href', {"cmd": "voteSongLike", "options": ["target"]});
                    setData(grp, 'uri', obj.result.uri);
                    tbody.appendChild(
                        elCreateNodes('tr', {}, [
                            elCreateTextTn('th', {}, 'Like'),
                            elCreateNode('td', {}, grp)
                        ])
                    );
                }
                else if (sticker === 'rating' &&
                    features.featRating === true)
                {
                    const grp = createStarRating(obj.result.rating, "song");
                    setData(grp, 'href', {"cmd": "voteSongRating", "options": ["target"]});
                    setData(grp, 'uri', obj.result.uri);
                    tbody.appendChild(
                        elCreateNodes('tr', {}, [
                            elCreateTextTn('th', {}, 'Stars'),
                            elCreateNode('td', {}, grp)
                        ])
                    );
                }
            }
            else {
                tbody.appendChild(
                    songDetailsRow(sticker, printValue(sticker, obj.result[sticker]))
                );
            }
        }
    }
    unsetUpdateViewId('modalSongDetailsTagsList');
    //populate other tabs
    if (features.featLyrics === true) {
        elClearId('modalSongDetailsTabLyricsText');
        getLyrics(obj.result.uri, elGetById('modalSongDetailsTabLyricsText'));
    }
    getComments(obj.result.uri);
    const imgEl = elGetById('modalSongDetailsTabPics');
    createImgCarousel(imgEl, 'modalSongDetailsPicsCarousel', obj.result.uri, obj.result.images, obj.result.embeddedImageCount);
}

/**
 * Gets the song comments
 * @param {string} uri song uri
 * @returns {void}
 */
function getComments(uri) {
    setUpdateViewId('modalSongDetailsCommentsList');
    sendAPI("MYMPD_API_SONG_COMMENTS", {
        "uri": uri
    }, parseComments, true);
}

/**
 * Gets the song comments
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parseComments(obj) {
    const table = elGetById('modalSongDetailsCommentsList');
    const tbody = table.querySelector('tbody');
    elClear(tbody);
    if (checkResult(obj, table, 'table') === false) {
        return;
    }
    for (const key in obj.result.data) {
        tbody.appendChild(
            elCreateNodes('tr', {}, [
                elCreateText('td', {}, key),
                elCreateText('td', {}, obj.result.data[key])
            ])
        );
    }
    unsetUpdateView(table);
}
