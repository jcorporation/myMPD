"use strict";
/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

const cmds = Object.keys(APImethods);
let i = 0;
let error = 0;
let warn = 0;
let ok = 0;
let time_start = 0;
let time_end = 0;
let time_all = 0;

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
    const duration = time_end - time_start;
    time_all += duration;
    document.getElementById('testCount').textContent = 'Test ' + (i + 1) + '/' + cmds.length + ' - ' +
        ok + ' ok, ' + warn + ' warnings, ' + error + ' errors, duration: ' + time_all + ' ms';
    const tr = document.createElement('tr');
    tr.innerHTML = '<td>' + (i + 1) + '</td><td>' + JSON.stringify(cmd) + '</td><td>' + duration + ' ms</td><td>' + response + '</td>';
    tr.childNodes[2].style.backgroundColor = (state === 'ok' ? 'green' : (state === 'warn' ? 'yellow' : 'red'));
    document.getElementsByTagName('tbody')[0].appendChild(tr);
}

function sendAPI(method) {
    let ajaxRequest = new XMLHttpRequest();
    ajaxRequest.open('POST', '/api/default', true);
    ajaxRequest.setRequestHeader('Content-type', 'application/json');
    ajaxRequest.onreadystatechange = function() {
        if (ajaxRequest.readyState === 4) {
            if (ajaxRequest.responseText !== '') {
                let obj;
                try {
                    obj = JSON.parse(ajaxRequest.responseText);
                    time_end = new Date().getTime();
                    if (!obj.error && !obj.result && !obj.jsonrpc) {
                        setTest(request, 'error', 'Invalid response: ' + ajaxRequest.responseText);
                    }
                    else if (obj.result) {
                        setTest(request, 'ok', ajaxRequest.responseText);
                    }
                    else if (obj.error &&
                        (obj.error.message === 'Invalid API request' ||
                         obj.error.message === 'No response for method %{method}')
                    ) {
                        setTest(request, 'error', ajaxRequest.responseText);
                    }
                    else {
                        setTest(request, 'warn', ajaxRequest.responseText);
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
                document.getElementsByTagName('h5')[0].textContent = 'Finished';
            }
        }
    };
    let request = {"jsonrpc": "2.0", "id": 0, "method": method, "params": apiParamsToObject(APImethods[method].params)};
    document.getElementsByTagName('h5')[0].textContent = 'Running ' + JSON.stringify(request);
    time_start = new Date().getTime();
    ajaxRequest.send(JSON.stringify(request));
}

function apiParamsToObject(p) {
    let args = {};
    for (const param in p) {
        if (p[param].params !== undefined) {
            args[param] = apiParamsToObject(p[param].params);
        }
        else if (p[param].type === 'array' || p[param].type === 'object') {
            args[param] = JSON.parse(p[param].example);
        }
        else {
            args[param] = p[param].example;
        }
    }
    return args;
}

sendAPI(cmds[i]);
