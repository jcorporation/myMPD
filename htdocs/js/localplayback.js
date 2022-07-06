"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function initLocalPlayback() {
    //do not hide volume menu on click on volume change buttons
    for (const elName of ['btnLocalPlaybackChVolumeDown', 'btnLocalPlaybackChVolumeUp', 'localPlaybackVolumeBar']) {
        document.getElementById(elName).addEventListener('click', function(event) {
            event.stopPropagation();
        }, false);
    }

    document.getElementById('localPlaybackVolumeBar').addEventListener('change', function(event) {
        setLocalPlaybackVolume(Number(event.target.value));
    }, false);
}

//eslint-disable-next-line no-unused-vars
function localPlaybackVolumeStep(dir) {
    chLocalPlaybackVolume(dir === 'up' ? 0.1 : -0.1);
}

function chLocalPlaybackVolume(increment) {
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

function setLocalPlaybackVolume(volume) {
    document.getElementById('localPlayer').volume = volume;
}

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
    }
}

//eslint-disable-next-line no-unused-vars
function createLocalPlaybackEl(createEvent) {
    createEvent.stopPropagation();
    const el = createEvent.target.nodeName === 'SPAN' ? createEvent.target.parentNode : createEvent.target;
    const curState = getData(el, 'state');
    elReplaceChild(el, elCreateText('span', {"class": ["mi"]}, 'play_arrow'));

    //stop playback off old audio element
    const curAudioEl = document.getElementById('localPlayer');
    curAudioEl.setAttribute('disabled', 'disabled');
    curAudioEl.pause();
    curAudioEl.src = '';

    //replace old audio element
    const parent = curAudioEl.parentNode;
    const oldVolume = curAudioEl.volume;
    curAudioEl.remove();
    const localPlayer = elCreateEmpty('audio', {"class": ["mx-4"], "preload": "none", "id": "localPlayer"});
    localPlayer.volume = oldVolume;
    parent.appendChild(localPlayer);
    //add eventhandlers
    document.getElementById('localPlayer').addEventListener('canplay', function() {
        logDebug('localPlayer event: canplay');
        elHideId('errorLocalPlayback');
        setData(el, 'state', 'play');
        btnWaiting(el, false);
        elReplaceChild(el, elCreateText('span', {"class": ["mi"]}, 'stop'));
    });
    document.getElementById('localPlayer').addEventListener('progress', function(event) {
        if (isNaN(event.target.duration)) {
            return;
        }
        document.getElementById('localPlayerProgress').textContent = beautifySongDuration(event.target.currentTime);
    });
    document.getElementById('localPlayer').addEventListener('volumechange', function() {
        document.getElementById('localPlaybackVolumeBar').value = document.getElementById('localPlayer').volume;
        document.getElementById('localPlaybackVolume').textContent = Math.floor(
                document.getElementById('localPlayer').volume * 100) + ' %';
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
            elReplaceChild(el, elCreateText('span', {"class": ["mi"]}, 'play_arrow'));
            elClear(document.getElementById('localPlayerProgress'));
        });
    }
    if (curState === undefined ||
        curState === 'stop')
    {
        //load and play
        localPlayer.src = window.location.protocol + '//' + window.location.hostname +
            (window.location.port !== '' ? ':' + window.location.port : '') + subdir + '/stream/';
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
