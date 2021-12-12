"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

//eslint-disable-next-line no-unused-vars
function createLocalPlaybackEl(createEvent) {
    createEvent.stopPropagation();
    const el = createEvent.target;
    const curState = getData(el, 'state');
    el.textContent = 'play_arrow';

    //stop playback off old audio element
    const curAudioEl = document.getElementById('localPlayer');
    curAudioEl.setAttribute('disabled', 'disabled');
    curAudioEl.pause();
    curAudioEl.src = '';

    //replace old audio element
    const parent = curAudioEl.parentNode;
    curAudioEl.remove();
    const localPlayer = elCreateEmpty('audio', {"class": ["mx-4"], "preload": "none", "id": "localPlayer"});
    parent.appendChild(localPlayer);
    //add eventhandlers
    document.getElementById('localPlayer').addEventListener('canplay', function() {
        logDebug('localPlayer event: canplay');
        elHideId('errorLocalPlayback');
        setData(el, 'state', 'play');
        el.textContent = 'stop';
        el.removeAttribute('disabled');
    });
    document.getElementById('localPlayer').addEventListener('progress', function(event) {
        if (isNaN(event.target.duration) ||
            isFinite(event.target.duration) === false)
        {
            return;
        }
        document.getElementById('localPlayerProgress').textContent = beautifySongDuration(event.target.duration);
    });
    for (const ev of ['error', 'abort', 'stalled']) {
        document.getElementById('localPlayer').addEventListener(ev, function(event) {
            if (event.target.getAttribute('disabled') === 'disabled') {
                //show now error while removing audio element
                return;
            }
            logError('localPlayer event: ' + ev);
            elShowId('errorLocalPlayback');
            el.textContent = tn('Retry');
            el.removeAttribute('disabled');
            setData(el, 'state', 'stop');
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
        el.textContent = 'autorenew';
        el.setAttribute('disabled', 'disabled');
    }
    else {
        setData(el, 'state', 'stop');
        elClear(document.getElementById('localPlayerProgress'));
    }
}
