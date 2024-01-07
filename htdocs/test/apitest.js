"use strict";
/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
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

//escapes html characters to avoid xss
function e(x) {
    if (x!== undefined && isNaN(x)) {
        return x.replace(/([<>"'])/g, function(m0, m1) {
            if (m1 === '<') return '&lt;';
            else if (m1 === '>') return '&gt;';
            else if (m1 === '"') return '&quot;';
            else if (m1 === '\'') return '&apos;';
        });
    }
    return x;
}

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
    tr.innerHTML = '<td>' + (i + 1) + '</td><td>' + e(JSON.stringify(cmd)) + '</td><td>' + duration + ' ms</td><td><div class="response">' + e(response) + '</div></td>';
    tr.childNodes[2].style.backgroundColor = (state === 'ok' ? 'green' : (state === 'warn' ? 'yellow' : 'red'));
    document.getElementsByTagName('tbody')[0].appendChild(tr);
}

async function sendAPI(method) {
    const request = {"jsonrpc": "2.0", "id": 0, "method": method, "params": apiParamsToObject(APImethods[method].params)};
    document.getElementsByTagName('h5')[0].textContent = 'Running ' + JSON.stringify(request);
    time_start = new Date().getTime();
    const uri = '/api/default';
    const response = await fetch(uri, {
        method: 'POST',
        mode: 'same-origin',
        credentials: 'same-origin',
        cache: 'no-store',
        redirect: 'follow',
        headers: {
            'Content-Type': 'application/json',
            'X-myMPD-Session': ''
        },
        body: JSON.stringify(request)
    });
    time_end = new Date().getTime();

    if (response.ok === false) {
        setTest(request, 'error', ajaxRequest.responseText);
    }
    else {
        try {
            const obj = await response.json();
            if (!obj.error && !obj.result && !obj.jsonrpc) {
                setTest(request, 'error', 'Invalid response: ' + JSON.stringify(obj));
            }
            else if (obj.result) {
                setTest(request, 'ok', JSON.stringify(obj));
            }
            else if (obj.error &&
                (obj.error.message === 'Invalid API request' ||
                 obj.error.message === 'No response for method %{method}')
            ) {
                setTest(request, 'error', JSON.stringify(obj));
            }
            else {
                setTest(request, 'warn', JSON.stringify(obj));
            }
        }
        catch(error) {
            const text = await response.text();
            setTest(request, 'error', '<p>JSON parse error: ' + error + '</p><small>' + text + '</small>');
        }
    }

    //call next test
    i++;
    if (i < cmds.length) {
        sendAPI(cmds[i]);
    }
    else {
        document.getElementsByTagName('h5')[0].textContent = 'Finished';
    }
}

function apiParamsToObject(p) {
    let args = {};
    for (const param in p) {
        if (p[param].params !== undefined) {
            args[param] = apiParamsToObject(p[param].params);
        }
        else if (p[param].type === APItypes.array ||
                 p[param].type === APItypes.object)
        {
            args[param] = JSON.parse(p[param].example);
        }
        else {
            args[param] = p[param].example;
        }
    }
    return args;
}

sendAPI(cmds[i]);
