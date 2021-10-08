"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function initJukebox() {
    document.getElementById('QueueJukeboxList').addEventListener('click', function(event) {
        if (event.target.nodeName === 'TD') {
            if (settings.jukeboxMode === 1) {
                clickSong(getCustomDomProperty(event.target.parentNode, 'data-uri'), getCustomDomProperty(event.target.parentNode, 'data-name'));
            }
            else if (settings.jukeboxMode === 2) {
                clickAlbumPlay(getCustomDomProperty(event.target.parentNode, 'data-albumartist'), getCustomDomProperty(event.target.parentNode, 'data-album'));
            }
        }
        else if (event.target.nodeName === 'A') {
            showMenu(event.target, event);
        }
    }, false);
}

//eslint-disable-next-line no-unused-vars
function delQueueJukeboxSong(pos) {
    sendAPI("MYMPD_API_JUKEBOX_RM", {
        "pos": pos
    }, function() {
        sendAPI("MYMPD_API_JUKEBOX_LIST", {
            "offset": app.current.offset,
            "limit": app.current.limit,
            "cols": settings.colsQueueJukebox
        }, parseJukeboxList);
    });
}

function parseJukeboxList(obj) {
    if (checkResult(obj, 'QueueJukebox', null) === false) {
        if (obj.result !== undefined && obj.result.jukeboxMode === 0) {
            elHideId('QueueJukeboxList');
            elShowId('QueueJukeboxDisabled');
        }
        return;
    }

    elHideId('QueueJukeboxDisabled');
    elShowId('QueueJukeboxList');

    const rowTitle = webuiSettingsDefault.clickAlbumPlay.validValues[settings.webuiSettings.clickAlbumPlay];
    updateTable(obj, 'QueueJukebox', function(row, data) {
        setCustomDomProperty(row, 'data-uri', data.uri);
        setCustomDomProperty(row, 'data-name', data.Title);
        setCustomDomProperty(row, 'data-type', data.uri === 'Album' ? 'album' : 'song');
        setCustomDomProperty(row, 'data-pos', (data.Pos - 1));
        if (data.Album !== undefined) {
            setCustomDomProperty(row, 'data-album', data.Album);
        }
        if (data[tagAlbumArtist] !== undefined) {
            setCustomDomProperty(row, 'data-albumartist', data[tagAlbumArtist]);
        }
        row.setAttribute('title', t(rowTitle));
        row.setAttribute('tabindex', 0);
    });
}
