"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

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
    curAudioEl.remove();
    const localPlayer = elCreateEmpty('audio', {"class": ["mx-4"], "preload": "none", "id": "localPlayer"});
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
    }
    else {
        setData(el, 'state', 'stop');
        elClear(document.getElementById('localPlayerProgress'));
    }
}
