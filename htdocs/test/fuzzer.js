"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

var i = 0;
var j = 0;
var t = 0;
var blns_len = blns.length;
var cmds;

function defineCmds() {
    var int0 = getRandomInt();
    var int1 = getRandomInt();
    var int2 = getRandomInt();
    var int3 = getRandomInt();
    var int4 = getRandomInt();
    var string1 = blns[j];
    var string2 = blns[getRandomUint(blns_len)];
    var string3 = blns[getRandomUint(blns_len)];
    var string4 = blns[getRandomUint(blns_len)];
    var string5 = blns[getRandomUint(blns_len)];
    var bool0 = Math.random() >= 0.5;
    return [
        {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_BOOKMARK_SAVE","params":{"id": int1, "name":string1, "uri":string2, "type":string3}},
        {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_BOOKMARK_LIST","params":{"offset":int1}},
        {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_BOOKMARK_RM","params":{"id":int1}},
        {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_BOOKMARK_CLEAR"},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_UPDATE","params":{"uri":string1}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_RESCAN","params":{"uri":string1}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_SEARCH","params":{"offset":int1,"filter":string1,"searchstr":string2,"plist":string3,"cols":[string4,string5],"replace":bool0}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_TRACK","params":{"uri":string1}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_SEARCH","params":{"offset":int1,"filter":string1,"searchstr":string2,"plist":string3,"cols":[string4,string5],"replace":bool0}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_LIST","params":{"offset":int1,"cols":[string1,string2]}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_MOVE_TRACK","params":{"from":int1,"to":int2}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_RANDOM","params":{"playlist":"Database","quantity":int1, "mode":int2}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_RANDOM","params":{"playlist":"Database","quantity":int1, "mode":int2}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_RM_RANGE","params":{"start":int1,"end":int2}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_PLAY_TRACK","params":{"track":int1}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_SEEK","params":{"songid":int1,"seek":int2}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_SEEK_CURRENT","params":{"seek":int1,"realtive":bool0}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_RM_TRACK","params":{"track":int1}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_SAVE","params":{"plist":string1}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_SEARCH","params":{"offset":int1,"filter":string1,"searchstr":string2,"cols":[string3,string4]}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_PLAY_TRACK","params":{"uri":string1}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_REPLACE_TRACK","params":{"uri":string1}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_TRACK_AFTER","params":{"uri":string1,"to":int1}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_RENAME","params":{"from":string1,"to":string2}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_CONTENT_LIST","params":{"uri":string1,"offset":int1,"filter":string2,"cols":[string3,string4]}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_PLAYLIST","params":{"plist":string1}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_REPLACE_PLAYLIST","params":{"plist":string1}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_LIST","params":{"offset":int1,"filter":string1}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_CLEAR","params":{"uri":string1}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_SEARCH","params":{"offset":int1,"filter":string1,"searchstr":string2,"plist":string3,"cols":[string4,string5],"replace":bool0}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_TRACK","params":{"uri":string1}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_TRACK","params":{"uri":string1}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_LAST_PLAYED","params":{"offset":int1,"cols":[string1,string2,string3]}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_ADD_TRACK","params":{"plist":string1,"uri":string2}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_MOVE_TRACK","params":{"plist":string1,"from":int1,"to":int2}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_RM_TRACK","params":{"uri":string1,"track":int1}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_RM","params":{"uri":string1}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_SEARCH_ADV","params":{"offset":int1,"expression":"(any contains '"+string1+"')","sort":string2, "sortdesc":false,"plist":string3,"cols":[string4],"replace":bool0}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_SEARCH_ADV","params":{"offset":int1,"expression":string1,"sort":string2, "sortdesc":true,"plist":string3,"cols":[string4],"replace":bool0}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_SEARCH_ADV","params":{"offset":int1,"expression":string1,"sort":string2, "sortdesc":string3,"plist":string4,"cols":[string5],"replace":bool0}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_FILESYSTEM_LIST","params":{"offset":int1,"filter":string1,"path":string2,"cols":[string3,string4]}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_TAG_LIST","params":{"offset":int1,"filter":string1,"tag":string2}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_TAG_ALBUM_LIST","params":{"offset":int1,"filter":string1,"search":string3,"tag":string4}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_TAG_ALBUM_TITLE_LIST","params":{"album":string1,"search":string2,"tag":string3,"cols":[string4,string5]}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_GET_ALBUMS","params":{"offset":int1, "searchstr":string1, "tag":string2, "sort":string3, "sortdesc":bool0}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_SONGDETAILS","params":{"uri":string1}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_FINGERPRINT","params":{"uri":string1}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_VOLUME_SET","params":{"volume":int1}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_TOGGLE_OUTPUT","params":{"output":int1,"state":int2}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_LIKE","params":{"uri":string1,"like":int1}},
        {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_SETTINGS_SET","params":{"random": int1}},
        {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_SYSCMD","params":{"cmd": string1}},
        {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_COLS_SAVE","params":{"table":string1,"cols":[string2,string3,string4]}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_SMARTPLS_SAVE","params":{"type":string1,"playlist":string2,"timerange":int1,"sort":string3}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_SMARTPLS_GET","params":{"playlist":string1}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_SMARTPLS_UPDATE", "params":{"playlist":string1}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_RM_ALL", "params":{"type":string1}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_SORT", "params":{"uri":string1,"tag":string2}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_SHUFFLE", "params":{"uri":string1}},
        {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_TIMER_SAVE","params":{"timerid": 0, "name": string1, "enabled": bool0, "startHour": int1, "startMinute": int2, "action": string2, "volume": int3, "playlist": string3, "jukeboxMode": int4, "weekdays":[bool0,bool0,bool0,bool0,bool0,bool0,bool0]}},
        {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_TIMER_SAVE","params":{"timerid": int0, "name": string1, "enabled": bool0, "startHour": int1, "startMinute": int2, "action": string2, "volume": int3, "playlist": string3, "jukeboxMode": int4, "weekdays":[bool0,bool0,bool0,bool0,bool0,bool0,bool0]}},
        {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_TIMER_GET","params":{"timerid": int0}},
        {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_TIMER_TOGGLE","params":{"timerid": int0}},
        {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_TIMER_RM","params":{"timerid": int0}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_MOUNT_MOUNT","params":{"mountUrl":string1,"mountPoint":string2}},
        {"jsonrpc":"2.0","id":0,"method":"MPD_API_MOUNT_UNMOUNT","params":{"mountPoint":string1}},
    ];
}

function setTest(cmd, response) {
    var tr = document.createElement('tr');
    tr.innerHTML = '<td>' + (i + 1) + '</td><td>' + e(JSON.stringify(cmd)) + '</td><td>' + response + '</td>';
    var tbody = document.getElementsByTagName('tbody')[0];
    tbody.appendChild(tr);
    t++;
    document.getElementById('testCount').innerText = t + '/' + i + '/' + j;
    if (t > 10) {
        tbody.deleteRow(0);
    }
}

function getRandomUint(max) {
    return Math.floor(Math.random() * Math.floor(max));
}

function getRandomInt() {
    var int = Math.floor(Math.random() * Math.floor(9999999999));
    if (getRandomUint(3) === 0) {
        int = 0 - int;
    }
    return int;
}

function sendAPI(id) {
    if (id === 0) {
        cmds = defineCmds();
    }
    var request = cmds[id];
    
    var ajaxRequest=new XMLHttpRequest();
    ajaxRequest.open('POST', '/api', true);
    ajaxRequest.setRequestHeader('Content-type', 'application/json');
    ajaxRequest.onreadystatechange = function() {
        if (ajaxRequest.readyState === 4) {
            var sleep = 0;
            try {
                var obj = JSON.parse(ajaxRequest.responseText);
                if (obj.error && obj.error.message === 'MPD disconnected') {
                    sleep = 3000;
                    document.getElementsByTagName('h5')[0].innerText = 'Sleeping...';
                }
                setTest(request, ajaxRequest.responseText);
            }
            catch(error) {
                setTest(request, 'JSON parse error: ' + error);
                console.error('Request: ' + JSON.stringify(request));
                console.error('JSON parse error: ' + error);
                console.error('Response: ' + ajaxRequest.responseText);
            }
            i++;
            if (i < cmds.length) {
                setTimeout(function() { sendAPI(i); }, sleep);
            }
            else if (j < blns_len) {
                j++;
                i = 0;
                setTimeout(function() { sendAPI(i); }, sleep);
            }
            else {
                document.getElementsByTagName('h5')[0].innerText = 'Finished';
                return;
            }
        }
    };
    
    document.getElementsByTagName('h5')[0].innerText = 'Running ' + JSON.stringify(request);
    ajaxRequest.send(JSON.stringify(request));
}

function e(x) {
    if (isNaN(x)) {
        return x.replace(/([<>])/g, function(m0, m1) {
            if (m1 === '<') return '&lt;';
            else if (m1 === '>') return '&gt;';
        });
    }
    else {
        return x;
    }
}

sendAPI(0);
