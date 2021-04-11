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
    sendAPI("MYMPD_API_JUKEBOX_RM", {"pos": pos}, function() {
        sendAPI("MYMPD_API_JUKEBOX_LIST", {"offset": app.current.offset, "limit": app.current.limit, "cols": settings.colsQueueJukebox}, parseJukeboxList);
    });
}

function parseJukeboxList(obj) {
    const rowTitle = advancedSettingsDefault.clickAlbumPlay.validValues[settings.advanced.clickAlbumPlay];
    updateTable(obj, 'QueueJukebox', function(row, data) {
        setAttEnc(row, 'data-uri', data.uri);
        setAttEnc(row, 'data-name', data.Title);
        setAttEnc(row, 'data-type', data.uri === 'Album' ? 'album' : 'song');
        setAttEnc(row, 'data-pos', (data.Pos - 1));
        if (data.Album !== undefined) {
            setAttEnc(row, 'data-album', data.Album);
        }
        if (data[tagAlbumArtist] !== undefined) {
            setAttEnc(row, 'data-albumartist', data[tagAlbumArtist]);
        }
        row.setAttribute('title', t(rowTitle));
        row.setAttribute('tabindex', 0);
    });
}
