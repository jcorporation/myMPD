"use strict";
// SPDX-License-Identifier: GPL-2.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

//eslint-disable-next-line no-unused-vars
function delQueueJukeboxSong(pos) {
    sendAPI("MPD_API_JUKEBOX_RM", {"pos": pos}, function() {
        sendAPI("MPD_API_JUKEBOX_LIST", {"offset": app.current.offset, "cols": settings.colsQueueJukebox}, parseJukeboxList);
    });
}

function parseJukeboxList(obj) {
    const rowTitle = advancedSettingsDefault.clickSong.validValues[settings.advanced.clickSong];
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
        setAttEnc(row, 'data-type', 'song');
        setAttEnc(row, 'data-pos', i);
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
