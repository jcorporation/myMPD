"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/* global cmds */

function init() {
    let tbody = document.getElementsByTagName('tbody')[0];
    const methods = Object.keys(cmds).sort();
    for (const method of methods) {
        let tr = document.createElement('tr');
        tr.innerHTML = '<td>' + method + '<br/><small>' + cmds[method].desc + '</small></td><td>' +
            paramsToString(cmds[method].params) + '</td></tr>';
        tbody.appendChild(tr);
    }
}

function paramsToString(p) {
    let html = '<table class="table table-sm ">';
    for (const param in p) {
        if (p[param].params !== undefined) {
            html += '<tr class="table-secondary"><td colspan="2">' + param + '</td></tr>' +
                '<tr><td></td><td>' + paramsToString(p[param].params) +
                '</td></tr>';
        }
        else {
            html += '<tr class="table-secondary"><td colspan="2">' + param + '</td></tr>' +
                '<tr><th>Type</th><td>' + p[param].type + '</td></tr>' +
                '<tr><th>Desc</th><td>' + p[param].desc + '</td></tr>' +
                '<tr><th>Example</th><td>' + p[param].example + '</td></tr>';
        }
    }
    html += '</table';
    return html;
}

init();
