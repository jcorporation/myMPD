"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/* global cmds */

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

function init() {
    let options = '<option></option>';
    const methods = Object.keys(cmds).sort();
    for (const method of methods) {
        options += '<option value="' + method + '">' + method + '</option>';
    }
    let select = document.getElementById('cmds');
    select.innerHTML = options;
    select.addEventListener('change', function() {
        let method = this.options[this.selectedIndex].value;
        let form = '';
        if (method !== '' && cmds[method].params !== undefined) {
            form = paramsToForm(cmds[method].params, '');
            document.getElementById('desc').innerText = cmds[method].desc;
        }
        document.getElementById('params').innerHTML = form;
        document.getElementById('desc').innerText = '';
        document.getElementById('resultText').innerText = '';
        document.getElementById('requestText').innerText = '';
        document.getElementById('resultState').innerText = 'Result';
    }, false);
    document.getElementById('btnSubmit').addEventListener('click', function(event) {
        event.preventDefault();
        sendAPI();
    }, false);
}

function paramsToForm(p, k) {
    let form = '';
    for (const param in p) {
        if (p[param].params !== undefined) {
            form += '<div class="form-group row">' +
                '<label class="col-sm-4 col-form-label">' + param + '</label>' +
                '<div class="col-sm-8">' + paramsToForm(p[param].params, param) +
                '</div>' + 
                '</div>';
        }
        else {
            form += '<div class="form-group row">' +
                '<label class="col-sm-4 col-form-label" for="input-' + param + '">' + param + ' <small>(' + p[param].type + ')</small></label>' +
                '<div class="col-sm-8"><input id="input-' + k + param + '" class="form-control" value="' + e(p[param].example) + '"/>' +
                '<small>' + e(p[param].desc) + '</small></div>' + 
                '</div>';
        }
    }
    return form;
}

function formToParams(p, k) {
    let request = {};
    for (const param in p) {
        if (p[param].params !== undefined) {
            request[param] = formToParams(p[param].params, param);
        }
        else {
            let value = document.getElementById('input-' + k + param).value;
            if (value.charAt(0) === '{' || value.charAt(0) === '[') {
                request[params] = JSON.parse(value);
            }
            else {
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
                request[param] = value;
            }
        }
    }
    return request;
}

function sendAPI() {
    let select = document.getElementById('cmds');
    let method = select.options[select.selectedIndex].value;
    let request = {"jsonrpc": "2.0", "id": 0, "method": method, "params": {}};
    if (cmds[method].params !== undefined) {
        request.params = formToParams(cmds[method].params, '');
    }
    let ajaxRequest = new XMLHttpRequest();
    ajaxRequest.open('POST', '/api/', true);
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
    document.getElementById('requestText').innerText = JSON.stringify(request);
}

init();
