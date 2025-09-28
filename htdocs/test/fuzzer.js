"use strict";
/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

let i = 0;
let j = 0;
let t = 0;
const blns_len = blns.length;
let cmds;

const subdir = window.location.pathname.replace('/test/fuzzer.html', '').replace(/\/$/, '');

function defineCmds() {
    let newCmds = [];
    for (const method in APImethods) {
        if (method === 'MYMPD_API_CONNECTION_SAVE') {
            continue;
        }
        let request = {"jsonrpc": "2.0", "id": 0, "method": method, "params": apiParamsToObject(APImethods[method].params)};
        newCmds.push(request);
    }
    return newCmds;
}

function getRandomByType(t) {
    if (getRandomBool() === true) {
        //return value of valid type
        if (t === APItypes.bool) { return getRandomBool(); }
        if (t === APItypes.int) { return getRandomInt(); }
        if (t === APItypes.uint) { return getRandomUint(); }
        return blns[getRandomUint(blns_len)];
    }
    else {
        //return value of invalid type
        const rt = getRandomUint(4);
        if (rt === 0) { return getRandomBool(); }
        if (rt === 1) { return getRandomInt(); }
        if (rt === 2) { return getRandomUint(); }
        if (rt === 2) { return blns[getRandomUint(blns_len)]; }
        return '';
    }
}

function apiParamsToObject(p) {
    let args = {};
    for (const param in p) {
        if (p[param].params !== undefined) {
            args[param] = apiParamsToObject(p[param].params);
        }
        else {
            if (getRandomBool() === true) {
                //return valid example value
                args[param] = p[param].example;
            }
            else {
                //return random value with random type
                args[param] = getRandomByType(p[param].type);
            }
        }
    }
    return args;
}

function setTest(cmd, response) {
    var tr = document.createElement('tr');
    tr.innerHTML = '<td>' + (i + 1) + '</td><td>' + e(JSON.stringify(cmd)) + '</td><td>' + e(response) + '</td>';
    const tbody = document.getElementsByTagName('tbody')[0];
    tbody.appendChild(tr);
    t++;
    document.getElementById('testCount').textContent = t + '/' + i + '/' + j;
    if (t > 10) {
        tbody.deleteRow(0);
    }
}

function getRandomUint(max) {
    if (max === null) {
        max = 9999999999;
    }
    return Math.floor(Math.random() * Math.floor(max));
}

function getRandomInt() {
    let int = Math.floor(Math.random() * Math.floor(9999999999));
    if (getRandomUint(3) === 0) {
        int = 0 - int;
    }
    return int;
}

function getRandomBool() {
    return Math.random() >= 0.5;
}

async function sendAPI(id) {
    if (id === 0) {
        cmds = defineCmds();
    }
    const request = cmds[id];
    document.getElementsByTagName('h5')[0].textContent = request.method;
    let sleep = 0;
    const uri = subdir + '/api/default';
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

    if (response.ok === false) {
        const text = await response.text();
        setTest(request, 'error', text);
    }
    else {
        try {
            const obj = await response.json();
            if (obj.error &&
                obj.error.message === 'MPD disconnected')
            {
                sleep = 1000;
                document.getElementsByTagName('h5')[0].textContent = 'Sleeping...';
            }
            setTest(request, JSON.stringify(obj));
        }
        catch(error) {
            setTest(request, 'JSON parse error: ' + error);
            console.error('Request: ' + JSON.stringify(request));
            console.error(error);
            return;
        }
    }

    //next
    i++;
    if (i < cmds.length) {
        if (getRandomBool() === true) {
            //delete random params
            for (const key in cmds[i].params) {
                if (getRandomBool() === true) {
                    delete cmds[i].params[key];
                }
            }
        }
        setTimeout(function() { sendAPI(i); }, sleep);
    }
    else if (j < blns_len) {
        j++;
        i = 0;
        setTimeout(function() { sendAPI(i); }, sleep);
    }
    else {
        document.getElementsByTagName('h5')[0].textContent = 'Finished';
        return;
    }
}

function e(x) {
    if (isNaN(x)) {
        return x.replace(/([<>"'])/g, function(m0, m1) {
            if (m1 === '<') return '&lt;';
            else if (m1 === '>') return '&gt;';
            else if (m1 === '"') return '&quot;';
            else if (m1 === '\'') return '&apos;';
        }).replace(/\\u(003C|003E|0022|0027)/gi, function(m0, m1) {
            if (m1 === '003C') return '&lt;';
            else if (m1 === '003E') return '&gt;';
            else if (m1 === '0022') return '&quot;';
            else if (m1 === '0027') return '&apos;';
        });
    }
    return x;
}

sendAPI(0);
