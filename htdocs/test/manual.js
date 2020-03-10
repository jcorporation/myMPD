"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

var cmds = [
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_CLEAR"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_CROP"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_SAVE","params":{"plist":""}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_LIST","params":{"offset":0,"cols":["",""]}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_SEARCH","params":{"offset":0,"filter":"","searchstr":"","cols":["",""]}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_RM_TRACK","params":{"track":0}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_RM_RANGE","params":{"start":0,"end":0}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_MOVE_TRACK","params":{"from":0,"to":0}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_TRACK_AFTER","params":{"uri":"","to":0}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_TRACK","params":{"uri":""}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_PLAY_TRACK","params":{"uri":""}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_REPLACE_TRACK","params":{"uri":""}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_PLAYLIST","params":{"plist":""}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_ADD_RANDOM","params":{"playlist":"Database","quantity":0, "mode":0}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_REPLACE_PLAYLIST","params":{"plist":""}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_SHUFFLE"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_QUEUE_LAST_PLAYED","params":{"offset":0,"cols":["","",""]}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_RM","params":{"uri":""}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_CLEAR","params":{"uri":""}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_RENAME","params":{"from":"","to":""}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_MOVE_TRACK","params":{"plist":"","from":0,"to":0}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_ADD_TRACK","params":{"plist":"","uri":""}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_RM_TRACK","params":{"uri":"","track":0}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_RM_ALL", "params":{"type":""}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_LIST","params":{"offset":0,"filter":""}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_CONTENT_LIST","params":{"uri":"","offset":0,"filter":"","cols":["",""]}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_SHUFFLE", "params":{"uri":""}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYLIST_SORT", "params":{"uri":"","tag":""}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_SMARTPLS_UPDATE_ALL"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_SMARTPLS_UPDATE", "params":{"playlist":""}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_SMARTPLS_SAVE","params":{"type":"","playlist":"","timerange":0,"sort":""}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_SMARTPLS_GET","params":{"playlist":""}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_SEARCH_ADV","params":{"offset":0,"expression":"(any contains '"+""+"')","sort":"", "sortdesc":false,"plist":"","cols":[""],"replace":false}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_SEARCH","params":{"offset":0,"filter":"","searchstr":"","plist":"","cols":["",""],"replace":false}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_UPDATE","params":{"uri":""}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_RESCAN","params":{"uri":""}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_FILESYSTEM_LIST","params":{"offset":0,"filter":"","path":"","cols":["",""]}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_TAG_LIST","params":{"offset":0,"filter":"","tag":""}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_TAG_ALBUM_LIST","params":{"offset":0,"filter":"","search":"","tag":""}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_TAG_ALBUM_TITLE_LIST","params":{"album":"","search":"","tag":"","cols":["",""]}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_STATS"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_SONGDETAILS","params":{"uri":""}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_FINGERPRINT","params":{"uri":""}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_DATABASE_GET_ALBUMS","params":{"offset":0, "searchstr":"", "tag":"", "sort":"", "sortdesc":false}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_PLAY_TRACK","params":{"track":0}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_VOLUME_SET","params":{"volume":0}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_VOLUME_GET"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_PAUSE"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_PLAY"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_STOP"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_SEEK_CURRENT","params":{"seek":0,"realtive":false}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_SEEK","params":{"songid":0,"seek":0}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_NEXT"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_PREV"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_OUTPUT_LIST"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_TOGGLE_OUTPUT","params":{"output":0,"state":0}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_CURRENT_SONG"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_PLAYER_STATE"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_LIKE","params":{"uri":"","like":0}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_SETTINGS_GET"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_MOUNT_LIST"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_MOUNT_NEIGHBOR_LIST"},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_MOUNT_MOUNT","params":{"mountUrl":"", "mountPoint":""}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_MOUNT_UNMOUNT","params":{"mountPoint":""}},
    {"jsonrpc":"2.0","id":0,"method":"MPD_API_URLHANDLERS"},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_SETTINGS_GET"},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_SETTINGS_SET","params":{"random":0}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_SETTINGS_RESET"},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_COLS_SAVE","params":{"table":"","cols":["","",""]}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_SYSCMD","params":{"cmd": ""}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_BOOKMARK_RM","params":{"id":0}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_BOOKMARK_LIST","params":{"offset":0}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_BOOKMARK_SAVE","params":{"id": 0, "name":"", "uri":"", "type":""}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_BOOKMARK_CLEAR"},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_TIMER_SAVE","params":{"timerid": 0, "name": "", "enabled": false, "startHour": 0, "startMinute": 0, "action": "", "volume": 0, "playlist": "", "jukeboxMode": 0, "weekdays":[false,false,false,false,false,false,false]}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_TIMER_LIST","params":{}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_TIMER_GET","params":{"timerid":0}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_TIMER_RM","params":{"timerid":0}},
    {"jsonrpc":"2.0","id":0,"method":"MYMPD_API_TIMER_TOGGLE","params":{"timerid":0}}
];

function init() {
    let options = '<option></option>';
    for (let i = 0; i < cmds.length; i++) {
        options += '<option value="' + i + '">' + cmds[i].method + '</option>';
    }
    let select = document.getElementById('cmds');
    select.innerHTML = options;
    select.addEventListener('change', function() {
        let id = this.options[this.selectedIndex].value;
        let form = '';
        if (cmds[id].params !== undefined) {
            let params = Object.keys(cmds[id].params);
            for (let key of params) {
                form += '<div class="form-group row">' +
                    '<label class="col-sm-4 col-form-label" for="input-' + key + '">' + key + '</label>' +
                    '<div class="col-sm-8"><input id="input-' + key + '" class="form-control"/></div>' +
                    '</div>';
            }
        }
        document.getElementById('params').innerHTML = form;
        document.getElementById('resultText').innerText = '';
        document.getElementById('resultState').innerText = 'Result';
    }, false);
    document.getElementById('btnSubmit').addEventListener('click', function(event) {
        event.preventDefault();
        sendAPI();
    }, false);
}

function sendAPI() {
    let select = document.getElementById('cmds');
    let id = select.options[select.selectedIndex].value;
    let request = cmds[id];
    if (cmds[id].params !== undefined) {
        let params = Object.keys(cmds[id].params);
        for (let key of params) {
            let value = document.getElementById('input-' + key).value;
            if (value === '') {
                //do nothing
            }
            else if (value === 'true') {
                value = true;
            }
            else if (value === 'false') {
                value = false;
            }
            else if (!isNaN(value)) {
                value = parseFloat(value);
            }
            request.params[key] = value;
        }
    }
    let ajaxRequest=new XMLHttpRequest();
    ajaxRequest.open('POST', '/api', true);
    ajaxRequest.setRequestHeader('Content-type', 'application/json');
    ajaxRequest.onreadystatechange = function() {
        if (ajaxRequest.readyState === 4) {
            try {
                let obj = JSON.parse(ajaxRequest.responseText);
                if (obj.result) {
                    document.getElementById('resultState').innerText = 'OK';
                }
                else {
                    document.getElementById('resultState').innerText = 'ERROR';
                }
            }
            catch(e) {
                document.getElementById('resultState').innerText = 'JSON parse error: ' + e;
            }
            document.getElementById('resultText').innerText = ajaxRequest.responseText;
        }
    };
    ajaxRequest.send(JSON.stringify(request));
}

init();
