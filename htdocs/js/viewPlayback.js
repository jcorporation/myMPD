"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
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
 function initViewPlayback() {
    elGetById('PlaybackListTags').addEventListener('click', function(event) {
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
    const list = elGetById('PlaybackList');
    unsetUpdateView(list);

    const textNotification = [];
    const pageTitle = [];

    if (isEmptyTag(obj.result.Title) === false) {
        textNotification.push(obj.result.Title);
    }

    mediaSessionSetMetadata(obj.result.Title, obj.result.Artist, obj.result.Album, obj.result.uri);
    setCurrentCover(obj.result.uri);

    const footerAlbumEl = elGetById('footerAlbum');
    const footerArtistEl = elGetById('footerArtist');
    const footerTitleEl = elGetById('footerTitle');
    const footerCoverEl = elGetById('footerCover');
    const footerDividerEl = elGetById('footerDivider');
    const PlaybackTitleEl = elGetById('PlaybackTitle');
    const PlaybackCoverEl = elGetById('PlaybackCover');

    for (const el of [footerTitleEl, footerArtistEl, footerAlbumEl, footerCoverEl,
                      PlaybackTitleEl, PlaybackCoverEl])
    {
        el.classList.remove('clickable');
    }

    if (isEmptyTag(obj.result.Artist) === false) {
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
    }

    if (isEmptyTag(obj.result.Album) === false) {
        textNotification.push(obj.result.Album);
        footerAlbumEl.textContent = obj.result.Album;
        setData(footerAlbumEl, 'name', obj.result.Album);
        setData(footerAlbumEl, 'AlbumId', obj.result.AlbumId);
        footerAlbumEl.classList.add('clickable');
        footerAlbumEl.setAttribute('data-tag', 'Album');
        footerDividerEl.classList.remove('d-none');
    }
    else if (isEmptyTag(obj.result.Album) === true &&
             isEmptyTag(obj.result.Name) === false)
    {
        footerAlbumEl.textContent = obj.result.Name;
        footerAlbumEl.setAttribute('data-tag', 'undefined');
        footerDividerEl.classList.add('d-none');
    }
    else {
        elClear(footerAlbumEl);
        setData(footerAlbumEl, 'name', '');
        setData(footerAlbumEl, 'AlbumId', '');
        footerAlbumEl.setAttribute('data-tag', 'undefined');
        footerDividerEl.classList.add('d-none');
    }

    if (isEmptyTag(obj.result.Title) === false) {
        pageTitle.push(obj.result.Title);
        PlaybackTitleEl.textContent = obj.result.Title;
        setData(PlaybackTitleEl, 'uri', obj.result.uri);
        footerTitleEl.textContent = obj.result.Title;
        footerCoverEl.classList.add('clickable');
        PlaybackTitleEl.classList.add('clickable');
        PlaybackCoverEl.classList.add('clickable');
    }
    else {
        if (currentState.songPos === -1) {
            PlaybackTitleEl.textContent = tn('Not playing');
            footerTitleEl.textContent = tn('Not playing');
        }
        else {
            elClear(PlaybackTitleEl);
            elClear(footerTitleEl);
        }
        setData(PlaybackTitleEl, 'uri', '');
    }
    document.title = 'myMPD: ' + pageTitle.join(smallSpace + nDash + smallSpace);
    footerCoverEl.title = pageTitle.join(smallSpace + nDash + smallSpace);

    if (isValidUri(obj.result.uri) === true &&
        isStreamUri(obj.result.uri) === false)
    {
        // valid uri and not a stream
        footerTitleEl.classList.add('clickable');
        PlaybackTitleEl.classList.add('clickable');
    }

    if (obj.result.uri !== undefined) {
        obj.result['Filetype'] = filetype(obj.result.uri, true);
        elEnableId('PlaybackAddToPlaylist');
    }
    else {
        obj.result['Filetype'] = '';
        elDisableId('PlaybackAddToPlaylist');
    }

    if (features.featStickers === true) {
        if (settings.webuiSettings.feedback === 'like') {
            setVoteSongBtns(obj.result.like, obj.result.uri);
        }
        else if (settings.webuiSettings.feedback === 'rating') {
            setRating(elGetById('PlaybackSongRating'), obj.result.rating);
        }
    }

    setPlaybackCardTags(obj.result);

    const bookletEl = elGetById('PlaybackBooklet');
    elClear(bookletEl);
    if (isEmptyTag(obj.result.bookletPath) === false) {
        bookletEl.appendChild(
            elCreateText('span', {"class": ["mi", "me-2"]}, 'description')
        );
        bookletEl.appendChild(
            elCreateTextTn('a', {"target": "_blank", "href": myEncodeURI(subdir + obj.result.bookletPath)}, 'Booklet')
        );
    }

    const infoTxtEl = elGetById('PlaybackInfoTxt');
    elClear(infoTxtEl);
    if (isEmptyTag(obj.result.infoTxtPath) === false) {
        infoTxtEl.appendChild(
            elCreateText('span', {"class": ["mi", "me-2"]}, 'article')
        );
        infoTxtEl.appendChild(
            elCreateTextTn('span', {"href": myEncodeURI(subdir + obj.result.infoTxtPath)}, 'Album info')
        );
        setData(infoTxtEl, 'uri', obj.result.infoTxtPath);
    }
    else {
        rmData(infoTxtEl, 'uri');
    }

    //update queue card
    queueSetCurrentSong();

    //update title in queue view for streams
    const playingTr = elGetById('queueSongId' + obj.result.currentSongId);
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
        elHideId('PlaybackListWebradio');
        elShowId('PlaybackListTags');
        for (const col of settings.viewPlayback.fields) {
            const c = elGetById('current' + col);
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
                    const value = songObj[col];
                    const valueEl = c.querySelector('p');
                    elReplaceChild(valueEl, printValue(col, value));
                    if (isEmptyTag(value) === true ||
                        settings.tagListBrowse.includes(col) === false)
                    {
                        valueEl.classList.remove('clickable');
                    }
                    else {
                        valueEl.classList.add('clickable');
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
        const PlaybackListWebradio = elGetById('PlaybackListWebradio');
        elShow(PlaybackListWebradio);
        elHideId('PlaybackListTags');

        const webradioName = elCreateText('p', {"href": "#", "class": ["clickable"]}, songObj.webradio.Name);
        setData(webradioName, 'href', {"cmd": "editRadioFavorite", "options": [songObj.webradio.filename]});
        webradioName.addEventListener('click', function(event) {
            parseCmd(event, getData(event.target, 'href'));
        }, false);
        elReplaceChild(PlaybackListWebradio,
            elCreateNodes('div', {"class": ["col-xl-6"]}, [
                elCreateTextTn('small', {}, 'Webradio'),
                webradioName
            ])
        );
        PlaybackListWebradio.appendChild(
            elCreateNodes('div', {"class": ["col-xl-6"]}, [
                elCreateTextTn('small', {}, 'Genre'),
                elCreateText('p', {}, joinArray(songObj.webradio.Genres))
            ])
        );
        PlaybackListWebradio.appendChild(
            elCreateNodes('div', {"class": ["col-xl-6"]}, [
                elCreateTextTn('small', {}, 'Country'),
                elCreateText('p', {}, songObj.webradio.Country + 
                    (songObj.webradio.State !== '' && songObj.webradio.State !== undefined ? smallSpace + nDash + smallSpace + songObj.webradio.State : ''))
            ])
        );
        PlaybackListWebradio.appendChild(
            elCreateNodes('div', {"class": ["col-xl-6"]}, [
                elCreateTextTn('small', {}, 'Language'),
                elCreateText('p', {}, joinArray(songObj.webradio.Languages))
            ])
        );
        if (songObj.webradio.Homepage !== '') {
            PlaybackListWebradio.appendChild(
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
            PlaybackListWebradio.appendChild(
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
            PlaybackListWebradio.appendChild(
                elCreateNodes('div', {"class": ["col-xl-6"]}, [
                    elCreateTextTn('small', {}, 'Description'),
                    elCreateText('p', {}, songObj.webradio.Description)
                ])
            );
        }
    }
}

/**
 * Handler for the PlaybackTitle element click event
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function clickTitle() {
    const uri = getDataId('PlaybackTitle', 'uri');
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
    const uri = getDataId('PlaybackTitle', 'uri');
    if (uri !== '') {
        showAddToPlaylist('song', [uri]);
    }
}

/**
 * Sets the state of the song vote button group
 * @param {number} vote the vote 0 = hate, 1 = neutral, 2 = love
 * @param {string} uri song uri
 * @returns {void}
 */
function setVoteSongBtns(vote, uri) {
    if (uri === undefined) {
        uri = '';
    }

    const btnLove = elGetById('PlaybackSongLoveBtn');
    const btnHate = elGetById('PlaybackSongHateBtn');

    if (isValidUri(uri) === false ||
        isStreamUri(uri) === true)
    {
        elDisable(btnLove);
        elDisable(btnHate);
        elDisable(btnLove.parentNode);
        btnLove.classList.remove('active');
        btnHate.classList.remove('active');
    }
    else {
        elEnable(btnLove);
        elEnable(btnHate);
        elEnable(btnLove.parentNode);
    }

    switch(vote) {
        case 0:
            btnLove.classList.remove('active');
            btnHate.classList.add('active');
            break;
        case 2:
            btnLove.classList.add('active');
            btnHate.classList.remove('active');
            break;
        default:
            btnLove.classList.remove('active');
            btnHate.classList.remove('active');
            break;
    }
}
