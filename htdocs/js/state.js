"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module state_js */

/**
 * Clears the mpd error
 * @returns {void}
 */
function clearMPDerror() {
    sendAPI('MYMPD_API_PLAYER_CLEARERROR',{}, function() {
        getState();
    }, false);
}

/**
 * Creates the elapsed / duration counter text
 * @returns {string} song counter text
 */
function getCounterText() {
    return fmtSongDuration(currentState.elapsedTime) +
        ( currentState.totalTime > 0
            ? smallSpace + '/' + smallSpace + fmtSongDuration(currentState.totalTime)
            : ''
        );
}

/**
 * Sets the song counters
 * @returns {void}
 */
function setCounter() {
    //progressbar in footer
    //calc percent with two decimals after comma
    const prct = currentState.totalTime > 0
        ? Math.ceil((100 / currentState.totalTime) * currentState.elapsedTime * 100) / 100
        : 0;
    domCache.progressBar.style.width = `${prct}vw`;
    domCache.progress.style.cursor = currentState.totalTime <= 0 ? 'default' : 'pointer';

    //counter
    const counterText = getCounterText();
    //counter in footer
    domCache.counter.textContent = counterText;
    //update queue card
    const playingRow = elGetById('queueSongId' + currentState.currentSongId);
    if (playingRow !== null) {
        //progressbar and counter in queue card
        if (currentState.state === 'stop') {
            resetDuration(playingRow);
        }
        else {
            setQueueCounter(playingRow, counterText);
        }
    }

    //synced lyrics
    if (showSyncedLyrics === true &&
        settings.colsPlayback.includes('Lyrics'))
    {
        const sl = elGetById('currentLyrics');
        const toHighlight = sl.querySelector('[data-sec="' + currentState.elapsedTime + '"]');
        const highlighted = sl.querySelector('.highlight');
        if (highlighted !== toHighlight &&
            toHighlight !== null)
        {
            toHighlight.classList.add('highlight');
            if (scrollSyncedLyrics === true) {
                toHighlight.scrollIntoView({behavior: "smooth"});
            }
            if (highlighted !== null) {
                highlighted.classList.remove('highlight');
            }
        }
    }

    if (progressTimer) {
        clearTimeout(progressTimer);
    }
    if (currentState.state === 'play') {
        if (currentState.totalTime > 0 &&
            currentState.totalTime < currentState.elapsedTime)
        {
            // this should not appear, update state
            getState();
            return;
        }
        progressTimer = setTimeout(function() {
            currentState.elapsedTime += 1;
            setCounter();
        }, 1000);
    }
}

/**
 * Gets the player state
 * @returns {void}
 */
function getState() {
    sendAPI("MYMPD_API_PLAYER_STATE", {}, parseState, false);
}

/**
 * Parses the MYMPD_API_PLAYER_STATE jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parseState(obj) {
    if (obj.result === undefined) {
        logDebug('State is undefined');
        return;
    }
    //Get current song if songid or queueVersion has changed
    //On stream updates only the queue version will change
    if (currentState.currentSongId !== obj.result.currentSongId ||
        currentState.queueVersion !== obj.result.queueVersion)
    {
        sendAPI("MYMPD_API_PLAYER_CURRENT_SONG", {}, parseCurrentSong, false);
    }
    //save state
    currentState = obj.result;
    // set state of playback controls
    updatePlaybackControls();
    // update playing row in current queue view
    if (app.id === 'QueueCurrent') {
        setPlayingRow();
    }
    //media session
    mediaSessionSetState();
    mediaSessionSetPositionState(obj.result.totalTime, obj.result.elapsedTime);
    //local playback
    controlLocalPlayback(currentState.state);
    //queue badge in navbar
    const badgeQueueItemsEl = elGetById('badgeQueueItems');
    if (badgeQueueItemsEl) {
        badgeQueueItemsEl.textContent = obj.result.queueLength;
    }
    //Set volume
    parseVolume(obj);
    //Set play counters
    setCounter();
    //clear playback card if no current song
    if (obj.result.songPos === -1) {
        document.title = 'myMPD';
        elGetById('PlaybackTitle').textContent = tn('Not playing');
        const footerTitleEl = elGetById('footerTitle');
        footerTitleEl.textContent = tn('Not playing');
        footerTitleEl.removeAttribute('title');
        footerTitleEl.classList.remove('clickable');
        elGetById('footerCover').classList.remove('clickable');
        elGetById('PlaybackTitle').classList.remove('clickable');
        elGetById('PlaybackCover').classList.remove('clickable');
        clearCurrentCover();
        const pb = document.querySelectorAll('#PlaybackListTags p');
        for (let i = 0, j = pb.length; i < j; i++) {
            elClear(pb[i]);
        }
    }
    else {
        const cff = elGetById('currentAudioFormat');
        if (cff) {
            elReplaceChild(
                cff.querySelector('p'),
                printValue('AudioFormat', obj.result.AudioFormat)
            );
        }
    }

    //handle error from mpd status response
    toggleAlert('alertMpdStatusError', (obj.result.lastError === '' ? false : true), obj.result.lastError);

    //handle mpd update status
    toggleAlert('alertUpdateDBState', (obj.result.updateState === 0 ? false : true), tn('Updating MPD database'));
    
    //handle myMPD cache update status
    toggleAlert('alertUpdateCacheState', obj.result.updateCacheState, tn('Updating caches'));

    //check if we need to refresh the settings
    if (localSettings.partition !== obj.result.partition || /* partition has changed */
        settings.partition.mpdConnected === false ||        /* mpd is not connected */
        uiEnabled === false)                                /* ui is disabled at startup */
    {
        if (elGetById('modalSettings').classList.contains('show') ||
            elGetById('modalConnection').classList.contains('show') ||
            elGetById('modalPlayback').classList.contains('show'))
        {
            //do not refresh settings, if a settings modal is open
            return;
        }
        logDebug('Refreshing settings');
        getSettings(parseSettings);
    }
}

/**
 * Sets the state of the playback control buttons
 * @returns {void}
 */
function updatePlaybackControls() {
    const prefixes = ['footer'];
    if (document.querySelector('.playbackPopoverBtns') !== null) {
        prefixes.push('popoverFooter');
        if (currentState.songPos < 0 ||
            currentState.totalTime === 0 ||
            currentState.state === 'stop')
        {
            elDisableId('popoverFooterGotoBtn');
        }
        else {
            elEnableId('popoverFooterGotoBtn');
        }
    }
    for (const prefix of prefixes) {
        if (currentState.state === 'stop') {
            elGetById(prefix + 'PlayBtn').textContent = 'play_arrow';
            domCache.progressBar.style.width = '0';
            elDisableId(prefix + 'StopBtn');
            elDisableId(prefix + 'NextBtn');
            elDisableId(prefix + 'PrevBtn');
        }
        else if (currentState.state === 'play') {
            elGetById(prefix + 'PlayBtn').textContent =
                settings.webuiSettings.footerPlaybackControls === 'stop'
                    ? 'stop'
                    : 'pause';
            elEnableId(prefix + 'StopBtn');
            elEnableId(prefix + 'NextBtn');
            elEnableId(prefix + 'PrevBtn');
        }
        else {
            // pause
            elGetById(prefix + 'PlayBtn').textContent = 'play_arrow';
            elEnableId(prefix + 'StopBtn');
            elEnableId(prefix + 'NextBtn');
            elEnableId(prefix + 'PrevBtn');
        }

        if (currentState.songPos < 0 ||
            currentState.totalTime === 0 ||
            currentState.state === 'stop')
        {
            elDisableId(prefix + 'FastRewindBtn');
            elDisableId(prefix + 'FastForwardBtn');
        }
        else {
            // enable seeking only if totalTime is known
            elEnableId(prefix + 'FastRewindBtn');
            elEnableId(prefix + 'FastForwardBtn');
        }

        if (currentState.queueLength === 0) {
            // no song in queue
            elDisableId(prefix + 'PlayBtn');
        }
        else {
            elEnableId(prefix + 'PlayBtn');
        }

        if (currentState.nextSongPos === -1 &&
            settings.partition.jukeboxMode === 'off')
        {
            // last song in queue and disabled jukebox
            elDisableId(prefix + 'NextBtn');
        }
        else if (currentState.state !== 'stop') {
            // next button triggers jukebox
            elEnableId(prefix + 'NextBtn');
        }

        if (currentState.songPos < 0) {
            // no current song
            elDisableId(prefix + 'PrevBtn');
        }
        else if (currentState.state !== 'stop') {
            elEnableId(prefix + 'PrevBtn');
        }
    }
}

/**
 * Sets the background image
 * @param {HTMLElement} el element for the background image
 * @param {string} url background image url
 * @returns {void}
 */
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
        div.style.filter = settings.webuiSettings.bgCssFilter;
    }
    div.style.backgroundImage = 'url("' + bgImageUrl + '")';
    div.style.opacity = 0;
    setData(div, 'uri', bgImageUrl);
    el.insertBefore(div, el.firstChild);
    //create dummy img element for preloading and fade-in
    const img = new Image();
    // add reference to image container
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

/**
 * Clears the background image
 * @param {HTMLElement} el element for the background image
 * @returns {void}
 */
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

/**
 * Sets the current cover in playback view and footer
 * @param {string} url song uri
 * @returns {void}
 */
function setCurrentCover(url) {
    setBackgroundImage(elGetById('PlaybackCover'), url);
    setBackgroundImage(elGetById('footerCover'), url);
    if (settings.webuiSettings.bgCover === true) {
        setBackgroundImage(domCache.body, url);
    }
}

/**
 * Clears the current cover in playback view and footer
 * @returns {void}
 */
function clearCurrentCover() {
    clearBackgroundImage(elGetById('PlaybackCover'));
    clearBackgroundImage(elGetById('footerCover'));
    if (settings.webuiSettings.bgCover === true) {
        clearBackgroundImage(domCache.body);
    }
}

/**
 * Sets the elapsed time for the media session api
 * @param {number} duration song duration
 * @param {number} position elapsed time
 * @returns {void}
 */
function mediaSessionSetPositionState(duration, position) {
    if (features.featMediaSession === false ||
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

/**
 * Sets the state for the media session api
 * @returns {void}
 */
function mediaSessionSetState() {
    if (features.featMediaSession === false) {
        return;
    }
    const state = currentState.state === 'play'
        ? 'playing'
        : 'paused';
    logDebug('Set mediaSession.playbackState to ' + state);
    navigator.mediaSession.playbackState = state;
}

/**
 * Sets metadata for the media session api
 * @param {string} title song title
 * @param {object} artist array of artists
 * @param {string} album album name
 * @param {string} url song uri
 * @returns {void}
 */
function mediaSessionSetMetadata(title, artist, album, url) {
    if (features.featMediaSession === false) {
        return;
    }
    const artwork = getMyMPDuri() + '/albumart-thumb?offset=0&uri=' + myEncodeURIComponent(url);
    navigator.mediaSession.metadata = new MediaMetadata({
        title: title,
        artist: joinArray(artist),
        album: album,
        artwork: [{src: artwork}]
    });
}
