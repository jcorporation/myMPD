"use strict";
// SPDX-License-Identifier: GPL-2.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function initJukebox() {
    document.getElementById('QueueJukeboxList').addEventListener('click', function(event) {
        if (event.target.nodeName === 'TD') {
            if (settings.jukeboxMode === 1) {
                clickSong(getAttDec(event.target.parentNode, 'data-uri'), getAttDec(event.target.parentNode, 'data-name'));
            }
            else if (settings.jukeboxMode === 2) {
                clickAlbumPlay(getAttDec(event.target.parentNode, 'data-albumartist'), getAttDec(event.target.parentNode, 'data-album'));
            }
        }
        else if (event.target.nodeName === 'A') {
            showMenu(event.target, event);
        }
    }, false);
}

//eslint-disable-next-line no-unused-vars
function delQueueJukeboxSong(pos) {
    sendAPI("MPD_API_JUKEBOX_RM", {"pos": pos}, function() {
        sendAPI("MPD_API_JUKEBOX_LIST", {"offset": app.current.offset, "limit": app.current.limit, "cols": settings.colsQueueJukebox}, parseJukeboxList);
    });
}

function parseJukeboxList(obj) {
    const rowTitle = advancedSettingsDefault.clickAlbumPlay.validValues[settings.advanced.clickAlbumPlay];
    updateTable(obj, 'QueueJukebox', function(row, data) {
        setAttEnc(row, 'data-uri', obj.result.data[i].uri);
        setAttEnc(row, 'data-name', obj.result.data[i].Title);
        setAttEnc(row, 'data-type', obj.result.data[i].uri === 'Album' ? 'album' : 'song');
        setAttEnc(row, 'data-pos', i);
        if (obj.result.data[i].Album !== undefined) {
            setAttEnc(row, 'data-album', obj.result.data[i].Album);
        }
        if (obj.result.data[i][tagAlbumArtist] !== undefined) {
            setAttEnc(row, 'data-albumartist', obj.result.data[i][tagAlbumArtist]);
        }
        row.setAttribute('title', t(rowTitle));
        row.setAttribute('tabindex', 0);
    });
}
