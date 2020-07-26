"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

//eslint-disable-next-line no-unused-vars
function saveTrigger() {
    let formOK = true;
    
    let nameEl = document.getElementById('inputTriggerName');
    if (!validatePlnameEl(nameEl)) {
        formOK = false;
    }
    
    if (formOK === true) {
        sendAPI("MPD_API_TRIGGER_SAVE", {
            "id": parseInt(document.getElementById('inputTriggerId').value),
            "name": nameEl.value,
            "event": getSelectValue('selectTriggerEvent'),
            "script": getSelectValue('selectTriggerScript')
            }, showListTrigger, false);
    }
}

//eslint-disable-next-line no-unused-vars
function showEditTrigger(id) {
    document.getElementById('listTrigger').classList.remove('active');
    document.getElementById('newTrigger').classList.add('active');
    document.getElementById('listTriggerFooter').classList.add('hide');
    document.getElementById('newTriggerFooter').classList.remove('hide');
    
    const nameEl = document.getElementById('inputTriggerName');
    nameEl.classList.remove('is-invalid');
    nameEl.value = '';
    nameEl.focus();
    document.getElementById('inputTriggerId').value = '-1';
    document.getElementById('selectTriggerEvent').selectedIndex = 0;
    document.getElementById('selectTriggerScript').selectedIndex = 0;
    if (id > -1) {
        sendAPI("MPD_API_TRIGGER_GET", {"id": id}, parseTriggerEdit, false);
    }
}

function parseTriggerEdit(obj) {
    document.getElementById('inputTriggerId').value = obj.result.id;
    document.getElementById('inputTriggerName').value = obj.result.name;
    document.getElementById('selectTriggerEvent').value = obj.result.event;
    document.getElementById('selectTriggerScript').value = obj.result.script;
}

function showListTrigger() {
    document.getElementById('listTrigger').classList.add('active');
    document.getElementById('newTrigger').classList.remove('active');
    document.getElementById('listTriggerFooter').classList.remove('hide');
    document.getElementById('newTriggerFooter').classList.add('hide');
    sendAPI("MPD_API_TRIGGER_LIST", {}, parseTriggerList, false);
}

function deleteTrigger(id) {
    sendAPI("MPD_API_TRIGGER_DELETE", {"id": id}, function(obj) {
        sendAPI("MPD_API_TRIGGER_LIST", {}, parseTriggerList, false);
    }, true);
}

function parseTriggerList(obj) {
    if (obj.result.data.length > 0) {
        let triggerList = '';
        for (let i = 0; i < obj.result.data.length; i++) {
            triggerList += '<tr data-trigger-id="' + encodeURI(obj.result.data[i].id) + '"><td class="' +
                (obj.result.data[i].name === settings.trigger ? 'font-weight-bold' : '') +
                '">' + e(obj.result.data[i].name) + 
                '</td>' +
                '<td>' + t(obj.result.data[i].eventName) + '</td>' +
                '<td>' + e(obj.result.data[i].script) + '</td>' +
                '<td data-col="Action">' +
                (obj.result.data[i].name === 'default' || obj.result.data[i].name === settings.trigger  ? '' : 
                    '<a href="#" title="' + t('Delete') + '" data-action="delete" class="material-icons color-darkgrey">delete</a>') +
                '</td></tr>';
        }
        document.getElementById('listTriggerList').innerHTML = triggerList;
    }
    else {
        document.getElementById('listTriggerList').innerHTML = '<tr class="not-clickable"><td><span class="material-icons">error_outline</span></td>' +
            '<td colspan="2">' + t('Empty list') + '</td></tr>';
    }
}
