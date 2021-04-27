"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

var i = 0;
var j = 0;
var t = 0;
var blns_len = blns.length;
var cmds;

function defineCmds() {
    var newCmds = cmds.slice();
    for (const c in newCmds) {
        for (const p in newCmds[c].params) {
            switch(typeof p) {
                case "number": 
                    newCmds[c].params[p] = getRandomInt(); 
                    break;
                case "boolean":
                    newCmds[c].params[p] = getRandomBool();
                    break;
                default:
                    newCmds[c].params[p] = blns[getRandomUint(blns_len)];
            }
        }
    }
    return newCmds;
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

function getRandomBool() {
    return Math.random() >= 0.5;
}

function sendAPI(id) {
    if (id === 0) {
        cmds = defineCmds();
    }
    var request = cmds[id];
    
    var ajaxRequest=new XMLHttpRequest();
    ajaxRequest.open('POST', '/api/', true);
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
