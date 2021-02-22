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
    let nrItems = obj.result.returnedEntities;
    let table = document.getElementById('QueueJukeboxList');
    let navigate = document.activeElement.parentNode.parentNode === table ? true : false;
    let activeRow = 0;
    let tbody = table.getElementsByTagName('tbody')[0];
    let tr = tbody.getElementsByTagName('tr');
    for (let i = 0; i < nrItems; i++) {
        obj.result.data[i].Duration = beautifySongDuration(obj.result.data[i].Duration);
        obj.result.data[i].LastPlayed = localeDate(obj.result.data[i].LastPlayed);
        let row = document.createElement('tr');
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
        let tds = '';
        for (let c = 0; c < settings.colsQueueJukebox.length; c++) {
            tds += '<td data-col="' + encodeURI(settings.colsQueueJukebox[c]) + '">' + e(obj.result.data[i][settings.colsQueueJukebox[c]]) + '</td>';
        }
        tds += '<td data-col="Action">';
        if (obj.result.data[i].uri !== '') {
            tds += '<a href="#" class="mi color-darkgrey">' + ligatureMore + '</a>';
        }
        tds += '</td>';
        row.innerHTML = tds;
        if (i < tr.length) {
            activeRow = replaceTblRow(tr[i], row) === true ? i : activeRow;
        }
        else {
            tbody.append(row);
        }
    }
    let trLen = tr.length - 1;
    for (let i = trLen; i >= nrItems; i --) {
        tr[i].remove();
    }                    

    let colspan = settings['colsQueueJukebox'].length;
    
    if (nrItems === 0) {
        tbody.innerHTML = '<tr class="not-clickable"><td><span class="mi">error_outline</span></td>' +
            '<td colspan="' + colspan + '">' + t('Empty list') + '</td></tr>';
    }

    if (navigate === true) {
        focusTable(activeRow);
    }

    setPagination(obj.result.totalEntities, obj.result.returnedEntities);
    document.getElementById('QueueJukeboxList').classList.remove('opacity05');
}
