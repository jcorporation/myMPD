"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

let strings = {};
strings['cols'] = 'cols: array of columns to return';
strings['offset'] = 'offset: Offset of the returned list';

let desc = {};
desc['MPD_API_QUEUE_CLEAR'] = 'Clears the queue';
desc['MPD_API_QUEUE_CROP'] = 'Crops the queue - removes all songs but the current playing one';
desc['MPD_API_QUEUE_CROP_OR_CLEAR'] = 'Crops the queue if playing or clears the queue if not playing';
desc['MPD_API_QUEUE_SAVE'] = 'Saves the queue as a playlist' +
    '<ul>' +
    '<li>plist: name of the playlist</li>' +
    '</ul>';
desc['MPD_API_QUEUE_LIST'] = 'List the contents of the queu' +
    '<ul>' +
    '<li>' + strings['offset'] + '</li>' +
    '<li>' + strings['cols'] + '</li>' +
    '</ul>';
desc['MPD_API_QUEUE_SEARCH'] = 'Searches the queue' +
    '<ul>' +
    '<li>' + strings['offset'] + '</li>' +
    '<li>filter: tag to search, <code>any</code> for any tag</li>' +
    '<li>searchstr: string to search</li>' +
    '<li>' + strings['cols'] + '</li>' +
    '</ul>';
desc['MPD_API_QUEUE_RM_TRACK'] = 'Removes the track from the queue' +
    '<ul>' +
    '<li>track: the track id to remove from the queue</li>' +
    '</ul>';

let tbody = document.getElementsByTagName('tbody')[0];
for (let i = 0; i < cmds.length; i++) {
    let tr = document.createElement('tr');
    tr.innerHTML = '<td>' + cmds[i].method + '</td>' +
        '<td>' + paramsToString(cmds[i].params) + '</td>' +
        '<td>' + (desc[cmds[i].method] !== undefined ? desc[cmds[i].method] : '') + '</td>';
    tbody.appendChild(tr);
}

function paramsToString(obj) {
    if (obj === undefined) {
        return 'Without parameters';
    }
    
    return JSON.stringify(obj).
        replace(/</g, '&lt;').
        replace(/>/g, '&gt;').
        replace(/:/g, ': ').
        replace(/,/g, ', ');
}
