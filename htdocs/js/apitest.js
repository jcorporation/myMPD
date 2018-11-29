var i = 0;
var failed = 0;
var ok = 0;
var trackId = 0;
var outputId = 0;

var cmds = [
    '{"cmd":"MPD_API_WELCOME"}',
    '{"cmd":"MPD_API_QUEUE_CLEAR"}',
    '{"cmd":"MPD_API_QUEUE_ADD_TRACK","data":{"uri":"Various-Captain_Future"}}',
    '{"cmd":"MPD_API_QUEUE_LIST","data":{"offset":0}}',
    '{"cmd":"MPD_API_QUEUE_MOVE_TRACK","data":{"from":0,"to":1}}',
    '{"cmd":"MPD_API_QUEUE_RM_RANGE","data":{"start":1,"end":-1}}',
    '{"cmd":"MPD_API_PLAYER_PLAY_TRACK","data":{"track":__TRACKID__}}',
    '{"cmd":"MPD_API_PLAYER_SEEK","data":{"songid":__TRACKID__,"seek":10}}',
    '{"cmd":"MPD_API_QUEUE_RM_TRACK","data":{"track":__TRACKID__}}',
    '{"cmd":"MPD_API_QUEUE_SAVE","data":{"plist":"test"}}',
    '{"cmd":"MPD_API_QUEUE_SEARCH","data":{"offset":0,"filter":"any","searchstr":"ch"}}',
    '{"cmd":"MPD_API_QUEUE_ADD_PLAY_TRACK","data":{"uri":"Various-Captain_Future/01.Christian_Bruhn-Captain_Future.mp3"}}',
    '{"cmd":"MPD_API_QUEUE_SHUFFLE"}',
    '{"cmd":"MPD_API_QUEUE_REPLACE_TRACK","data":{"uri":"Various-Captain_Future/01.Christian_Bruhn-Captain_Future.mp3"}}',
    '{"cmd":"MPD_API_QUEUE_ADD_TRACK_AFTER","data":{"uri":"Various-Captain_Future/01.Christian_Bruhn-Captain_Future.mp3","to":1}',
    '{"cmd":"MPD_API_PLAYLIST_RENAME","data":{"from":"test","to":"test2"}}',
    '{"cmd":"MPD_API_PLAYLIST_CONTENT_LIST","data":{"uri":"test2","offset":0,"filter":""}}',
    '{"cmd":"MPD_API_QUEUE_ADD_PLAYLIST","data":{"plist":"test2"}}',
    '{"cmd":"MPD_API_QUEUE_REPLACE_PLAYLIST","data":{"plist":"test2"}}',
    '{"cmd":"MPD_API_PLAYLIST_LIST","data":{"offset":0,"filter":""}}',
    '{"cmd":"MPD_API_PLAYLIST_CLEAR","data":{"uri":"test2"}}',
    '{"cmd":"MPD_API_QUEUE_CROP"}',
    '{"cmd":"MPD_API_QUEUE_LAST_PLAYED","data":{"offset":0}}',
    '{"cmd":"MPD_API_PLAYLIST_ADD_TRACK","data":{"plist":"test2","uri":"Various-Captain_Future/01.Christian_Bruhn-Captain_Future.mp3"}}',
    '{"cmd":"MPD_API_PLAYLIST_ADD_TRACK","data":{"plist":"test2","uri":"Various-Captain_Future/01.Christian_Bruhn-Captain_Future.mp3"}}',
    '{"cmd":"MPD_API_PLAYLIST_MOVE_TRACK","data":{"plist":"test2","from":1,"to":2}}',
    '{"cmd":"MPD_API_PLAYLIST_RM_TRACK","data":{"uri":"test2","track":1}',
    '{"cmd":"MPD_API_PLAYLIST_RM","data":{"uri":"test2"}}',
    '{"cmd":"MPD_API_SMARTPLS_UPDATE_ALL"}',
    '{"cmd":"MPD_API_SMARTPLS_SAVE"}',
    '{"cmd":"MPD_API_SMARTPLS_GET"}',
    '{"cmd":"MPD_API_DATABASE_SEARCH_ADV","data":{"offset":0,"expression":"(any contains \'ch\')","sort":"Title", "sortdesc":false,"plist":""}}',
    '{"cmd":"MPD_API_DATABASE_SEARCH","data":{"offset":0,"filter":"any","searchstr":"ch","plist":""}}',
    '{"cmd":"MPD_API_DATABASE_UPDATE"}',
    '{"cmd":"MPD_API_DATABASE_RESCAN"}',
    '{"cmd":"MPD_API_DATABASE_FILESYSTEM_LIST","data":{"offset":0,"filter":"","path":""}}',
    '{"cmd":"MPD_API_DATABASE_TAG_LIST","data":{"offset":0,"filter":"","tag":"Artist"}}',
    '{"cmd":"MPD_API_DATABASE_TAG_ALBUM_LIST","data":{"offset":0,"filter":"","search":"Christian Bruhn","tag":"Artist"}}',
    '{"cmd":"MPD_API_DATABASE_TAG_ALBUM_TITLE_LIST","data":{"album":"Captain Future","search":"Christian Bruhn","tag":"Artist"}}',
    '{"cmd":"MPD_API_DATABASE_STATS"}',
    '{"cmd":"MPD_API_DATABASE_SONGDETAILS","data":{"uri":"Various-Captain_Future/01.Christian_Bruhn-Captain_Future.mp3"}}',
    '{"cmd":"MPD_API_PLAYER_VOLUME_SET","data":{"volume":10}}',
    '{"cmd":"MPD_API_PLAYER_VOLUME_GET"}',
    '{"cmd":"MPD_API_PLAYER_PAUSE"}',
    '{"cmd":"MPD_API_PLAYER_STOP"}',    
    '{"cmd":"MPD_API_PLAYER_PLAY"}',
    '{"cmd":"MPD_API_PLAYER_NEXT"}',
    '{"cmd":"MPD_API_PLAYER_PREV"}',
    '{"cmd":"MPD_API_PLAYER_STOP"}',
    '{"cmd":"MPD_API_PLAYER_OUTPUT_LIST"}',
    '{"cmd":"MPD_API_PLAYER_TOGGLE_OUTPUT","data":{"output":"__OUTPUTID__","state":0}}',
    '{"cmd":"MPD_API_PLAYER_CURRENT_SONG"}',
    '{"cmd":"MPD_API_PLAYER_STATE"}',
    '{"cmd":"MPD_API_SETTINGS_GET"}',
    '{"cmd":"MPD_API_SETTINGS_SET"}',
    '{"cmd":"MPD_API_LIKE","data":{"uri":"Various-Captain_Future/01.Christian_Bruhn-Captain_Future.mp3","like":2}}',
    '{"cmd":"MPD_API_SYSCMD"}',
    '{"cmd":"MPD_API_COLS_SAVE"}'
];

function setTest(cmd, state, response) {
    if (state == "ok")
        ok++;
    else
        failed++;
    document.getElementById('testCount').innerText = 'Test ' + (i + 1) + '/' + cmds.length + ' - ' +
        ok + ' ok, ' + failed + ' failed';
    var tr = document.createElement('tr');
    tr.innerHTML = '<td>' + i + '</td><td>' + cmd + '</td><td class="td-' + state + '">&nbsp;</td><td>' + response + '</td>';
    document.getElementsByTagName('tbody')[0].appendChild(tr);
}

function sendAPI(request, callback) {
    var ajaxRequest=new XMLHttpRequest();
    ajaxRequest.open('POST', '/api', true);
    ajaxRequest.setRequestHeader('Content-type', 'application/json');
    ajaxRequest.onreadystatechange = function() {
        if (ajaxRequest.readyState == 4) {
            if (ajaxRequest.responseText != '') {
                var obj = JSON.parse(ajaxRequest.responseText);
                if (obj.type == 'queue' && obj.data.length > 0)
                    trackId = obj.data[0].id;
                else if (obj.type == 'outputs' && obj.data.length > 0)
                    ouputId = obj.data[0].id;
                if (obj.type == 'error')
                    setTest(request, 'error', ajaxRequest.responseText);
                else 
                    setTest(request, 'ok', ajaxRequest.responseText);
            }
            else {
                setTest(request, 'error', ajaxRequest.responseText);
            }
            i++;
            if (i < cmds.length)
                sendAPI(cmds[i]);
        }
    };
    request = request.replace(/__TRACKID__/,trackId);
    request = request.replace(/__OUTPUTID__/,outputId);
    ajaxRequest.send(request);
}

sendAPI(cmds[i]);
