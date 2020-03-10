"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

var i = 0;
var error = 0;
var warn = 0;
var ok = 0;
var trackId = 0;
var outputId = 0;
var timerId = 0;
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
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_CLEAR"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_SEARCH","params":{"offset":0,"filter":"any","searchstr":searchstr,"plist":"","cols":["Title","Album"],"replace":false}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_TRACK","params":{"uri":"uri1"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_SEARCH","params":{"offset":0,"filter":"any","searchstr":searchstr,"plist":"queue","cols":["Title","Album"],"replace":false}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_LIST","params":{"offset":0,"cols":["Title","Album"]}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_MOVE_TRACK","params":{"from":1,"to":2}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_RANDOM","params":{"playlist":"Database","quantity":2, "mode":1}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_RANDOM","params":{"playlist":"Database","quantity":1, "mode":2}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_RM_RANGE","params":{"start":1,"end":-1}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_PLAY_TRACK","params":{"track":trackId}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_SEEK","params":{"songid":trackId,"seek":10}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_SEEK_CURRENT","params":{"seek":10,"realtive":true}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_SEEK_CURRENT","params":{"seek":10,"realtive":false}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_RM_TRACK","params":{"track":trackId}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_SAVE","params":{"plist":"test"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_SEARCH","params":{"offset":0,"filter":"any","searchstr":searchstr,"cols":["Title","Album"],"replace":false}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_PLAY_TRACK","params":{"uri":"uri2"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_SHUFFLE"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_REPLACE_TRACK","params":{"uri":"uri1"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_TRACK_AFTER","params":{"uri":"uri1","to":1}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_RENAME","params":{"from":"test","to":"test2"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_CONTENT_LIST","params":{"uri":"test2","offset":0,"filter":"","cols":["Title","Album"]}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_PLAYLIST","params":{"plist":"test2"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_REPLACE_PLAYLIST","params":{"plist":"test2"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_LIST","params":{"offset":0,"filter":""}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_CLEAR","params":{"uri":"test2"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_SEARCH","params":{"offset":0,"filter":"any","searchstr":searchstr,"plist":"test2","cols":["Title","Album","Artist"],"replace":false}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_TRACK","params":{"uri":"uri1"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_TRACK","params":{"uri":"uri2"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_PLAY"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_CROP"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_LAST_PLAYED","params":{"offset":0,"cols":["Artist","Album","AlbumArtist"]}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_ADD_TRACK","params":{"plist":"test2","uri":"uri1"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_ADD_TRACK","params":{"plist":"test2","uri":"uri1"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_MOVE_TRACK","params":{"plist":"test2","from":1,"to":2}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_RM_TRACK","params":{"uri":"test2","track":1}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_RM","params":{"uri":"test2"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_SEARCH_ADV","params":{"offset":0,"expression":"(any contains '"+searchstr+"')","sort":"Title", "sortdesc":false,"plist":"","cols":["Title","Album","Artist"],"replace":false}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_UPDATE","params":{"uri":""}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_RESCAN","params":{"uri":""}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_FILESYSTEM_LIST","params":{"offset":0,"filter":"","path":"","cols":["Title","Album"]}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_TAG_LIST","params":{"offset":0,"filter":"","tag":"Artist"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_TAG_ALBUM_LIST","params":{"offset":0,"filter":"","search":"artist1","tag":"Artist"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_TAG_ALBUM_TITLE_LIST","params":{"album":"album1","search":"artist1","tag":"Artist","cols":["Title","Album"]}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_GET_ALBUMS","params":{"offset":0, "searchstr":"artist1", "tag":"Artist", "sort":"Artist", "sortdesc":false}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_STATS"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_SONGDETAILS","params":{"uri":"uri1"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_FINGERPRINT","params":{"uri":"uri1"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_VOLUME_SET","params":{"volume":30}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_VOLUME_GET"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_NEXT"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_CURRENT_SONG"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_PREV"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_PAUSE"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_STOP"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_OUTPUT_LIST"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_TOGGLE_OUTPUT","params":{"output":outputId,"state":1}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_STATE"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_LIKE","params":{"uri":"uri2","like":2}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_SETTINGS_RESET"},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_SETTINGS_GET"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_SETTINGS_GET"},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_SETTINGS_SET","params":{"random": 0}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_SYSCMD","params":{"cmd": "Echo"}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_COLS_SAVE","params":{"table":"colsPlayback","cols":["Artist","Album","AlbumArtist"]}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_SMARTPLS_UPDATE_ALL"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_SMARTPLS_SAVE","params":{"type":"newest","playlist":"myMPDsmart-newestSongs","timerange":2678400,"sort":""}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_SMARTPLS_GET","params":{"playlist":"myMPDsmart-newestSongs"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_SMARTPLS_UPDATE", "params":{"playlist":"myMPDsmart-newestSongs"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_RM_ALL", "params":{"type":"deleteEmptyPlaylists"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_SORT", "params":{"uri":"myMPDsmart-newestSongs","tag":"Artist"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_SHUFFLE", "params":{"uri":"myMPDsmart-newestSongs"}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_BOOKMARK_SAVE","params":{"id": -1, "name":"testdir1", "uri":"parent1/child1", "type": "dir"}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_BOOKMARK_SAVE","params":{"id": -1, "name":"testdir2", "uri":"parent2/child2", "type": "dir"}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_BOOKMARK_SAVE","params":{"id": 2, "name":"testdir2id2", "uri":"parent1/child1/child2", "type": "dir"}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_BOOKMARK_LIST","params":{"offset":0}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_BOOKMARK_RM","params":{"id":1}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_BOOKMARK_CLEAR"},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_TIMER_SAVE","params":{"timerid": 0, "name": "test", "enabled": true, "startHour": 10, "startMinute": 15, "action": "startPlay", "volume": 40, "playlist": "Database", "jukeboxMode": 1, "weekdays":[false,false,true,true,false,false,false]}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_TIMER_LIST"},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_TIMER_GET","params":{"timerid":timerId}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_TIMER_TOGGLE","params":{"timerid":timerId}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_TIMER_RM","params":{"timerid":timerId}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_MOUNT_MOUNT","params":{"mountUrl":"udisks:///dev/null","mountPoint":"apitest"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_MOUNT_LIST"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_MOUNT_UNMOUNT","params":{"mountPoint":"apitest"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_MOUNT_NEIGHBOR_LIST"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_URLHANDLERS"}
];

function setTest(cmd, state, response) {
    if (state === 'ok') {
        ok++;
    }
    else if (state === 'warn') {
        warn++;
    }
    else {
        error++;
    }
    var duration = time_end - time_start;
    time_all += duration;
    document.getElementById('testCount').innerText = 'Test ' + (i + 1) + '/' + cmds.length + ' - ' +
        ok + ' ok, ' + warn + ' warnings, ' + error + ' errors, duration: ' + time_all + ' ms';
    var tr = document.createElement('tr');
    tr.innerHTML = '<td>' + (i + 1) + '</td><td>' + JSON.stringify(cmd) + '</td><td>' + duration + ' ms</td><td>' + response + '</td>';
    tr.childNodes[2].style.backgroundColor = (state === 'ok' ? 'green' : (state === 'warn' ? 'yellow' : 'red'));
    document.getElementsByTagName('tbody')[0].appendChild(tr);
}

function sendAPI(request) {
    if (request.params !== undefined) {
        if (request.params.uri !== undefined) { 
            if (request.params.uri === 'uri1') { request.params.uri = uri1; }
            if (request.params.uri === 'uri2') { request.params.uri = uri2; }
        }
        if (request.params.album !== undefined) { 
            if (request.params.album === 'album1') { request.params.album = album1; }
            if (request.params.album === 'album2') { request.params.uri = album2; }
        }
        if (request.params.search !== undefined) { 
            if (request.params.search === 'artist1') { request.params.search = artist1; }
            if (request.params.search === 'artist2') { request.params.search = artist2; }
        }
        if (request.params.trackId !== undefined) { request.params.trackId = trackId; }
        if (request.params.outputId !== undefined) { request.params.outputId = outputId; }
    }

    var ajaxRequest = new XMLHttpRequest();
    ajaxRequest.open('POST', '/api', true);
    ajaxRequest.setRequestHeader('Content-type', 'application/json');
    ajaxRequest.onreadystatechange = function() {
        if (ajaxRequest.readyState === 4) {
            if (ajaxRequest.responseText !== '') {
                var obj;
                try {
                    obj = JSON.parse(ajaxRequest.responseText);
                    time_end = new Date().getTime();
                    if (!obj.error && !obj.result && !obj.jsonrpc) {
                        setTest(request, 'error', 'Invalid response: ' + ajaxRequest.responseText);
                    }
                    else {
                        if (obj.result) {
                            if (obj.result.method === 'MPD_API_DATABASE_SEARCH' && obj.result.data && obj.result.data.length > 1) {
                                uri1 = obj.result.data[0].uri;
                                artist1 = obj.result.data[0].Artist;
                                album1 = obj.result.data[0].Album;
                                uri2 = obj.result.data[1].uri;
                                artist2 = obj.result.data[1].Artist;
                                album2 = obj.result.data[1].Album;
                            }
                            else if (obj.result.method === 'MPD_API_QUEUE_LIST' && obj.result.data.length > 0) {
                                trackId = obj.result.data[0].id;
                            }
                            else if (obj.result.method === 'MPD_API_PLAYER_OUTPUT_LIST' && obj.result.data.length > 0) {
                                outputId = obj.result.data[0].id;
                            }
                            else if (obj.result.method === 'MPD_API_TIMER_LIST' && obj.result.data.length > 0) {
                                timerId = obj.result.data[0].timerid;
                            }
                            setTest(request, 'ok', ajaxRequest.responseText);
                        } 
                        else {
                            setTest(request, 'warn', ajaxRequest.responseText);
                        }
                    }
                }
                catch(e) {
                    setTest(request, 'error', '<p>JSON parse error: ' + e + '</p><small>' + ajaxRequest.responseText + '</small>');
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
                document.getElementsByTagName('h5')[0].innerText = 'Finished';
            }
        }
    };
    document.getElementsByTagName('h5')[0].innerText = 'Running ' + JSON.stringify(request);
    time_start = new Date().getTime();
    ajaxRequest.send(JSON.stringify(request));
}

sendAPI(cmds[i]);
