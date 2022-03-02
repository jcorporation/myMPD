"use strict";
/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
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
    const methods = Object.keys(APImethods).sort();
    for (const method of methods) {
        options += '<option value="' + method + '">' + method + '</option>';
    }
    let select = document.getElementById('cmds');
    select.innerHTML = options;
    select.addEventListener('change', function() {
        let method = this.options[this.selectedIndex].value;
        let form = '';
        if (method !== '' && APImethods[method].params !== undefined) {
            form = paramsToForm(APImethods[method].params, '');
            document.getElementById('desc').textContent = APImethods[method].desc;
            if (APImethods[method].protected === true) {
                document.getElementById('protected').classList.remove('d-none');
            }
            else {
                document.getElementById('protected').classList.add('d-none');
            }
        }
        else {
            document.getElementById('desc').textContent = '';
        }
        document.getElementById('params').innerHTML = form;
        document.getElementById('resultText').textContent = '';
        document.getElementById('requestText').textContent = '';
        document.getElementById('resultState').textContent = 'Result';
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
                request[param] = JSON.parse(value);
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
                    value = Number(value);
                }
                request[param] = value;
            }
        }
    }
    return request;
}

function sendAPI() {
    document.getElementById('resultState').textContent = 'Sending...';
    document.getElementById('resultText').textContent = '';
    let select = document.getElementById('cmds');
    let method = select.options[select.selectedIndex].value;
    let request = {"jsonrpc": "2.0", "id": 0, "method": method, "params": {}};
    if (APImethods[method].params !== undefined) {
        request.params = formToParams(APImethods[method].params, '');
    }
    let time_start = 0;
    let time_end = 0;
    let ajaxRequest = new XMLHttpRequest();
    ajaxRequest.open('POST', '/api/', true);
    ajaxRequest.setRequestHeader('Content-type', 'application/json');
    ajaxRequest.setRequestHeader('Authorization', 'Bearer ' + document.getElementById('session').value);
    ajaxRequest.onreadystatechange = function() {
        if (ajaxRequest.readyState === 4 && ajaxRequest.status == 200) {
            try {
                let obj = JSON.parse(ajaxRequest.responseText);
                if (obj.result) {
                    time_end = new Date().getTime();
                    const duration = time_end - time_start;
                    document.getElementById('resultState').textContent = 'OK - ' + duration + ' ms';
                }
                else {
                    document.getElementById('resultState').textContent = 'ERROR';
                }
            }
            catch(e) {
                document.getElementById('resultState').textContent = 'JSON parse error: ' + e;
            }
            document.getElementById('resultText').textContent = ajaxRequest.responseText;
        }
        else {
            document.getElementById('resultState').textContent = 'Response code: ' + ajaxRequest.status;
            document.getElementById('resultText').textContent = ajaxRequest.responseText;
        }
    };
    time_start = new Date().getTime();
    ajaxRequest.send(JSON.stringify(request));
    document.getElementById('requestText').textContent = JSON.stringify(request);
}

init();
