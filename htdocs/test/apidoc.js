"use strict";
/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

function init() {
    const tbody = document.getElementsByTagName('tbody')[0];
    const methods = Object.keys(APImethods).sort();
    for (const method of methods) {
        let tr = document.createElement('tr');
        tr.setAttribute('id', method);
        tr.innerHTML = '<td>' + method + '<br/><small>' + APImethods[method].desc + '<br/>' +
            'Protected: ' + (APImethods[method].protected === true ? 'yes' : 'no') + '</small></td><td>' +
            paramsToString(APImethods[method].params) + '</td></tr>';
        tbody.appendChild(tr);
    }

    let options = '<option></option>';
    for (const method of methods) {
        options += '<option value="' + method + '">' + method + '</option>';
    }

    let select = document.getElementById('selectMethod');
    select.innerHTML = options;
    select.addEventListener('change', function() {
        const m = this.options[this.selectedIndex].value;
        if (m !== '') {
            for (const row of tbody.rows) {
                if (row.getAttribute('id') === m) {
                    row.classList.remove('d-none');
                }
                else {
                    row.classList.add('d-none');
                }
            }
        }
    }, false);
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
