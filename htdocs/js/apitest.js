"use strict";
/* myMPD
   (c) 2018 Juergen Mang <mail@jcgames.de>
   This project's homepage is: https://github.com/jcorporation/mympd
   
   myMPD ist fork of:

   ympd
   (c) 2013-2014 Andrew Karpow <andy@ndyk.de>
   This project's homepage is: https://www.ympd.org
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

var i = 0;
var failed = 0;
var ok = 0;
var trackId = 0;
var outputId = 0;
var uri1 = '';
var album1 = '';
var artist1 = '';
var uri2 = '';
var album2 = '';
var artist2 = '';
var searchstr = 'tabula';
var time_start = 0;
var time_end = 0;
var time_all = 0;

var cmds = [
    '{"jsonrpc":"2.0","id":0,"method":"MYMPD_API_BOOKMARK_SAVE","params":{"id": -1, "name":"testdir1", "uri":"parent1/child1", "type": "dir"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MYMPD_API_BOOKMARK_SAVE","params":{"id": -1, "name":"testdir2", "uri":"parent2/child2", "type": "dir"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MYMPD_API_BOOKMARK_SAVE","params":{"id": 2, "name":"testdir2id2", "uri":"parent1/child1/child2", "type": "dir"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MYMPD_API_BOOKMARK_LIST","params":{"offset":0}}',
    '{"jsonrpc":"2.0","id":0,"method":"MYMPD_API_BOOKMARK_RM","params":{"id":1}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_CLEAR"}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_SEARCH","params":{"offset":0,"filter":"any","searchstr":"__SEARCHSTR__","plist":"","cols":["Title","Album"]}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_TRACK","params":{"uri":"__URI1__"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_SEARCH","params":{"offset":0,"filter":"any","searchstr":"__SEARCHSTR__","plist":"queue","cols":["Title","Album"]}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_LIST","params":{"offset":0,"cols":["Title","Album"]}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_MOVE_TRACK","params":{"from":1,"to":2}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_RANDOM","params":{"playlist":"Database","quantity":2, "mode":1}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_RANDOM","params":{"playlist":"Database","quantity":1, "mode":2}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_RM_RANGE","params":{"start":1,"end":-1}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_PLAY_TRACK","params":{"track":__TRACKID__}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_SEEK","params":{"songid":__TRACKID__,"seek":10}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_RM_TRACK","params":{"track":__TRACKID__}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_SAVE","params":{"plist":"test"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_SEARCH","params":{"offset":0,"filter":"any","searchstr":"__SEARCHSTR__","cols":["Title","Album"]}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_PLAY_TRACK","params":{"uri":"__URI2__"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_SHUFFLE"}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_REPLACE_TRACK","params":{"uri":"__URI1__"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_TRACK_AFTER","params":{"uri":"__URI1__","to":1}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_RENAME","params":{"from":"test","to":"test2"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_CONTENT_LIST","params":{"uri":"test2","offset":0,"filter":"","cols":["Title","Album"]}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_PLAYLIST","params":{"plist":"test2"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_REPLACE_PLAYLIST","params":{"plist":"test2"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_LIST","params":{"offset":0,"filter":""}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_CLEAR","params":{"uri":"test2"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_SEARCH","params":{"offset":0,"filter":"any","searchstr":"__SEARCHSTR__","plist":"test2","cols":["Title","Album","Artist"]}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_TRACK","params":{"uri":"__URI1__"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_TRACK","params":{"uri":"__URI2__"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_PLAY"}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_CROP"}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_LAST_PLAYED","params":{"offset":0,"cols":["Artist","Album","AlbumArtist"]}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_ADD_TRACK","params":{"plist":"test2","uri":"__URI1__"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_ADD_TRACK","params":{"plist":"test2","uri":"__URI1__"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_MOVE_TRACK","params":{"plist":"test2","from":1,"to":2}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_RM_TRACK","params":{"uri":"test2","track":1}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_RM","params":{"uri":"test2"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_SEARCH_ADV","params":{"offset":0,"expression":"(any contains \'__SEARCHSTR__\')","sort":"Title", "sortdesc":false,"plist":"","cols":["Title","Album","Artist"]}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_UPDATE"}',
//    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_RESCAN"}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_FILESYSTEM_LIST","params":{"offset":0,"filter":"","path":"","cols":["Title","Album"]}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_TAG_LIST","params":{"offset":0,"filter":"","tag":"Artist"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_TAG_ALBUM_LIST","params":{"offset":0,"filter":"","search":"__ARTIST1__","tag":"Artist"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_TAG_ALBUM_TITLE_LIST","params":{"album":"__ALBUM1__","search":"__ARTIST1__","tag":"Artist","cols":["Title","Album"]}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_STATS"}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_SONGDETAILS","params":{"uri":"__URI1__"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_VOLUME_SET","params":{"volume":30}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_VOLUME_GET"}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_NEXT"}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_CURRENT_SONG"}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_PREV"}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_PAUSE"}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_STOP"}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_OUTPUT_LIST"}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_TOGGLE_OUTPUT","params":{"output":"__OUTPUTID__","state":1}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_STATE"}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_LIKE","params":{"uri":"__URI2__","like":2}}',
    '{"jsonrpc":"2.0","id":0,"method":"MYMPD_API_SETTINGS_GET"}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_SETTINGS_GET"}',
    '{"jsonrpc":"2.0","id":0,"method":"MYMPD_API_SETTINGS_SET","params":{"random": 0}}',
    '{"jsonrpc":"2.0","id":0,"method":"MYMPD_API_SYSCMD","params":{"cmd": "Echo"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MYMPD_API_COLS_SAVE","params":{"table":"colsPlayback","cols":["Artist","Album","AlbumArtist"]}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_SMARTPLS_UPDATE_ALL"}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_SMARTPLS_SAVE","params":{"type":"newest","playlist":"myMPDsmart-newestSongs","timerange":2678400}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_SMARTPLS_GET","params":{"playlist":"myMPDsmart-newestSongs"}}'
];

function setTest(cmd, state, response) {
    if (state == "ok")
        ok++;
    else
        failed++;
    var duration = time_end - time_start;
    time_all += duration;
    document.getElementById('testCount').innerText = 'Test ' + (i + 1) + '/' + cmds.length + ' - ' +
        ok + ' ok, ' + failed + ' failed, duration: ' + time_all + ' ms';
    var tr = document.createElement('tr');
    tr.innerHTML = '<td>' + (i + 1) + '</td><td>' + cmd + '</td><td>' + duration + ' ms</td><td>' + response + '</td>';
    tr.childNodes[2].style.backgroundColor = (state == 'ok' ? 'green' : 'red');
    document.getElementsByTagName('tbody')[0].appendChild(tr);
}

function sendAPI(request, callback) {
    var ajaxRequest=new XMLHttpRequest();
    ajaxRequest.open('POST', '/api', true);
    ajaxRequest.setRequestHeader('Content-type', 'application/json');
    ajaxRequest.onreadystatechange = function() {
        if (ajaxRequest.readyState == 4) {
            if (ajaxRequest.responseText != '') {
                var obj;
                try {
                    obj = JSON.parse(ajaxRequest.responseText);
                    time_end = new Date().getTime();
                    if (!obj.error && !obj.result && !obj.jsonrpc) {
                        setTest(request, 'error', 'Invalid response: ' + ajaxRequest.responseText);
                    }
                    else {
                        if (obj.result) {
                            if (obj.result.method == 'MPD_API_DATABASE_SEARCH' && obj.result.data && obj.result.data.length > 1) {
                                uri1 = obj.result.data[0].uri;
                                artist1 = obj.result.data[0].Artist;
                                album1 = obj.result.data[0].Album;
                                uri2 = obj.result.data[1].uri;
                                artist2 = obj.result.data[1].Artist;
                                album2 = obj.result.data[1].Album;
                            }
                            else if (obj.result.method == 'MPD_API_QUEUE_LIST' && obj.result.data.length > 0) {
                                trackId = obj.result.data[0].id;
                            }
                            else if (obj.result.method == 'MPD_API_PLAYER_OUTPUT_LIST' && obj.result.data.length > 0) {
                                outputId = obj.result.data[0].id;
                            }
                            setTest(request, 'ok', ajaxRequest.responseText);
                        } 
                        else {
                            setTest(request, 'error', ajaxRequest.responseText);
                        }
                    }
                }
                catch(e) {
                    setTest(request, 'error', 'JSON parse error: ' + e);
                }
            }
            else {
                setTest(request, 'error', ajaxRequest.responseText);
            }
            i++;
            if (i < cmds.length) {
                sendAPI(cmds[i]);
            }
            else {
                document.getElementsByTagName('h2')[0].innerText = 'Finished';
            }
        }
    };
    request = request.replace(/__TRACKID__/, trackId);
    request = request.replace(/__OUTPUTID__/, outputId);
    request = request.replace(/__URI1__/, uri1);
    request = request.replace(/__ARTIST1__/, artist1);
    request = request.replace(/__ALBUM1__/, album1);
    request = request.replace(/__URI2__/, uri2);
    request = request.replace(/__ARTIST2__/, artist2);
    request = request.replace(/__ALBUM2__/, album2);
    request = request.replace(/__SEARCHSTR__/, searchstr);
    document.getElementsByTagName('h2')[0].innerText = 'Running ' + request;
    time_start = new Date().getTime();
    ajaxRequest.send(request);
}

sendAPI(cmds[i]);
