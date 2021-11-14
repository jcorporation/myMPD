"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function initLocalplayer() {
   document.getElementById('alertLocalPlayback').getElementsByTagName('a')[0].addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        clickCheckLocalPlayerState(event);
    }, false);

    document.getElementById('errorLocalPlayback').getElementsByTagName('a')[0].addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        clickCheckLocalPlayerState(event);
    }, false);

    initLocalPlaybackControl();
}

function initLocalPlaybackControl() {
    document.getElementById('localPlayer').addEventListener('click', function(event) {
        event.stopPropagation();
    });

    document.getElementById('localPlayer').addEventListener('canplay', function() {
        logDebug('localPlayer event: canplay');
        elHideId('alertLocalPlayback');
        elHideId('errorLocalPlayback');
    });


    for (const ev of ['error', 'abort', 'stalled']) {
        document.getElementById('localPlayer').addEventListener(ev, function() {
            logError('localPlayer event: ' + ev);
            elShowId('errorLocalPlayback');
        });
    }
}

function setLocalPlayerUrl() {
    const streamUrl = window.location.protocol + '//' + window.location.hostname +
        (window.location.port !== '' ? ':' + window.location.port : '') + subdir + '/stream/';

    const localPlayer = document.getElementById('localPlayer');
    if (localPlayer.src !== streamUrl) {
        localPlayer.pause();
        localPlayer.src = streamUrl;
        localPlayer.load();
        setTimeout(function() {
            checkLocalPlayerState();
        }, 500);
    }
}

function clickCheckLocalPlayerState(event) {
    const el = event.target;
    el.classList.add('disabled');
    const parent = document.getElementById('localPlayer').parentNode;
    document.getElementById('localPlayer').remove();
    const localPlayer = elCreateEmpty('audio', {"class": ["mx-4"], "preload": "none", "controls": "", "id": "localPlayer"});
    parent.appendChild(localPlayer);
    initLocalPlaybackControl();
    setLocalPlayerUrl();
    setTimeout(function() {
        el.classList.remove('disabled');
        localPlayer.play();
    }, 500);
}

function checkLocalPlayerState() {
    const localPlayer = document.getElementById('localPlayer');
    elHideId('errorLocalPlayback');
    elHideId('alertLocalPlayback');
    if (localPlayer.networkState === 0) {
        logDebug('localPlayer networkState: ' + localPlayer.networkState);
        elShowId('alertLocalPlayback');
    }
    else if (localPlayer.networkState >=1) {
        logDebug('localPlayer networkState: ' + localPlayer.networkState);
    }
    if (localPlayer.networkState === 3) {
        logError('localPlayer networkState: ' + localPlayer.networkState);
        elShowId('errorLocalPlayback');
    }
}
