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
var track = 1;
var outputId = 0;
var timerId = 0;
var uri = '';
var album = '';
var artist = '';
var searchstr = 'tabula';
var time_start = 0;
var time_end = 0;
var time_all = 0;

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
        if (request.params.uri !== undefined) { request.params.uri = uri; }
        if (request.params.tag !== undefined) { request.params.tag = 'Artist'; }
        if (request.params.album !== undefined) { request.params.album = album; }
        if (request.params.filter !== undefined) { request.params.filter = 'any'; }
        if (request.params.searchstr !== undefined) { request.params.searchstr = 'tabula'; }
        if (request.params.trackId !== undefined) { request.params.trackId = trackId; }
        if (request.params.outputId !== undefined) { request.params.outputId = outputId; }
        if (request.params.plist !== undefined) { request.params.plist = 'test'; }
        if (request.params.track !== undefined) { request.params.track = track; }
        if (request.params.from !== undefined) { request.params.from = 1; request.params.to = 2; }
        if (request.params.start !== undefined) { request.params.start = 1; request.params.end = 2; }
        if (request.method === 'MYMPD_API_DATABASE_SEARCH_ADV') {
            request.params.plist = '';
            request.params.expression = '((any contains \'' + searchstr + '\'))';
        }
        else if (request.method === 'MYMPD_API_PLAYLIST_RENAME') {
            if (request.params.from !== undefined) { request.params.from = 'test'; request.params.to = 'test2'; }
        }
        else if (request.method === 'MYMPD_API_QUEUE_ADD_RANDOM') {
            request.params.plist = 'Database';
        }
    }

    var ajaxRequest = new XMLHttpRequest();
    ajaxRequest.open('POST', '/api/', true);
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
                            if (obj.result.method === 'MYMPD_API_DATABASE_SEARCH_ADV' && obj.result.data && obj.result.data.length > 1) {
                                uri = obj.result.data[0].uri;
                                artist = obj.result.data[0].Artist;
                                album = obj.result.data[0].Album;
                            }
                            else if (obj.result.method === 'MYMPD_API_QUEUE_LIST' && obj.result.data.length > 0) {
                                trackId = obj.result.data[0].id;
                            }
                            else if (obj.result.method === 'MYMPD_API_PLAYER_OUTPUT_LIST' && obj.result.data.length > 0) {
                                outputId = obj.result.data[0].id;
                            }
                            else if (obj.result.method === 'MYMPD_API_TIMER_LIST' && obj.result.data.length > 0) {
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
