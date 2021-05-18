"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

let tbody = document.getElementsByTagName('tbody')[0];
for (let i = 0; i < cmds.length; i++) {
    let tr = document.createElement('tr');
    tr.innerHTML = '<td>' + cmds[i].method + '</td>' +
        '<td>' + paramsToString(cmds[i].params) + '</td>' +
        '<td>' + (desc[cmds[i].method] !== undefined ? desc[cmds[i].method] : '') + '</td>';
    tbody.appendChild(tr);
}

function paramsToString(obj) {
    if (obj === undefined) {
        return 'Without parameters';
    }
    
    return JSON.stringify(obj).
        replace(/</g, '&lt;').
        replace(/>/g, '&gt;').
        replace(/:/g, ': ').
        replace(/,/g, ', ');
}
