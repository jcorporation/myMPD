"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module localplayback_js */

/**
 * Initializes the local playback related elements
 * @returns {void}
 */
function initLocalPlayback() {
    elGetById('localPlaybackVolumeBar').addEventListener('change', function(event) {
        setLocalPlaybackVolume(Number(event.target.value));
    }, false);
    // @ts-ignore
    elGetById('localPlayer').volume = 0.5;
}

/**
 * Changes the local volume by +/-10%
 * @param {string} dir direction, up or down
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function localPlaybackVolumeStep(dir) {
    const increment = dir === 'up'
        ? 0.1
        : -0.1;
    const volumeBar = elGetById('localPlaybackVolumeBar');
    let newValue = Number(volumeBar.value) + increment;
    if (newValue < 0) {
        newValue = 0;
    }
    else if (newValue > 1) {
        newValue = 1;
    }
    volumeBar.value = newValue;
    setLocalPlaybackVolume(newValue);
}

/**
 * Sets local playback volume
 * @param {number} volume volume to set
 * @returns {void}
 */
function setLocalPlaybackVolume(volume) {
    // @ts-ignore
    elGetById('localPlayer').volume = volume;
}

/**
 * Sets the local playback state
 * @param {string} newState one of play, stop, pause
 * @returns {void}
 */
function controlLocalPlayback(newState) {
    if (features.featLocalPlayback === false ||
        localSettings.localPlaybackAutoplay === false)
    {
        return;
    }
    const el = elGetById('localPlaybackBtn');
    const curState = getData(el, 'state');

    switch(newState) {
        case 'play':
            if (curState === 'stop' ||
                curState === undefined)
            {
                el.click();
            }
            break;
        case 'stop':
        case 'pause':
            if (curState === 'play') {
                el.click();
            }
            break;
        default:
            logError('Invalid state: ' + newState);
    }
}

/**
 * Creates the local playback element
 * @param {Event} createEvent triggering event
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function createLocalPlaybackEl(createEvent) {
    createEvent.stopPropagation();
    const el = createEvent.target.nodeName === 'SPAN' ?
        createEvent.target.parentNode :
        createEvent.target;
    const curState = getData(el, 'state');
    elReplaceChild(el,
        elCreateText('span', {"class": ["mi"]}, 'play_arrow')
    );

    //stop playback off old audio element
    const curAudioEl = elGetById('localPlayer');
    curAudioEl.setAttribute('disabled', 'disabled');
    // @ts-ignore
    curAudioEl.pause();
    // @ts-ignore
    curAudioEl.src = '';

    //replace old audio element
    const parent = curAudioEl.parentNode;
    // @ts-ignore
    const oldVolume = curAudioEl.volume;
    curAudioEl.remove();
    /** @type {HTMLAudioElement} */
    // @ts-ignore
    const localPlayer = elCreateEmpty('audio', {"class": ["mx-4"], "preload": "none", "id": "localPlayer"});
    localPlayer.volume = oldVolume;
    parent.appendChild(localPlayer);
    //add eventhandlers
    elGetById('localPlayer').addEventListener('canplay', function() {
        logDebug('localPlayer event: canplay');
        elHideId('errorLocalPlayback');
        setData(el, 'state', 'play');
        btnWaiting(el, false);
        elReplaceChild(el,
            elCreateText('span', {"class": ["mi"]}, 'stop')
        );
    });
    elGetById('localPlayer').addEventListener('progress', function(event) {
        // @ts-ignore
        if (isNaN(event.target.duration)) {
            return;
        }
        // @ts-ignore
        elGetById('localPlayerProgress').textContent = fmtSongDuration(event.target.currentTime);
    });
    elGetById('localPlayer').addEventListener('volumechange', function(event) {
        // @ts-ignore
        elGetById('localPlaybackVolumeBar').value = elGetById('localPlayer').volume;
        elGetById('localPlaybackVolume').textContent = Math.floor(
            // @ts-ignore
            event.target.volume * 100) + ' %';
    });
    for (const ev of ['error', 'abort', 'stalled']) {
        elGetById('localPlayer').addEventListener(ev, function(event) {
            if (event.target.getAttribute('disabled') === 'disabled') {
                //show now error while removing audio element
                return;
            }
            logError('localPlayer event: ' + ev);
            elShowId('errorLocalPlayback');
            setData(el, 'state', 'stop');
            btnWaiting(el, false);
            elReplaceChild(el,
                elCreateText('span', {"class": ["mi"]}, 'play_arrow')
            );
            elClear(elGetById('localPlayerProgress'));
        });
    }
    if (curState === undefined ||
        curState === 'stop')
    {
        //load and play
        if (settings.partition.streamUri === '') {
            localPlayer.src = getMyMPDuri() + '/stream/' + localSettings.partition;
        }
        else {
            localPlayer.src = settings.partition.streamUri;
        }
        localPlayer.load();
        localPlayer.play();
        elClear(el);
        btnWaiting(el, true);
        elGetById('localPlaybackVolumeBar').value = localPlayer.volume;
        elGetById('localPlaybackVolume').textContent = localPlayer.volume * 100 + ' %';
    }
    else {
        setData(el, 'state', 'stop');
        elClear(elGetById('localPlayerProgress'));
    }
}
