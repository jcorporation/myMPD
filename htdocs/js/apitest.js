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
    '{"cmd":"MYMPD_API_BOOKMARK_SAVE","data":{"name":"testdir", "uri":"test/blabla", "type": "dir"}}',
    '{"cmd":"MYMPD_API_BOOKMARK_SAVE","data":{"name":"testplaylist", "uri":"blabla.m3u", "type": "plist"}}',
    '{"cmd":"MYMPD_API_BOOKMARK_SAVE","data":{"name":"testsong", "uri":"tes/blabla/blabla.mp3", "type": "song"}}',
    '{"cmd":"MYMPD_API_BOOKMARK_LIST","data":{"offset":0}}',
    '{"cmd":"MYMPD_API_BOOKMARK_RM","data":{"id":1}}',
    '{"cmd":"MPD_API_QUEUE_CLEAR"}',
    '{"cmd":"MPD_API_DATABASE_SEARCH","data":{"offset":0,"filter":"any","searchstr":"__SEARCHSTR__","plist":""}}',
    '{"cmd":"MPD_API_QUEUE_ADD_TRACK","data":{"uri":"__URI1__"}}',
    '{"cmd":"MPD_API_DATABASE_SEARCH","data":{"offset":0,"filter":"any","searchstr":"__SEARCHSTR__","plist":"queue"}}',
    '{"cmd":"MPD_API_QUEUE_LIST","data":{"offset":0}}',
    '{"cmd":"MPD_API_QUEUE_MOVE_TRACK","data":{"from":1,"to":2}}',
    '{"cmd":"MPD_API_QUEUE_RM_RANGE","data":{"start":1,"end":-1}}',
    '{"cmd":"MPD_API_PLAYER_PLAY_TRACK","data":{"track":__TRACKID__}}',
    '{"cmd":"MPD_API_PLAYER_SEEK","data":{"songid":__TRACKID__,"seek":10}}',
    '{"cmd":"MPD_API_QUEUE_RM_TRACK","data":{"track":__TRACKID__}}',
    '{"cmd":"MPD_API_QUEUE_SAVE","data":{"plist":"test"}}',
    '{"cmd":"MPD_API_QUEUE_SEARCH","data":{"offset":0,"filter":"any","searchstr":"__SEARCHSTR__"}}',
    '{"cmd":"MPD_API_QUEUE_ADD_PLAY_TRACK","data":{"uri":"__URI2__"}}',
    '{"cmd":"MPD_API_QUEUE_SHUFFLE"}',
    '{"cmd":"MPD_API_QUEUE_REPLACE_TRACK","data":{"uri":"__URI1__"}}',
    '{"cmd":"MPD_API_QUEUE_ADD_TRACK_AFTER","data":{"uri":"__URI1__","to":1}',
    '{"cmd":"MPD_API_PLAYLIST_RENAME","data":{"from":"test","to":"test2"}}',
    '{"cmd":"MPD_API_PLAYLIST_CONTENT_LIST","data":{"uri":"test2","offset":0,"filter":""}}',
    '{"cmd":"MPD_API_QUEUE_ADD_PLAYLIST","data":{"plist":"test2"}}',
    '{"cmd":"MPD_API_QUEUE_REPLACE_PLAYLIST","data":{"plist":"test2"}}',
    '{"cmd":"MPD_API_PLAYLIST_LIST","data":{"offset":0,"filter":""}}',
    '{"cmd":"MPD_API_PLAYLIST_CLEAR","data":{"uri":"test2"}}',
    '{"cmd":"MPD_API_DATABASE_SEARCH","data":{"offset":0,"filter":"any","searchstr":"__SEARCHSTR__","plist":"test2"}}',
    '{"cmd":"MPD_API_QUEUE_ADD_TRACK","data":{"uri":"__URI1__"}}',
    '{"cmd":"MPD_API_QUEUE_ADD_TRACK","data":{"uri":"__URI2__"}}',
    '{"cmd":"MPD_API_PLAYER_PLAY"}',
    '{"cmd":"MPD_API_QUEUE_CROP"}',
    '{"cmd":"MPD_API_QUEUE_LAST_PLAYED","data":{"offset":0}}',
    '{"cmd":"MPD_API_PLAYLIST_ADD_TRACK","data":{"plist":"test2","uri":"__URI1__"}}',
    '{"cmd":"MPD_API_PLAYLIST_ADD_TRACK","data":{"plist":"test2","uri":"__URI1__"}}',
    '{"cmd":"MPD_API_PLAYLIST_MOVE_TRACK","data":{"plist":"test2","from":1,"to":2}}',
    '{"cmd":"MPD_API_PLAYLIST_RM_TRACK","data":{"uri":"test2","track":1}',
    '{"cmd":"MPD_API_PLAYLIST_RM","data":{"uri":"test2"}}',
    '{"cmd":"MPD_API_DATABASE_SEARCH_ADV","data":{"offset":0,"expression":"(any contains \'__SEARCHSTR__\')","sort":"Title", "sortdesc":false,"plist":""}}',
    '{"cmd":"MPD_API_DATABASE_UPDATE"}',
//    '{"cmd":"MPD_API_DATABASE_RESCAN"}',
    '{"cmd":"MPD_API_DATABASE_FILESYSTEM_LIST","data":{"offset":0,"filter":"","path":""}}',
    '{"cmd":"MPD_API_DATABASE_TAG_LIST","data":{"offset":0,"filter":"","tag":"Artist"}}',
    '{"cmd":"MPD_API_DATABASE_TAG_ALBUM_LIST","data":{"offset":0,"filter":"","search":"__ARTIST1__","tag":"Artist"}}',
    '{"cmd":"MPD_API_DATABASE_TAG_ALBUM_TITLE_LIST","data":{"album":"__ALBUM1__","search":"__ARTIST1__","tag":"Artist"}}',
    '{"cmd":"MPD_API_DATABASE_STATS"}',
    '{"cmd":"MPD_API_DATABASE_SONGDETAILS","data":{"uri":"__URI1__"}}',
    '{"cmd":"MPD_API_PLAYER_VOLUME_SET","data":{"volume":30}}',
    '{"cmd":"MPD_API_PLAYER_VOLUME_GET"}',
    '{"cmd":"MPD_API_PLAYER_NEXT"}',
    '{"cmd":"MPD_API_PLAYER_CURRENT_SONG"}',
    '{"cmd":"MPD_API_PLAYER_PREV"}',
    '{"cmd":"MPD_API_PLAYER_PAUSE"}',
    '{"cmd":"MPD_API_PLAYER_STOP"}',
    '{"cmd":"MPD_API_PLAYER_OUTPUT_LIST"}',
    '{"cmd":"MPD_API_PLAYER_TOGGLE_OUTPUT","data":{"output":"__OUTPUTID__","state":1}}',
    '{"cmd":"MPD_API_PLAYER_STATE"}',
    '{"cmd":"MYMPD_API_SETTINGS_GET"}',
    '{"cmd":"MPD_API_SETTINGS_GET"}',
    '{"cmd":"MYMPD_API_SETTINGS_SET","data":{"random": 0}}',
    '{"cmd":"MPD_API_LIKE","data":{"uri":"__URI2__","like":2}}',
    '{"cmd":"MYMPD_API_SYSCMD","data":{"cmd": "Echo"}}',
    '{"cmd":"MYMPD_API_COLS_SAVE","data":{"table":"colsPlayback","cols":["Artist","Album","AlbumArtist"]}}',
    '{"cmd":"MPD_API_SMARTPLS_UPDATE_ALL"}',
    '{"cmd":"MPD_API_SMARTPLS_SAVE","data":{"type":"newest","playlist":"myMPDsmart-newestSongs","timerange":2678400}}',
    '{"cmd":"MPD_API_SMARTPLS_GET","data":{"playlist":"myMPDsmart-newestSongs"}}'
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
    tr.innerHTML = '<td>' + (i + 1) + '</td><td>' + cmd + '</td><td class="td-' + state + '">' + duration + ' ms</td><td>' + response + '</td>';
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
                    if (!obj.type)
                        setTest(request, 'error', 'Invalid response: ' + ajaxRequest.responseText);
                    else {
                        if (obj.type == 'search' && obj.data.length > 1) {
                            uri1 = obj.data[0].uri;
                            artist1 = obj.data[0].Artist;
                            album1 = obj.data[0].Album;
                            uri2 = obj.data[1].uri;
                            artist2 = obj.data[1].Artist;
                            album2 = obj.data[1].Album;
                        }
                        else if (obj.type == 'queue' && obj.data.length > 0)
                            trackId = obj.data[0].id;
                        else if (obj.type == 'outputs' && obj.data.length > 0)
                            ouputId = obj.data[0].id;
                        
                        if (obj.type == 'error')
                            setTest(request, 'error', ajaxRequest.responseText);
                        else 
                            setTest(request, 'ok', ajaxRequest.responseText);
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
            if (i < cmds.length)
                sendAPI(cmds[i]);
            else
                document.getElementsByTagName('h2')[0].innerText = 'Finished';
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
