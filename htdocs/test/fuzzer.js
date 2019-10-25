"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

var i = 0;
var j = 0;
var blns_len = blns.length;

var cmds = [
    '{"jsonrpc":"2.0","id":0,"method":"MYMPD_API_BOOKMARK_SAVE","params":{"id": __INTEGER__, "name":"__STRING__", "uri":"__STRING__", "type":"__STRING__"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MYMPD_API_BOOKMARK_LIST","params":{"offset":__INTEGER__}}',
    '{"jsonrpc":"2.0","id":0,"method":"MYMPD_API_BOOKMARK_RM","params":{"id":__INTEGER__}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_SEARCH","params":{"offset":__INTEGER__,"filter":"__STRING__","searchstr":"__STRING__","plist":"__STRING__","cols":["__STRING__","__STRING__"]}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_TRACK","params":{"uri":"__URI1__"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_SEARCH","params":{"offset":__INTEGER__,"filter":"__STRING__","searchstr":"__STRING__","plist":"__STRING__","cols":["__STRING__","__STRING__"]}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_LIST","params":{"offset":__INTEGER__,"cols":["__STRING__","__STRING__"]}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_MOVE_TRACK","params":{"from":__INTEGER__,"to":__INTEGER__}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_RANDOM","params":{"playlist":"Database","quantity":__INTEGER__, "mode":__INTEGER__}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_RANDOM","params":{"playlist":"Database","quantity":__INTEGER__, "mode":__INTEGER__}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_RM_RANGE","params":{"start":__INTEGER__,"end":__INTEGER__}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_PLAY_TRACK","params":{"track":__INTEGER__}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_SEEK","params":{"songid":__INTEGER__,"seek":__INTEGER__}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_RM_TRACK","params":{"track":__INTEGER__}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_SAVE","params":{"plist":"__STRING__"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_SEARCH","params":{"offset":__INTEGER__,"filter":"__STRING__","searchstr":"__STRING__","cols":["__STRING__","__STRING__"]}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_PLAY_TRACK","params":{"uri":"__STRING__"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_REPLACE_TRACK","params":{"uri":"__STRING__"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_TRACK_AFTER","params":{"uri":"__STRING__","to":__INTEGER__}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_RENAME","params":{"from":"__STRING__","to":"__STRING__"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_CONTENT_LIST","params":{"uri":"__STRING__","offset":__INTEGER__,"filter":"__STRING__","cols":["__STRING__","__STRING__"]}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_PLAYLIST","params":{"plist":"__STRING__"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_REPLACE_PLAYLIST","params":{"plist":"__STRING__"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_LIST","params":{"offset":__INTEGER__,"filter":"__STRING__"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_CLEAR","params":{"uri":"__STRING__"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_SEARCH","params":{"offset":__INTEGER__,"filter":"__STRING__","searchstr":"__STRING____","plist":"__STRING__","cols":["__STRING__","__STRING__","__STRING__"]}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_TRACK","params":{"uri":"__STRING__"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_TRACK","params":{"uri":"__STRING__"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_LAST_PLAYED","params":{"offset":__INTEGER__,"cols":["__STRING__","__STRING__","__STRING__"]}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_ADD_TRACK","params":{"plist":"__STRING__","uri":"__STRING__"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_MOVE_TRACK","params":{"plist":"__STRING__","from":__INTEGER__,"to":__INTEGER__}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_RM_TRACK","params":{"uri":"__STRING__","track":__INTEGER__}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_RM","params":{"uri":"__STRING__"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_SEARCH_ADV","params":{"offset":__INTEGER__,"expression":"(any contains \'__STRING__\')","sort":"__STRING__", "sortdesc":false,"plist":"__STRING__","cols":["__STRING__"]}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_SEARCH_ADV","params":{"offset":__INTEGER__,"expression":"__STRING__","sort":"__STRING__", "sortdesc":true,"plist":"__STRING__","cols":["__STRING__"]}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_FILESYSTEM_LIST","params":{"offset":__INTEGER__,"filter":"__STRING__","path":"__STRING__","cols":["__STRING__","__STRING__"]}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_TAG_LIST","params":{"offset":__INTEGER__,"filter":"__STRING__","tag":"__STRING__"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_TAG_ALBUM_LIST","params":{"offset":__INTEGER__,"filter":"__STRING__","search":"__STRING__","tag":"__STRING__"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_TAG_ALBUM_TITLE_LIST","params":{"album":"__STRING__","search":"__STRING__","tag":"__STRING__","cols":["__STRING__","__STRING__"]}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_SONGDETAILS","params":{"uri":"__STRING__"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_VOLUME_SET","params":{"volume":__INTEGER__}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_TOGGLE_OUTPUT","params":{"output":__INTEGER__,"state":__INTEGER__}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_LIKE","params":{"uri":"__STRING__","like":__INTEGER__}}',
    '{"jsonrpc":"2.0","id":0,"method":"MYMPD_API_SETTINGS_SET","params":{"random": __INTEGER__}}',
    '{"jsonrpc":"2.0","id":0,"method":"MYMPD_API_SYSCMD","params":{"cmd": "__STRING__"}}',
    '{"jsonrpc":"2.0","id":0,"method":"MYMPD_API_COLS_SAVE","params":{"table":"__STRING__","cols":["__STRING__,"__STRING__","__STRING__"]}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_SMARTPLS_SAVE","params":{"type":"__STRING__","playlist":"__STRING__","timerange":__INTEGER__}}',
    '{"jsonrpc":"2.0","id":0,"method":"MPD_API_SMARTPLS_GET","params":{"playlist":"__STRING__"}}'
];

function setTest(cmd, response) {
    var tr = document.createElement('tr');
    tr.innerHTML = '<td>' + (i + 1) + '</td><td>' + cmd + '</td><td>' + response + '</td>';
    document.getElementsByTagName('tbody')[0].appendChild(tr);
}

function getRandomUint(max) {
    return Math.floor(Math.random() * Math.floor(max));
}

function getRandomInt() {
    var int = Math.floor(Math.random() * Math.floor(9999999999));
    if (getRandomUint(3) == 0) {
        int = 0 - int;
    }
    return int;
}

function sendAPI(request) {
    var ajaxRequest=new XMLHttpRequest();
    ajaxRequest.open('POST', '/api', true);
    ajaxRequest.setRequestHeader('Content-type', 'application/json');
    ajaxRequest.onreadystatechange = function() {
        if (ajaxRequest.readyState == 4) {
            setTest(request, ajaxRequest.responseText);
            i++;
            if (i < cmds.length) {
                sendAPI(cmds[i]);
            }
            else if (j < blns_len) {
                j++;
                i = 0;
                sendAPI(cmds[i]);
            }
        }
    };
    var myRequest = request.replace(/__STRING__/, blns[j]);
    myRequest = myRequest.replace(/__STRING__/g, function() { return blns[getRandomUint(blns_len)]; });
    myRequest = myRequest.replace(/__INTEGER__/g, function() { return getRandomInt(); });
    
    document.getElementsByTagName('h5')[0].innerText = 'Running ' + myRequest;
    time_start = new Date().getTime();
    ajaxRequest.send(myRequest);
}

sendAPI(cmds[i]);
