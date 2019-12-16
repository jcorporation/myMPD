"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

async function localplayerPlay() {
    let localPlayer = document.getElementById('localPlayer');
    if (localPlayer.paused) {
        try {
            await localPlayer.play();
        } 
        catch(err) {
            showNotification(t('Local playback'), t('Can not start playing'), '', 'danger');
        }
    }
}

//eslint-disable-next-line no-unused-vars
function addStream() {
    let streamUriEl = document.getElementById('streamUrl');
    if (validateStream(streamUriEl) === true) {
        sendAPI("MPD_API_QUEUE_ADD_TRACK", {"uri": streamUriEl.value});
        modalAddToPlaylist.hide();
        showNotification(t('Added stream %{streamUri} to queue', {"streamUri": streamUriEl.value}), '', '', 'success');
    }
}

//eslint-disable-next-line no-unused-vars
function clickPlay() {
    if (playstate !== 'play') {
        sendAPI("MPD_API_PLAYER_PLAY", {});
    }
    else {
        sendAPI("MPD_API_PLAYER_PAUSE", {});
    }
}

//eslint-disable-next-line no-unused-vars
function clickStop() {
    sendAPI("MPD_API_PLAYER_STOP", {});
}

//eslint-disable-next-line no-unused-vars
function clickPrev() {
    sendAPI("MPD_API_PLAYER_PREV", {});
}

//eslint-disable-next-line no-unused-vars
function clickNext() {
    sendAPI("MPD_API_PLAYER_NEXT", {});
}

//eslint-disable-next-line no-unused-vars
function execSyscmd(cmd) {
    sendAPI("MYMPD_API_SYSCMD", {"cmd": cmd});
}

//eslint-disable-next-line no-unused-vars
function clearCovercache() {
    sendAPI("MYMPD_API_COVERCACHE_CLEAR", {});
}

//eslint-disable-next-line no-unused-vars
function cropCovercache() {
    sendAPI("MYMPD_API_COVERCACHE_CROP", {});
}

//eslint-disable-next-line no-unused-vars
function updateDB(uri) {
    sendAPI("MPD_API_DATABASE_UPDATE", {"uri": uri});
    updateDBstarted(true);
}

//eslint-disable-next-line no-unused-vars
function rescanDB() {
    sendAPI("MPD_API_DATABASE_RESCAN", {"uri": uri});
    updateDBstarted(true);
}

function updateDBstarted(showModal) {
    if (showModal === true) {
        document.getElementById('updateDBfinished').innerText = '';
        document.getElementById('updateDBfooter').classList.add('hide');
        let updateDBprogress = document.getElementById('updateDBprogress');
        updateDBprogress.style.width = '20px';
        updateDBprogress.style.marginLeft = '-20px';
        modalUpdateDB.show();
        updateDBprogress.classList.add('updateDBprogressAnimate');
    }
    else {
        showNotification(t('Database update started'), '', '', 'success');
    }
}

function updateDBfinished(idleEvent) {
    if (document.getElementById('modalUpdateDB').classList.contains('show')) {
        if (idleEvent === 'update_database') {
            document.getElementById('updateDBfinished').innerText = t('Database successfully updated');
        }
        else if (idleEvent === 'update_finished') {
            document.getElementById('updateDBfinished').innerText = t('Database update finished');
        }
        let updateDBprogress = document.getElementById('updateDBprogress');
        updateDBprogress.classList.remove('updateDBprogressAnimate');
        updateDBprogress.style.width = '100%';
        updateDBprogress.style.marginLeft = '0px';
        document.getElementById('updateDBfooter').classList.remove('hide');
    }
    else {
        if (idleEvent === 'update_database') {
            showNotification(t('Database successfully updated'), '', '', 'success');
        }
        else if (idleEvent === 'update_finished') {
            showNotification(t('Database update finished'), '', '', 'success');
        }
    }
}
