"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function initJukebox() {
    document.getElementById('QueueJukeboxList').addEventListener('click', function(event) {
        if (event.target.nodeName === 'TD') {
            if (settings.jukeboxMode === 1) {
                clickSong(getData(event.target.parentNode, 'uri'), getData(event.target.parentNode, 'name'));
            }
            else if (settings.jukeboxMode === 2) {
                clickAlbumPlay(getData(event.target.parentNode, 'AlbumArtist'), getData(event.target.parentNode, 'Album'));
            }
        }
        else if (event.target.nodeName === 'A') {
            showPopover(event);
        }
    }, false);
    document.getElementById('searchQueueJukeboxStr').addEventListener('keyup', function(event) {
        if (event.key === 'Escape') {
            this.blur();
        }
        else {
            appGoto(app.current.card, app.current.tab, app.current.view,
                0, app.current.limit, app.current.filter, app.current.sort, '-', this.value);
        }
    }, false);
}

//eslint-disable-next-line no-unused-vars
function clearJukeboxQueue() {
    sendAPI("MYMPD_API_JUKEBOX_CLEAR", {}, function() {
        sendAPI("MYMPD_API_JUKEBOX_LIST", {
            "offset": app.current.offset,
            "limit": app.current.limit,
            "cols": settings.colsQueueJukeboxFetch,
            "searchstr": app.current.search
        }, parseJukeboxList);
    });
}

//eslint-disable-next-line no-unused-vars
function delQueueJukeboxSong(pos) {
    sendAPI("MYMPD_API_JUKEBOX_RM", {
        "pos": pos
    }, function() {
        sendAPI("MYMPD_API_JUKEBOX_LIST", {
            "offset": app.current.offset,
            "limit": app.current.limit,
            "cols": settings.colsQueueJukeboxFetch,
            "searchstr": app.current.search
        }, parseJukeboxList);
    });
}

function parseJukeboxList(obj) {
    if (checkResultId(obj, 'QueueJukeboxList') === false) {
        if (obj.result !== undefined && obj.result.jukeboxMode === 0) {
            elHideId('QueueJukeboxList');
            elShowId('QueueJukeboxDisabled');
        }
        setPagination(0,0);
        return;
    }

    elHideId('QueueJukeboxDisabled');
    elShowId('QueueJukeboxList');

    const rowTitle = webuiSettingsDefault.clickAlbumPlay.validValues[settings.webuiSettings.clickAlbumPlay];
    updateTable(obj, 'QueueJukebox', function(row, data) {
        setData(row, 'uri', data.uri);
        setData(row, 'name', data.Title);
        setData(row, 'type', data.uri === 'Album' ? 'album' : 'song');
        setData(row, 'pos', data.Pos);
        row.setAttribute('title', tn(rowTitle));
        row.setAttribute('tabindex', 0);
    });
}
