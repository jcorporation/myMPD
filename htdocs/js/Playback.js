"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module Playback_js */

/**
 * Handles Playback
 * @returns {void}
 */
function handlePlayback() {
    sendAPI("MYMPD_API_PLAYER_CURRENT_SONG", {}, parseCurrentSong, false);
}

/**
 * Initializes the playback html elements
 * @returns {void}
 */
 function initPlayback() {
    document.getElementById('PlaybackColsDropdown').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON' &&
            event.target.classList.contains('mi'))
        {
            event.stopPropagation();
            event.preventDefault();
            toggleBtnChk(event.target, undefined);
        }
    }, false);

    document.getElementById('cardPlaybackTags').addEventListener('click', function(event) {
        if (event.target.nodeName === 'P' ||
            event.target.nodeName === 'SPAN')
        {
            gotoBrowse(event);
        }
    }, false);
}

/**
 * Parses the MYMPD_API_PLAYER_CURRENT_SONG jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parseCurrentSong(obj) {
    const list = document.getElementById('PlaybackList');
    unsetUpdateView(list);

    const textNotification = [];
    const pageTitle = [];

    if (obj.result.Title !== undefined &&
        obj.result.Title !== '-')
    {
        textNotification.push(obj.result.Title);
    }

    mediaSessionSetMetadata(obj.result.Title, obj.result.Artist, obj.result.Album, obj.result.uri);

    setCurrentCover(obj.result.uri);

    for (const elName of ['footerArtist', 'footerAlbum', 'footerCover', 'currentTitle']) {
        document.getElementById(elName).classList.remove('clickable');
    }

    const footerArtistEl = document.getElementById('footerArtist');
    if (obj.result.Artist !== undefined &&
        obj.result.Artist[0] !== '-')
    {
        const artists = joinArray(obj.result.Artist);
        textNotification.push(artists);
        pageTitle.push(artists);
        footerArtistEl.textContent = artists;
        setData(footerArtistEl, 'name', obj.result.Artist);
        footerArtistEl.classList.add('clickable');
    }
    else {
        elClear(footerArtistEl);
        setData(footerArtistEl, 'name', ['']);
        footerArtistEl.classList.remove('clickable');
    }

    const footerDividerEl = document.getElementById('footerDivider');
    const footerAlbumEl = document.getElementById('footerAlbum');
    if (obj.result.Album !== undefined &&
        obj.result.Album !== '-')
    {
        textNotification.push(obj.result.Album);
        footerAlbumEl.textContent = obj.result.Album;
        setData(footerAlbumEl, 'name', obj.result.Album);
        setData(footerAlbumEl, 'AlbumId', obj.result.AlbumId);
        footerAlbumEl.classList.add('clickable');
        footerAlbumEl.setAttribute('data-tag', 'Album');
        footerDividerEl.classList.remove('d-none');
    }
    else if (obj.result.Album === '-' &&
             obj.result.Name &&
             obj.result.Name !== '-')
    {
        footerAlbumEl.textContent = obj.result.Name;
        footerAlbumEl.classList.remove('clickable');
        footerAlbumEl.setAttribute('data-tag', 'undefined');
        footerDividerEl.classList.add('d-none');
    }
    else {
        elClear(footerAlbumEl);
        setData(footerAlbumEl, 'name', '');
        setData(footerAlbumEl, 'AlbumId', '');
        footerAlbumEl.setAttribute('data-tag', 'undefined');
        footerAlbumEl.classList.remove('clickable');
        footerDividerEl.classList.add('d-none');
    }

    const footerTitleEl = document.getElementById('footerTitle');
    const footerCoverEl = document.getElementById('footerCover');
    const currentTitleEl = document.getElementById('currentTitle');
    if (obj.result.Title !== undefined &&
        obj.result.Title !== '-')
    {
        pageTitle.push(obj.result.Title);
        currentTitleEl.textContent = obj.result.Title;
        setData(currentTitleEl, 'uri', obj.result.uri);
        footerTitleEl.textContent = obj.result.Title;
        footerCoverEl.classList.add('clickable');
    }
    else {
        if (currentState.songPos === -1) {
            currentTitleEl.textContent = tn('Not playing');
            footerTitleEl.textContent = tn('Not playing');
        }
        else {
            elClear(currentTitleEl);
            elClear(footerTitleEl);
        }
        setData(currentTitleEl, 'uri', '');
        currentTitleEl.classList.remove('clickable');
        footerTitleEl.classList.remove('clickable');
        footerCoverEl.classList.remove('clickable');
    }
    document.title = 'myMPD: ' + pageTitle.join(smallSpace + nDash + smallSpace);
    footerCoverEl.title = pageTitle.join(smallSpace + nDash + smallSpace);

    if (isValidUri(obj.result.uri) === true &&
        isStreamUri(obj.result.uri) === false)
    {
        footerTitleEl.classList.add('clickable');
        currentTitleEl.classList.add('clickable');
    }
    else {
        footerTitleEl.classList.remove('clickable');
        currentTitleEl.classList.remove('clickable');
    }

    if (obj.result.uri !== undefined) {
        obj.result['Filetype'] = filetype(obj.result.uri);
        elEnableId('addCurrentSongToPlaylist');
    }
    else {
        obj.result['Filetype'] = '';
        elDisableId('addCurrentSongToPlaylist');
    }

    if (features.featStickers === true) {
        setVoteSongBtns(obj.result.stickerLike, obj.result.uri);
    }

    setPlaybackCardTags(obj.result);

    const bookletEl = document.getElementById('currentBooklet');
    elClear(bookletEl);
    if (obj.result.bookletPath !== '' &&
        obj.result.bookletPath !== undefined &&
        features.featLibrary === true)
    {
        bookletEl.appendChild(
            elCreateText('span', {"class": ["mi", "me-2"]}, 'description')
        );
        bookletEl.appendChild(
            elCreateTextTn('a', {"target": "_blank", "href": myEncodeURI(subdir + obj.result.bookletPath)}, 'Download booklet')
        );
    }

    //update queue card
    queueSetCurrentSong();

    //update title in queue view for streams
    const playingTr = document.getElementById('queueSongId' + obj.result.currentSongId);
    if (playingTr !== null) {
        const titleCol = playingTr.querySelector('[data-col=Title');
        if (titleCol !== null) {
            titleCol.textContent = getDisplayTitle(obj.result.Name, obj.result.Title);
        }
        setData(playingTr, 'name', obj.result.Title);
    }

    if (currentState.state === 'play') {
        //check if song has really changed
        if (currentSongObj === null ||
            currentSongObj.Title !== obj.result.Title ||
            currentSongObj.uri !== obj.result.uri)
        {
            showNotification(textNotification.join('\n'), 'player', 'info');
        }
    }

    setScrollViewHeight(list);

    //remember current song
    currentSongObj = obj.result;
}

/**
 * Sets the values of the tags displayed in the playback view
 * @param {object} songObj song object (result object of MYMPD_API_PLAYER_CURRENT_SONG jsonrpc response)
 * @returns {void}
 */
function setPlaybackCardTags(songObj) {
    if (songObj.webradio === undefined) {
        for (const col of settings.colsPlayback) {
            elHideId('cardPlaybackWebradio');
            elShowId('cardPlaybackTags');
            const c = document.getElementById('current' + col);
            if (c === null) {
                continue;
            }
            switch(col) {
                case 'Lyrics':
                    getLyrics(songObj.uri, c.querySelector('p'));
                    break;
                case 'AudioFormat':
                    //songObj has no audioformat definition - use current state
                    elReplaceChild(c.querySelector('p'), printValue('AudioFormat', currentState.AudioFormat));
                    break;
                default: {
                    let value = songObj[col];
                    if (value === undefined) {
                        value = '-';
                    }
                    if (checkTagValue(value, '-') === true ||
                        settings.tagListBrowse.includes(col) === false)
                    {
                        elClear(c.querySelector('p'));
                        c.querySelector('p').classList.remove('clickable');
                    }
                    else {
                        elReplaceChild(c.querySelector('p'), printValue(col, value));
                        c.querySelector('p').classList.add('clickable');
                    }
                    setData(c, 'name', value);
                    if (col === 'Album') {
                        setData(c, 'AlbumId', songObj.AlbumId);
                    }
                }
            }
        }
        const mbField = addMusicbrainzFields(songObj, true);
        if (mbField !== null) {
            elReplaceChildId('currentMusicbrainz', mbField);
        }
        else {
            elClearId('currentMusicbrainz');
        }
    }
    else {
        //webradio info
        const cardPlaybackWebradio = document.getElementById('cardPlaybackWebradio');
        elShow(cardPlaybackWebradio);
        elHideId('cardPlaybackTags');

        const webradioName = elCreateText('p', {"href": "#", "class": ["clickable"]}, songObj.webradio.Name);
        setData(webradioName, 'href', {"cmd": "editRadioFavorite", "options": [songObj.webradio.filename]});
        webradioName.addEventListener('click', function(event) {
            parseCmd(event, getData(event.target, 'href'));
        }, false);
        elReplaceChild(cardPlaybackWebradio,
            elCreateNodes('div', {"class": ["col-xl-6"]}, [
                elCreateTextTn('small', {}, 'Webradio'),
                webradioName
            ])
        );
        cardPlaybackWebradio.appendChild(
            elCreateNodes('div', {"class": ["col-xl-6"]}, [
                elCreateTextTn('small', {}, 'Genre'),
                elCreateText('p', {}, songObj.webradio.Genre)
            ])
        );
        cardPlaybackWebradio.appendChild(
            elCreateNodes('div', {"class": ["col-xl-6"]}, [
                elCreateTextTn('small', {}, 'Country'),
                elCreateText('p', {}, songObj.webradio.Country + smallSpace + nDash + smallSpace + songObj.webradio.Language)
            ])
        );
        if (songObj.webradio.Homepage !== '') {
            cardPlaybackWebradio.appendChild(
                elCreateNodes('div', {"class": ["col-xl-6"]}, [
                    elCreateTextTn('small', {}, 'Homepage'),
                    elCreateNode('p', {}, 
                        printValue('homepage', songObj.webradio.Homepage)
                    )
                ])
            );
        }
        if (songObj.webradio.Codec !== '' &&
            songObj.webradio.Codec !== undefined)
        {
            cardPlaybackWebradio.appendChild(
                elCreateNodes('div', {"class": ["col-xl-6"]}, [
                    elCreateTextTn('small', {}, 'Format'),
                    elCreateText('p', {}, songObj.webradio.Codec + 
                        (songObj.webradio.Bitrate !== ''
                            ? ' / ' + songObj.webradio.Bitrate + ' ' + tn('kbit')
                            : ''
                        )
                    )
                ])
            );
        }
        if (songObj.webradio.Description !== '') {
            cardPlaybackWebradio.appendChild(
                elCreateNodes('div', {"class": ["col-xl-6"]}, [
                    elCreateTextTn('small', {}, 'Description'),
                    elCreateText('p', {}, songObj.webradio.Description)
                ])
            );
        }
    }
}

/**
 * Handler for the currentTitle element click event
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function clickTitle() {
    const uri = getDataId('currentTitle', 'uri');
    if (isValidUri(uri) === true &&
        isStreamUri(uri) === false)
    {
        songDetails(uri);
    }
}

/**
 * Adds the current playing song to a playlist
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showAddToPlaylistCurrentSong() {
    const uri = getDataId('currentTitle', 'uri');
    if (uri !== '') {
        showAddToPlaylist('song', [uri]);
    }
}
