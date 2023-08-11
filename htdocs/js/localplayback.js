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
    document.getElementById('localPlaybackVolumeBar').addEventListener('change', function(event) {
        setLocalPlaybackVolume(Number(event.target.value));
    }, false);
}

/**
 * Changes the local volume by +/-10%
 * @param {string} dir direction, up or down
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function localPlaybackVolumeStep(dir) {
    const increment = dir === 'up' ? 0.1 : -0.1;
    const volumeBar = document.getElementById('localPlaybackVolumeBar');
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
    document.getElementById('localPlayer').volume = volume;
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
    const el = document.getElementById('localPlaybackBtn');
    const curState = getData(el, 'state');

    switch(newState) {
        case 'play':
            if (curState === 'stop' || curState === undefined) {
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
    const el = createEvent.target.nodeName === 'SPAN' ? createEvent.target.parentNode : createEvent.target;
    const curState = getData(el, 'state');
    elReplaceChild(el,
        elCreateText('span', {"class": ["mi"]}, 'play_arrow')
    );

    //stop playback off old audio element
    const curAudioEl = document.getElementById('localPlayer');
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
    document.getElementById('localPlayer').addEventListener('canplay', function() {
        logDebug('localPlayer event: canplay');
        elHideId('errorLocalPlayback');
        setData(el, 'state', 'play');
        btnWaiting(el, false);
        elReplaceChild(el,
            elCreateText('span', {"class": ["mi"]}, 'stop')
        );
    });
    document.getElementById('localPlayer').addEventListener('progress', function(event) {
        // @ts-ignore
        if (isNaN(event.target.duration)) {
            return;
        }
        // @ts-ignore
        document.getElementById('localPlayerProgress').textContent = fmtSongDuration(event.target.currentTime);
    });
    document.getElementById('localPlayer').addEventListener('volumechange', function(event) {
        // @ts-ignore
        document.getElementById('localPlaybackVolumeBar').value = document.getElementById('localPlayer').volume;
        document.getElementById('localPlaybackVolume').textContent = Math.floor(
            // @ts-ignore
            event.target.volume * 100) + ' %';
    });
    for (const ev of ['error', 'abort', 'stalled']) {
        document.getElementById('localPlayer').addEventListener(ev, function(event) {
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
            elClear(document.getElementById('localPlayerProgress'));
        });
    }
    if (curState === undefined ||
        curState === 'stop')
    {
        //load and play
        if (settings.partition.streamUri === '') {
            localPlayer.src = window.location.protocol + '//' + window.location.hostname +
                (window.location.port !== '' ? ':' + window.location.port : '') + subdir + '/stream/' + localSettings.partition;
        }
        else {
            localPlayer.src = settings.partition.streamUri;
        }
        localPlayer.load();
        localPlayer.play();
        elClear(el);
        btnWaiting(el, true);
        document.getElementById('localPlaybackVolumeBar').value = localPlayer.volume;
        document.getElementById('localPlaybackVolume').textContent = localPlayer.volume * 100 + ' %';
    }
    else {
        setData(el, 'state', 'stop');
        elClear(document.getElementById('localPlayerProgress'));
    }
}
