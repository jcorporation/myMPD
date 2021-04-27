"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/* global cmds */

let strings = {};
strings['cols'] = 'cols: array of columns to return';
strings['offset'] = 'offset: start offset of the returned list';
strings['limit'] = 'limit: maximum number of elements to return';

let desc = {};
desc['MYMPD_API_QUEUE_CLEAR'] = 'Clears the queue';
desc['MYMPD_API_QUEUE_CROP'] = 'Crops the queue - removes all songs but the current playing one';
desc['MYMPD_API_QUEUE_CROP_OR_CLEAR'] = 'Crops the queue if playing or clears the queue if not playing';
desc['MYMPD_API_QUEUE_SAVE'] = 'Saves the queue as a playlist' +
    '<ul>' +
    '<li>plist: name of the playlist</li>' +
    '</ul>';
desc['MYMPD_API_QUEUE_LIST'] = 'List the contents of the queu' +
    '<ul>' +
    '<li>' + strings['offset'] + '</li>' +
    '<li>' + strings['limit'] + '</li>' +
    '<li>' + strings['cols'] + '</li>' +
    '</ul>';
desc['MYMPD_API_QUEUE_SEARCH'] = 'Searches the queue' +
    '<ul>' +
    '<li>' + strings['offset'] + '</li>' +
    '<li>' + strings['limit'] + '</li>' +
    '<li>filter: tag to search, <code>any</code> for any tag</li>' +
    '<li>searchstr: string to search</li>' +
    '<li>' + strings['cols'] + '</li>' +
    '</ul>';
desc['MYMPD_API_QUEUE_RM_TRACK'] = 'Removes the song from the queue' +
    '<ul>' +
    '<li>track: the track id to remove from the queue</li>' +
    '</ul>';
desc['MYMPD_API_QUEUE_RM_RANGE'] = 'Removes a range of songs from the queue. Position numbering starts with 0.' +
    '<ul>' +
    '<li>start: start of the range (position)</li>' +
    '<li>end: end of the range (position)</li>' +
    '</ul>';
desc['MYMPD_API_QUEUE_MOVE_TRACK'] = 'Moves a song in the queue. Position numbering starts with 0.' +
    '<ul>' +
    '<li>from: position in queue</li>' +
    '<li>to: position in queue </li>' +
    '</ul>';
desc['MYMPD_API_QUEUE_ADD_TRACK_AFTER'] = 'Adds a song to the queue after the position in to parameter.' +
    '<ul>' +
    '<li>uri: song uri</li>' +
    '<li>to: position in queue</li>' +
    '</ul>';
desc['MYMPD_API_QUEUE_ADD_TRACK'] = 'Appends a song to the queue' +
    '<ul>' +
    '<li>uri: song uri</li>' +
    '</ul>';
desc['MYMPD_API_QUEUE_ADD_PLAY_TRACK'] = 'Appends a song to the queue and plays it.' +
    '<ul>' +
    '<li>uri: song uri</li>' +
    '</ul>';
desc['MYMPD_API_QUEUE_REPLACE_TRACK'] = 'Replaces the queue with the song.' +
    '<ul>' +
    '<li>uri: song uri</li>' +
    '</ul>';
desc['MYMPD_API_QUEUE_ADD_PLAYLIST'] = 'Appends the playlist to the queue.' +
    '<ul>' +
    '<li>plist: playlist</li>' +
    '</ul>';
desc['MYMPD_API_QUEUE_ADD_RANDOM'] = 'Appends random songs or albums to the queue.' +
    '<ul>' +
    '<li>playlist: playlist from which the jukebox selects the songs (Database or playlist name).</li>' +
    '<li>quantity: number of songs/albums to add</li>' +
    '<li>mode: 1 = song, 2 = album (only valid if playlist=Database)</li>' +
    '</ul>';
desc['MYMPD_API_QUEUE_REPLACE_PLAYLIST'] = 'Replaces the queue with the playlist.' +
    '<ul>' +
    '<li>plist: playlist</li>' +
    '</ul>';
desc['MYMPD_API_QUEUE_SHUFFLE'] = 'Shuffles the queue.';
desc['MYMPD_API_QUEUE_LAST_PLAYED'] = 'List the last played songs.' +
    '<ul>' +
    '<li>' + strings['offset'] + '</li>' +
    '<li>' + strings['limit'] + '</li>' +
    '<li>' + strings['cols'] + '</li>' +
    '</ul>';
desc['MYMPD_API_PLAYLIST_RM'] = 'Removes the playlist.' +
    '<ul>' +
    '<li>uri: playlist</li>' +
    '</ul>';
desc['MYMPD_API_PLAYLIST_CLEAR'] = 'Clears the playlist.' +
    '<ul>' +
    '<li>uri: playlist</li>' +
    '</ul>';
desc['MYMPD_API_PLAYLIST_RENAME'] = 'Renames the playlist.' +
    '<ul>' +
    '<li>from: old playlist name</li>' +
    '<li>to: new playlist name</li>' +
    '</ul>';
desc['MYMPD_API_PLAYLIST_MOVE_TRACK'] = 'Moves a song in the playlist.' +
    '<ul>' +
    '<li>plist: playlist</li>' +
    '<li>from: from position</li>' +
    '<li>to: to position</li>' +
    '</ul>';
desc['MYMPD_API_PLAYLIST_ADD_TRACK'] = 'Adds a song to the playlist.' +
    '<ul>' +
    '<li>plist: playlist</li>' +
    '<li>uri: song uri to add</li>' +
    '</ul>';
desc['MYMPD_API_PLAYLIST_RM_TRACK'] = 'Removes a song from the playlist.' +
    '<ul>' +
    '<li>uri: playlist</li>' +
    '<li>track: song number to remove</li>' +
    '</ul>';
desc['MYMPD_API_PLAYLIST_RM_ALL'] = 'Removes all playlists.' +
    '<ul>' +
    '<li>type: <ul>' +
      '<li>deleteAllPlaylists: deletes all playlists</li>' +
      '<li>deleteSmartPlaylists: deletes all smart playlists</li>' +
      '<li>deleteEmptyPlaylists: deletes all empty playlists</li>' +
    '</ul></li>' +
    '</ul>';
desc['MYMPD_API_PLAYLIST_LIST'] = 'Lists all playlists (paginated).' +
    '<ul>' +
    '<li>' + strings['offset'] + '</li>' +
    '<li>' + strings['limit'] + '</li>' +
    '<li>searchstr: string to search</li>' +
    '</ul>';
desc['MYMPD_API_PLAYLIST_CONTENT_LIST'] = 'Lists the content of a playlist.' +
    '<ul>' +
    '<li>uri: playlist</li>' +
    '<li>' + strings['offset'] + '</li>' +
    '<li>' + strings['limit'] + '</li>' +
    '<li>searchstr: string to search</li>' +
    '<li>' + strings['cols'] + '</li>' +
    '</ul>';
desc['MYMPD_API_PLAYLIST_SHUFFLE'] = 'Shuffles the playlist.' +
    '<ul>' +
    '<li>uri: playlist</li>' +
    '</ul>';
desc['MYMPD_API_PLAYLIST_SORT'] = 'Sorts the playlist.' +
    '<ul>' +
    '<li>uri: playlist</li>' +
    '<li>tag: tag to sort</li>' +
    '</ul>';
desc['MYMPD_API_SMARTPLS_UPDATE_ALL'] = 'Updates all smart playlists.' +
    '<ul>' +
    '<li>force: <ul>' +
      '<li>true: updates all smart playlists' +
      '<li>false: updates smart playlists only if nedded' +
    '</li>' +
    '</ul>';
desc['MYMPD_API_SMARTPLS_UPDATE'] = 'Updates the smart playlist.' +
    '<ul>' +
    '<li>playlist: playlist to update</li>' +
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
