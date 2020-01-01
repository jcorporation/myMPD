"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

function saveTimer() {
    let formOK = true;
    
    if (formOK === true) {
        let weekdayBtns = ['btnTimerMon', 'btnTimerTue', 'btnTimerWed', 'btnTimerThu', 'btnTimerFri', 'btnTimerSat', 'btnTimerSun'];
        let weekdays = [];
        for (let i = 0; i < weekdayBtns.length; i++) {
            weekdays.push(document.getElementById(weekdayBtns[i]).classList.contains('active') ? true : false);
        }
        let selectTimerAction = document.getElementById('selectTimerAction');
        let selectTimerPlaylist = document.getElementById('selectTimerPlaylist');
        let selectTimerHour = document.getElementById('selectTimerHour');
        let selectTimerMinute = document.getElementById('selectTimerMinute');
        sendAPI("MYMPD_API_TIMER_SET", {
            "timerid": document.getElementById('inputTimerId').value,
            "name": document.getElementById('inputTimerName').value,
            "enabled": (document.getElementById('btnTimerEnabled').classList.contains('active') ? true : false),
            "startHour": selectTimerHour.options[selectTimerHour.selectedIndex].value,
            "startMinute": selectTimerMinute.options[selectTimerMinute.selectedIndex].value,
            "weekdays": weekdays,
            "action": selectTimerAction.options[selectTimerAction.selectedIndex].value,
            "volume": document.getElementById('inputTimerVolume').value, 
            "playlist": selectTimerPlaylist.options[selectTimerPlaylist.selectedIndex].value
            }, showListTimer);
    }
}

function showEditTimer() {
    document.getElementById('listTimer').classList.remove('active');
    document.getElementById('editTimer').classList.add('active');
    document.getElementById('listTimerFooter').classList.add('hide');
    document.getElementById('editTimerFooter').classList.remove('hide');
    playlistEl = 'selectTimerPlaylist';
    sendAPI("MPD_API_PLAYLIST_LIST", {"offset": 0, "filter": "-"}, getAllPlaylists);
}

function showListTimer() {
    document.getElementById('listTimer').classList.add('active');
    document.getElementById('editTimer').classList.remove('active');
    document.getElementById('listTimerFooter').classList.remove('hide');
    document.getElementById('editTimerFooter').classList.add('hide');
}
