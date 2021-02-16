"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
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
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_SEARCH","params":{"offset":0,"limit":10,"filter":"any","searchstr":searchstr,"plist":"","cols":["Title","Album","Artist"],"replace":false}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_TRACK","params":{"uri":"uri1"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_SEARCH","params":{"offset":0,"limit":10,"filter":"any","searchstr":searchstr,"plist":"queue","cols":["Title","Album","Artist"],"replace":false}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_LIST","params":{"offset":0,"limit":10,"cols":["Title","Album"]}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_MOVE_TRACK","params":{"from":1,"to":2}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_RANDOM","params":{"playlist":"Database","quantity":2, "mode":1}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_RANDOM","params":{"playlist":"Database","quantity":1, "mode":2}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_RM_RANGE","params":{"start":1,"end":-1}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_PLAY_TRACK","params":{"track":trackId}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_SEEK","params":{"songid":trackId,"seek":10}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_SEEK_CURRENT","params":{"seek":10,"relative":true}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_SEEK_CURRENT","params":{"seek":10,"relative":false}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_RM_TRACK","params":{"track":trackId}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_SAVE","params":{"plist":"test"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_SEARCH","params":{"offset":0,"limit":10,"filter":"any","searchstr":searchstr,"cols":["Title","Album"],"replace":false}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_PLAY_TRACK","params":{"uri":"uri2"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_SHUFFLE"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_REPLACE_TRACK","params":{"uri":"uri1"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_TRACK_AFTER","params":{"uri":"uri1","to":1}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_RENAME","params":{"from":"test","to":"test2"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_CONTENT_LIST","params":{"uri":"test2","offset":0,"limit":10,"searchstr":"","cols":["Title","Album"]}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_PLAYLIST","params":{"plist":"test2"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_REPLACE_PLAYLIST","params":{"plist":"test2"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_LIST","params":{"offset":0,"limit":10,"searchstr":""}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_CLEAR","params":{"uri":"test2"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_SEARCH","params":{"offset":0,"limit":10,"filter":"any","searchstr":searchstr,"plist":"test2","cols":["Title","Album","Artist"],"replace":false}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_TRACK","params":{"uri":"uri1"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_TRACK","params":{"uri":"uri2"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_PLAY"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_CROP"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_LAST_PLAYED","params":{"offset":0,"limit":10,"cols":["Artist","Album","AlbumArtist"]}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_ADD_TRACK","params":{"plist":"test2","uri":"uri1"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_ADD_TRACK","params":{"plist":"test2","uri":"uri1"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_MOVE_TRACK","params":{"plist":"test2","from":1,"to":2}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_RM_TRACK","params":{"uri":"test2","track":1}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_RM","params":{"uri":"test2"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_SEARCH_ADV","params":{"offset":0,"limit":10,"expression":"(any contains '"+searchstr+"')","sort":"Title", "sortdesc":false,"plist":"","cols":["Title","Album","Artist"],"replace":false}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_UPDATE","params":{"uri":""}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_RESCAN","params":{"uri":""}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_FILESYSTEM_LIST","params":{"offset":0,"limit":10,"searchstr":"","path":"","cols":["Title","Album"]}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_GET_ALBUMS","params":{"offset":0,"limit":10, "searchstr":"artist1", "filter":"Artist", "sort":"Artist", "sortdesc":false}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_TAG_LIST","params":{"offset":0,"limit":10, "searchstr":"artist1", "filter":"Artist", "tag":"Artist", "sort":"Artist", "sortdesc":false}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_TAG_ALBUM_TITLE_LIST","params":{"album":"album1","searchstr":"artist1","tag":"Artist","cols":["Title","Album"]}},
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
    {"jsonrpc":"2.0","id":0,"method":"MPDWORKER_API_SMARTPLS_UPDATE_ALL", "params":{"force":false}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_SMARTPLS_SAVE","params":{"type":"newest","playlist":"myMPDsmart-newestSongs","timerange":2678400,"sort":""}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_SMARTPLS_GET","params":{"playlist":"myMPDsmart-newestSongs"}},
    {"jsonrpc":"2.0","id":0,"method":"MPDWORKER_API_SMARTPLS_UPDATE", "params":{"playlist":"myMPDsmart-newestSongs"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_RM_ALL", "params":{"type":"deleteEmptyPlaylists"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_SORT", "params":{"uri":"myMPDsmart-newestSongs","tag":"Artist"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_SHUFFLE", "params":{"uri":"myMPDsmart-newestSongs"}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_BOOKMARK_SAVE","params":{"id": -1, "name":"testdir1", "uri":"parent1/child1", "type": "dir"}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_BOOKMARK_SAVE","params":{"id": -1, "name":"testdir2", "uri":"parent2/child2", "type": "dir"}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_BOOKMARK_SAVE","params":{"id": 2, "name":"testdir2id2", "uri":"parent1/child1/child2", "type": "dir"}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_BOOKMARK_LIST","params":{"offset":0,"limit":10}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_BOOKMARK_RM","params":{"id":1}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_BOOKMARK_CLEAR"},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_TIMER_SAVE","params":{"timerid": 0, "name": "test", "enabled": true, "startHour": 10, "startMinute": 15, "action": "player", "subaction": "start", "volume": 40, "playlist": "Database", "jukeboxMode": 1, "weekdays":[false,false,true,true,false,false,false], "arguments":{}}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_TIMER_LIST"},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_TIMER_GET","params":{"timerid":timerId}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_TIMER_TOGGLE","params":{"timerid":timerId}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_TIMER_RM","params":{"timerid":timerId}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_MOUNT_MOUNT","params":{"mountUrl":"udisks:///dev/null","mountPoint":"apitest"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_MOUNT_LIST"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_MOUNT_UNMOUNT","params":{"mountPoint":"apitest"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_MOUNT_NEIGHBOR_LIST"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_URLHANDLERS"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_STOP"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_CROP_OR_CLEAR"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_MESSAGE_SEND","params":{"channel":"test", "message":"test"}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_SCRIPT_SAVE","params":{"script":"test","oldscript":"","order":0,"content":"return arguments[\"arg1\"]","arguments":["arg1","arg2"]}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_SCRIPT_EXECUTE","params":{"script":"test","arguments":{"arg1":"test1","arg2":"test2"}}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_SCRIPT_LIST","params":{"all":true}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_SCRIPT_GET","params":{"script":"test"}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_TIMER_SAVE","params":{"timerid": 0, "name": "test", "enabled": true, "startHour": 10, "startMinute": 15, "action": "script", "subaction": "test", "volume": 40, "playlist": "Database", "jukeboxMode": 1, "weekdays":[false,false,true,true,false,false,false], "arguments":{"arg1":"test1","arg2":"test2"}}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_SCRIPT_SAVE","params":{"script":"test1","oldscript":"test","order":0,"content":"return arguments[\"arg1\"]","arguments":["arg1","arg2"]}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_SCRIPT_DELETE","params":{"script":"test1"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PARTITION_LIST","params":{}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PARTITION_NEW","params":{"name":"testpartition"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PARTITION_SWITCH","params":{"name":"testpartition"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PARTITION_OUTPUT_MOVE","params":{"name":"HTTP Stream"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PARTITION_SWITCH","params":{"name":"default"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PARTITION_OUTPUT_MOVE","params":{"name":"HTTP Stream"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PARTITION_RM","params":{"name":"testpartition"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_TRIGGER_LIST","params":{}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_TRIGGER_SAVE","params":{"id":-1,"name":"test1","event":0,"script":"test"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_TRIGGER_GET","params":{"id":0}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_TRIGGER_DELETE","params":{"id":0}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_HOME_ICON_SAVE","params":{"replace":false,"oldpos":0,"name":"test1","ligature":"home","bgcolor":"#fff","image":"","cmd":"test1","options":["option1","option2"]}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_HOME_ICON_SAVE","params":{"replace":false,"oldpos":0,"name":"test2","ligature":"home","bgcolor":"#ccc","image":"test2.png","cmd":"test2","options":["option1"]}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_HOME_LIST","params":{}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_HOME_ICON_MOVE","params":{"from":0,"to":1}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_HOME_ICON_SAVE","params":{"replace":true,"oldpos":0,"name":"test3","ligature":"test","bgcolor":"#000","image":"test.png","cmd":"test3","options":["option1","option2"]}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_HOME_ICON_DELETE","params":{"pos":0}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_PICTURE_LIST","params":{}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_JUKEBOX_LIST","params":{"offset":"0","limit":10,"cols":["Pos","Title","Artist","Album"]}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_JUKEBOX_RM","params":{"pos":0}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_LYRICS_UNSYNCED_GET","params":{"uri":"uri1"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_LYRICS_SYNCED_GET","params":{"uri":"uri1"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_LYRICS_GET","params":{"uri":"uri1"}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_PRIO_SET_HIGHEST","params":{"trackid":trackId}}
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
        if (request.params.searchstr !== undefined) { 
            if (request.params.searchstr === 'artist1') { request.params.searchstr = artist1; }
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
