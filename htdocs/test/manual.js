"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

function init() {
    let options = '<option></option>';
    for (let i = 0; i < cmds.length; i++) {
        options += '<option value="' + i + '">' + cmds[i].method + '</option>';
    }
    let select = document.getElementById('cmds');
    select.innerHTML = options;
    select.addEventListener('change', function() {
        let id = this.options[this.selectedIndex].value;
        let form = '';
        if (cmds[id].params !== undefined) {
            let params = Object.keys(cmds[id].params);
            for (let key of params) {
                form += '<div class="form-group row">' +
                    '<label class="col-sm-4 col-form-label" for="input-' + key + '">' + key + '</label>' +
                    '<div class="col-sm-8"><input id="input-' + key + '" class="form-control"/></div>' +
                    '</div>';
            }
        }
        document.getElementById('params').innerHTML = form;
        document.getElementById('resultText').innerText = '';
        document.getElementById('requestText').innerText = '';
        document.getElementById('resultState').innerText = 'Result';
    }, false);
    document.getElementById('btnSubmit').addEventListener('click', function(event) {
        event.preventDefault();
        sendAPI();
    }, false);
}

function sendAPI() {
    let select = document.getElementById('cmds');
    let id = select.options[select.selectedIndex].value;
    let request = cmds[id];
    if (cmds[id].params !== undefined) {
        let params = Object.keys(cmds[id].params);
        for (let key of params) {
            let value = document.getElementById('input-' + key).value;
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
            if (value.charAt(0) === '{' || value.charAt(0) === '[') {
                request.params[key] = JSON.parse(value);
            }
            else {
                request.params[key] = value;
            }
        }
    }
    let ajaxRequest=new XMLHttpRequest();
    ajaxRequest.open('POST', '/api', true);
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
