"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function clearMPDerror() {
    sendAPI('MYMPD_API_PLAYER_CLEARERROR',{}, function() {
        sendAPI('MYMPD_API_PLAYER_STATE', {}, function(obj) {
            parseState(obj);
        }, false);
    }, false);
}

function parseStats(obj) {
    document.getElementById('mpdstatsArtists').textContent = obj.result.artists;
    document.getElementById('mpdstatsAlbums').textContent = obj.result.albums;
    document.getElementById('mpdstatsSongs').textContent = obj.result.songs;
    document.getElementById('mpdstatsDbPlaytime').textContent = beautifyDuration(obj.result.dbPlaytime);
    document.getElementById('mpdstatsPlaytime').textContent = beautifyDuration(obj.result.playtime);
    document.getElementById('mpdstatsUptime').textContent = beautifyDuration(obj.result.uptime);
    document.getElementById('mpdstatsMympd_uptime').textContent = beautifyDuration(obj.result.myMPDuptime);
    document.getElementById('mpdstatsDbUpdated').textContent = localeDate(obj.result.dbUpdated);
    document.getElementById('mympdVersion').textContent = obj.result.mympdVersion;

    const mpdInfoVersionEl = document.getElementById('mpdInfoVersion');
    elClear(mpdInfoVersionEl);
    mpdInfoVersionEl.appendChild(document.createTextNode(obj.result.mpdProtocolVersion));

    const mpdProtocolVersion = obj.result.mpdProtocolVersion.match(/(\d+)\.(\d+)\.(\d+)/);
    if ((mpdProtocolVersion[1] < mpdVersion.major) ||
        (mpdProtocolVersion[1] <= mpdVersion.major && mpdProtocolVersion[2] < mpdVersion.minor) ||
        (mpdProtocolVersion[1] <= mpdVersion.major && mpdProtocolVersion[2] <= mpdVersion.minor && mpdProtocolVersion[3] < mpdVersion.patch)
       )
    {
        mpdInfoVersionEl.appendChild(
            elCreateText('div', {"class": ["alert", "alert-warning", "mt-2", "mb-1"], "data-phrase": 'MPD version is outdated'}, tn('MPD version is outdated'))
        );
    }
}

function getServerinfo() {
    const ajaxRequest = new XMLHttpRequest();
    ajaxRequest.open('GET', subdir + '/api/serverinfo', true);
    ajaxRequest.onreadystatechange = function() {
        if (ajaxRequest.readyState === 4) {
            let obj;
            try {
                obj = JSON.parse(ajaxRequest.responseText);
            }
            catch(error) {
                showNotification(tn('Can not parse response to json object'), '', 'general', 'error');
                logError('Can not parse response to json object:' + ajaxRequest.responseText);
            }
            document.getElementById('wsIP').textContent = obj.result.ip;
        }
    };
    ajaxRequest.send();
}

function getCounterText() {
    return beautifySongDuration(currentState.elapsedTime) + smallSpace +
        '/' + smallSpace + beautifySongDuration(currentState.totalTime);
}

function setCounter() {
    //progressbar in footer
    const progressPx = currentState.totalTime > 0 ?
        Math.ceil(domCache.progress.offsetWidth * currentState.elapsedTime / currentState.totalTime) : 0;
    if (progressPx < domCache.progressBar.offsetWidth - 50 ||
        progressPx > domCache.progressBar.offsetWidth + 50)
    {
        //prevent transition if change is to big
        domCache.progressBar.style.transition = 'none';
        elReflow(domCache.progressBar);
        domCache.progressBar.style.width = progressPx + 'px';
        elReflow(domCache.progressBar);
        domCache.progressBar.style.transition = progressBarTransition;
        elReflow(domCache.progressBar);
    }
    else {
        domCache.progressBar.style.width = progressPx + 'px';
    }
    domCache.progress.style.cursor = currentState.totalTime <= 0 ? 'default' : 'pointer';

    //counter
    const counterText = getCounterText();
    //counter in footer
    domCache.counter.textContent = counterText;
    //update queue card
    const playingRow = document.getElementById('queueTrackId' + currentState.currentSongId);
    if (playingRow !== null) {
        //progressbar and counter in queue card
        setQueueCounter(playingRow, counterText)
    }

    //synced lyrics
    if (showSyncedLyrics === true &&
        settings.colsPlayback.includes('Lyrics'))
    {
        const sl = document.getElementById('currentLyrics');
        const toHighlight = sl.querySelector('[data-sec="' + currentState.elapsedTime + '"]');
        const highlighted = sl.getElementsByClassName('highlight')[0];
        if (highlighted !== toHighlight &&
            toHighlight !== null)
        {
            toHighlight.classList.add('highlight');
            if (scrollSyncedLyrics === true) {
                toHighlight.scrollIntoView({behavior: "smooth"});
            }
            if (highlighted !== undefined) {
                highlighted.classList.remove('highlight');
            }
        }
    }

    if (progressTimer) {
        clearTimeout(progressTimer);
    }
    if (currentState.state === 'play') {
        progressTimer = setTimeout(function() {
            currentState.elapsedTime += 1;
            setCounter();
        }, 1000);
    }
}

function parseState(obj) {
    //Get current song if songid has changed
    //Get current song if queueVersion has changed - updates stream titles
    if (currentState.currentSongId !== obj.result.currentSongId ||
        currentState.queueVersion !== obj.result.queueVersion)
    {
        sendAPI("MYMPD_API_PLAYER_CURRENT_SONG", {}, songChange);
    }
    //save state
    currentState = obj.result;
    //Set playback buttons
    if (obj.result.state === 'stop') {
        document.getElementById('btnPlay').textContent = 'play_arrow';
        domCache.progressBar.style.transition = 'none';
        elReflow(domCache.progressBar);
        domCache.progressBar.style.width = '0';
        elReflow(domCache.progressBar);
        domCache.progressBar.style.transition = progressBarTransition;
        elReflow(domCache.progressBar);
    }
    else if (obj.result.state === 'play') {
        document.getElementById('btnPlay').textContent =
            settings.webuiSettings.uiFooterPlaybackControls === 'stop' ? 'stop' : 'pause';
    }
    else {
        //pause
        document.getElementById('btnPlay').textContent = 'play_arrow';
    }
    if (app.id === 'QueueCurrent') {
        setPlayingRow();
    }

    if (obj.result.queueLength === 0) {
        elDisableId('btnPlay');
    }
    else {
        elEnableId('btnPlay');
    }

    if (obj.result.nextSongPos === -1 &&
        settings.jukeboxMode === 'off')
    {
        elDisableId('btnNext');
    }
    else {
        elEnableId('btnNext');
    }

    if (obj.result.songPos < 0) {
        elDisableId('btnPrev');
    }
    else {
        elEnableId('btnPrev');
    }
    //media session
    mediaSessionSetState();
    mediaSessionSetPositionState(obj.result.totalTime, obj.result.elapsedTime);
    //local playback
    controlLocalPlayback(currentState.state);
    //queue badge in navbar
    const badgeQueueItemsEl = document.getElementById('badgeQueueItems');
    if (badgeQueueItemsEl) {
        badgeQueueItemsEl.textContent = obj.result.queueLength;
    }
    //Set volume
    parseVolume(obj);
    //Set play counters
    setCounter();
    //clear playback card if no current song
    if (obj.result.songPos === -1) {
        document.getElementById('currentTitle').textContent = tn('Not playing');
        document.title = 'myMPD';
        elClear(document.getElementById('footerTitle'));
        document.getElementById('footerTitle').removeAttribute('title');
        document.getElementById('footerTitle').classList.remove('clickable');
        document.getElementById('footerCover').classList.remove('clickable');
        document.getElementById('currentTitle').classList.remove('clickable');
        clearCurrentCover();
        const pb = document.getElementById('cardPlaybackTags').getElementsByTagName('p');
        for (let i = 0, j = pb.length; i < j; i++) {
            elClear(pb[i]);
        }
    }
    else {
        const cff = document.getElementById('currentAudioFormat');
        if (cff) {
            elClear(cff.getElementsByTagName('p')[0]);
            cff.getElementsByTagName('p')[0].appendChild(printValue('AudioFormat', obj.result.AudioFormat));
        }
    }

    //handle error from mpd status response
    if (obj.result.lastError === '') {
        toggleAlert('alertMpdStatusError', false, '');
    }
    else {
        toggleAlert('alertMpdStatusError', true, obj.result.lastError);
    }
    toggleTopAlert();

    //refresh settings if mpd is not connected or ui is disabled
    //ui is disabled at startup
    if (settings.mpdConnected === false ||
        uiEnabled === false)
    {
        logDebug((settings.mpdConnected === false ? 'MPD disconnected' : 'UI disabled') + ' - refreshing settings');
        getSettings(true);
    }
}

function setBackgroundImage(el, url) {
    if (url === undefined) {
        clearBackgroundImage(el);
        return;
    }
    const bgImageUrl = subdir + '/albumart?offset=0&uri=' + myEncodeURIComponent(url);
    const old = el.parentNode.querySelectorAll(el.tagName + '> div.albumartbg');
    //do not update if url is the same
    if (old[0] &&
        getData(old[0], 'uri') === bgImageUrl)
    {
        logDebug('Background image already set for: ' + el.tagName);
        return;
    }
    //remove old covers that are already hidden and
    //update z-index of current displayed cover
    for (let i = 0, j = old.length; i < j; i++) {
        if (old[i].style.zIndex === '-10') {
            old[i].remove();
        }
        else {
            old[i].style.zIndex = '-10';
        }
    }
    //add new cover and let it fade in
    const div = elCreateEmpty('div', {"class": ["albumartbg"]});
    if (el.tagName === 'BODY') {
        div.style.filter = settings.webuiSettings.uiBgCssFilter;
    }
    div.style.backgroundImage = 'url("' + bgImageUrl + '")';
    div.style.opacity = 0;
    setData(div, 'uri', bgImageUrl);
    el.insertBefore(div, el.firstChild);
    //create dummy img element for preloading and fade-in
    const img = new Image();
    setData(img, 'div', div);
    img.onload = function(event) {
        //fade-in current cover
        getData(event.target, 'div').style.opacity = 1;
        //fade-out old cover with some overlap
        setTimeout(function() {
            const bgImages = el.parentNode.querySelectorAll(el.tagName + '> div.albumartbg');
            for (let i = 0, j = bgImages.length; i < j; i++) {
                if (bgImages[i].style.zIndex === '-10') {
                    bgImages[i].style.opacity = 0;
                }
            }
        }, 800);
    };
    img.src = bgImageUrl;
}

function clearBackgroundImage(el) {
    const old = el.parentNode.querySelectorAll(el.tagName + '> div.albumartbg');
    for (let i = 0, j = old.length; i < j; i++) {
        if (old[i].style.zIndex === '-10') {
            old[i].remove();
        }
        else {
            old[i].style.zIndex = '-10';
            old[i].style.opacity = '0';
        }
    }
}

function setCurrentCover(url) {
    setBackgroundImage(document.getElementById('currentCover'), url);
    setBackgroundImage(document.getElementById('footerCover'), url);
    if (settings.webuiSettings.uiBgCover === true) {
        setBackgroundImage(domCache.body, url);
    }
}

function clearCurrentCover() {
    clearBackgroundImage(document.getElementById('currentCover'));
    clearBackgroundImage(document.getElementById('footerCover'));
    if (settings.webuiSettings.uiBgCover === true) {
        clearBackgroundImage(domCache.body);
    }
}

function songChange(obj) {
    //check for song change
    //use title to detect stream changes
    const newSong = obj.result.uri + ':' + obj.result.Title + ':' + obj.result.currentSongId;
    if (currentSong === newSong) {
        return;
    }
    let textNotification = '';
    let pageTitle = '';

    mediaSessionSetMetadata(obj.result.Title, joinArray(obj.result.Artist), obj.result.Album, obj.result.uri);

    setCurrentCover(obj.result.uri);

    for (const elName of ['footerArtist', 'footerAlbum', 'footerCover', 'currentTitle']) {
        document.getElementById(elName).classList.remove('clickable');
    }

    const footerArtistEl = document.getElementById('footerArtist');
    if (obj.result.Artist !== undefined &&
        obj.result.Artist[0] !== '-')
    {
        textNotification += joinArray(obj.result.Artist);
        pageTitle += obj.result.Artist.join(', ') + smallSpace + nDash + smallSpace;
        footerArtistEl.textContent = obj.result.Artist;
        setData(footerArtistEl, 'name', obj.result.Artist);
        if (features.featAdvsearch === true) {
            footerArtistEl.classList.add('clickable');
        }
    }
    else {
        footerArtistEl.textContent = '';
        setData(footerArtistEl, 'name', ['']);
    }

    const footerAlbumEl = document.getElementById('footerAlbum');
    if (obj.result.Album !== undefined &&
        obj.result.Album !== '-')
    {
        textNotification += ' - ' + obj.result.Album;
        footerAlbumEl.textContent = obj.result.Album;
        setData(footerAlbumEl, 'name', obj.result.Album);
        setData(footerAlbumEl, 'AlbumArtist', obj.result[tagAlbumArtist]);
        if (features.featAdvsearch === true) {
            footerAlbumEl.classList.add('clickable');
        }
    }
    else {
        footerAlbumEl.textContent = '';
        setData(footerAlbumEl, 'name', '');
        setData(footerAlbumEl, 'AlbumArtist', ['']);
    }

    const footerTitleEl = document.getElementById('footerTitle');
    const footerCoverEl = document.getElementById('footerCover');
    const currentTitleEl = document.getElementById('currentTitle');
    if (obj.result.Title !== undefined &&
        obj.result.Title !== '-')
    {
        pageTitle += obj.result.Title;
        currentTitleEl.textContent = obj.result.Title;
        setData(currentTitleEl, 'uri', obj.result.uri);
        footerTitleEl.textContent = obj.result.Title;
        footerCoverEl.classList.add('clickable');
    }
    else {
        document.getElementById('currentTitle').textContent = '';
        setData(currentTitleEl, 'uri', '');
        footerTitleEl.textContent = '';
        currentTitleEl.classList.remove('clickable');
        footerTitleEl.classList.remove('clickable');
        footerCoverEl.classList.remove('clickable');
    }
    document.title = 'myMPD: ' + pageTitle;
    footerCoverEl.title = pageTitle;

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
        bookletEl.appendChild(elCreateText('span', {"class": ["mi", "me-2"]}, 'description'));
        bookletEl.appendChild(elCreateText('a', {"target": "_blank", "href": subdir +
            myEncodeURI(obj.result.bookletPath)}, tn('Download booklet')));
    }

    //update queue card
    queueSetCurrentSong();

    //update title in queue view for http streams
    const playingTr = document.getElementById('queueTrackId' + obj.result.currentSongId);
    if (playingTr !== null) {
        const titleCol = playingTr.querySelector('[data-col=Title');
        if (titleCol) {
            titleCol.textContent = obj.result.Title;
        }
    }

    if (currentState.state === 'play') {
        showNotification(obj.result.Title, textNotification, 'player', 'info');
    }

    //remember current song
    currentSong = newSong;
    currentSongObj = obj.result;
}

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
                    getLyrics(songObj.uri, c.getElementsByTagName('p')[0]);
                    break;
                case 'AudioFormat':
                    //songObj has no audioformat definition - use current state
                    elReplaceChild(c.getElementsByTagName('p')[0], printValue('AudioFormat', currentState.AudioFormat));
                    break;
                default: {
                    let value = songObj[col];
                    if (value === undefined) {
                        value = '-';
                    }
                    elReplaceChild(c.getElementsByTagName('p')[0], printValue(col, value));
                    if ((typeof value === 'string' && value === '-') ||
                        (typeof value === 'object' && value[0] === '-') ||
                        settings.tagListBrowse.includes(col) === false)
                    {
                        c.getElementsByTagName('p')[0].classList.remove('clickable');
                    }
                    else {
                        c.getElementsByTagName('p')[0].classList.add('clickable');
                    }
                    setData(c, 'name', value);
                    if (col === 'Album' &&
                        songObj[tagAlbumArtist] !== undefined)
                    {
                        setData(c, 'AlbumArtist', songObj[tagAlbumArtist]);
                    }
                }
            }
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
            elCreateNodes('div', {}, [
                elCreateText('small', {}, tn('Webradio')),
                webradioName
            ])
        );
        cardPlaybackWebradio.appendChild(
            elCreateNodes('div', {}, [
                elCreateText('small', {}, tn('Genre')),
                elCreateText('p', {}, songObj.webradio.Genre)
            ])
        );
        cardPlaybackWebradio.appendChild(
            elCreateNodes('div', {}, [
                elCreateText('small', {}, tn('Country')),
                elCreateText('p', {}, songObj.webradio.Country + smallSpace + nDash + smallSpace + songObj.webradio.Language)
            ])
        );
        if (songObj.webradio.Homepage !== '') {
            cardPlaybackWebradio.appendChild(
                elCreateNodes('div', {}, [
                    elCreateText('small', {}, tn('Homepage')),
                    elCreateNode('p', {}, 
                        elCreateText('a', {"class": ["text-success", "external"],
                            "href": myEncodeURIhost(songObj.webradio.Homepage),
                            "rel": "noreferrer",
                            "target": "_blank"}, songObj.webradio.Homepage)
                    )
                ])
            );
        }
        if (songObj.webradio.Codec !== '' &&
            songObj.webradio.Codec !== undefined)
        {
            cardPlaybackWebradio.appendChild(
                elCreateNodes('div', {}, [
                    elCreateText('small', {}, tn('Format')),
                    elCreateText('p', {}, songObj.webradio.Codec + 
                        (songObj.webradio.Bitrate !== '' ? ' / ' + songObj.webradio.Bitrate + ' ' + tn('kbit') : ''))
                ])
            );
        }
        if (songObj.webradio.Description !== '') {
            cardPlaybackWebradio.appendChild(
                elCreateNodes('div', {}, [
                    elCreateText('small', {}, tn('Description')),
                    elCreateText('p', {}, songObj.webradio.Description)
                ])
            );
        }
    }
}

//eslint-disable-next-line no-unused-vars
function gotoTagList() {
    appGoto(app.current.card, app.current.tab, app.current.view, 0, undefined, '-', '-', '-', '');
}

//eslint-disable-next-line no-unused-vars
function clickTitle() {
    const uri = getDataId('currentTitle', 'uri');
    if (isValidUri(uri) === true &&
        isStreamUri(uri) === false)
    {
        songDetails(uri);
    }
}

function mediaSessionSetPositionState(duration, position) {
    if (checkMediaSessionSupport() === false ||
        navigator.mediaSession.setPositionState === undefined)
    {
        return;
    }
    if (position < duration) {
        //streams have position > duration
        navigator.mediaSession.setPositionState({
            duration: duration,
            position: position
        });
    }
}

function mediaSessionSetState() {
    if (checkMediaSessionSupport() === false) {
        return;
    }
    const state = currentState.state === 'play' ? 'playing' : 'paused';
    logDebug('Set mediaSession.playbackState to ' + state);
    navigator.mediaSession.playbackState = state;
}

function mediaSessionSetMetadata(title, artist, album, url) {
    if (checkMediaSessionSupport() === false) {
        return;
    }
    const artwork = window.location.protocol + '//' + window.location.hostname +
        (window.location.port !== '' ? ':' + window.location.port : '') + subdir + '/albumart?offset=0&uri=' + myEncodeURIComponent(url);
    navigator.mediaSession.metadata = new MediaMetadata({
        title: title,
        artist: artist,
        album: album,
        artwork: [{src: artwork}]
    });
}
