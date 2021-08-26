"use strict";
// SPDX-License-Identifier: GPL-2.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function initTrigger() {
    document.getElementById('listTriggerList').addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (event.target.nodeName === 'TD') {
            const id = decodeURI(event.target.parentNode.getAttribute('data-trigger-id'));
            showEditTrigger(id);
        }
        else if (event.target.nodeName === 'A') {
            const action = event.target.getAttribute('data-action');
            const id = decodeURI(event.target.parentNode.parentNode.getAttribute('data-trigger-id'));
            if (action === 'delete') {
                deleteTrigger(id);
            }
        }
    }, false);

    document.getElementById('selectTriggerScript').addEventListener('change', function() {
        selectTriggerActionChange();
    }, false);

    document.getElementById('modalTrigger').addEventListener('shown.bs.modal', function () {
        hideModalAlert();
        showListTrigger();
    });
}

//eslint-disable-next-line no-unused-vars
function saveTrigger() {
    let formOK = true;
    
    const nameEl = document.getElementById('inputTriggerName');
    if (!validatePlnameEl(nameEl)) {
        formOK = false;
    }
    
    const scriptEl = document.getElementById('selectTriggerScript');
    if (!validateSelect(scriptEl)) {
        formOK = false;
    }
    
    if (formOK === true) {
        const args = {};
        const argEls = document.getElementById('triggerActionScriptArguments').getElementsByTagName('input');
        for (let i = 0, j = argEls.length; i < j; i ++) {
            args[getCustomDomProperty(argEls[i], 'data-name')] = argEls[i].value;
        }

        sendAPI("MYMPD_API_TRIGGER_SAVE", {
            "id": Number(document.getElementById('inputTriggerId').value),
            "name": nameEl.value,
            "event": getSelectValue('selectTriggerEvent'),
            "script": getSelectValue('selectTriggerScript'),
            "arguments": args
            }, saveTriggerCheckError, true);
    }
}

function saveTriggerCheckError(obj) {
    removeEnterPinFooter(document.getElementById('modalTrigger').getElementsByClassName('enterPinFooter')[0]);
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        hideModalAlert();
        showListTrigger();
    }
}

//eslint-disable-next-line no-unused-vars
function showEditTrigger(id) {
    removeEnterPinFooter(document.getElementById('modalTrigger').getElementsByClassName('enterPinFooter')[0]);
    document.getElementById('listTrigger').classList.remove('active');
    document.getElementById('newTrigger').classList.add('active');
    document.getElementById('listTriggerFooter').classList.add('hide');
    document.getElementById('newTriggerFooter').classList.remove('hide');
    
    const nameEl = document.getElementById('inputTriggerName');
    removeIsInvalid(document.getElementById('modalTrigger'));
    nameEl.value = '';
    nameEl.focus();
    document.getElementById('inputTriggerId').value = '-1';
    document.getElementById('selectTriggerEvent').selectedIndex = 0;
    document.getElementById('selectTriggerScript').selectedIndex = 0;
    if (id > -1) {
        sendAPI("MYMPD_API_TRIGGER_GET", {"id": id}, parseTriggerEdit, false);
    }
    else {
        selectTriggerActionChange();
    }
}

function parseTriggerEdit(obj) {
    document.getElementById('inputTriggerId').value = obj.result.id;
    document.getElementById('inputTriggerName').value = obj.result.name;
    document.getElementById('selectTriggerEvent').value = obj.result.event;
    document.getElementById('selectTriggerScript').value = obj.result.script;
    selectTriggerActionChange(obj.result.arguments);
}

function selectTriggerActionChange(values) {
    const el = document.getElementById('selectTriggerScript');
    if (el.selectedIndex > -1) {
        showTriggerScriptArgs(el.options[el.selectedIndex], values);
    }
}

function showTriggerScriptArgs(option, values) {
    if (values === undefined) {
        values = {};
    }
    const args = JSON.parse(getCustomDomProperty(option, 'data-arguments'));
    let list = '';
    for (let i = 0, j = args.arguments.length; i < j; i++) {
        list += '<div class="form-group row">' +
                  '<label class="col-sm-4 col-form-label" for="triggerActionScriptArguments' + i + '">' + e(args.arguments[i]) + '</label>' +
                  '<div class="col-sm-8">' +
                    '<input name="triggerActionScriptArguments' + i + '" class="form-control border-secondary" type="text" value="' +
                    (values[args.arguments[i]] ? e(values[args.arguments[i]]) : '') + '"' +
                    'data-name="' + encodeURI(args.arguments[i]) + '">' +
                  '</div>' +
                '</div>';
    }
    if (args.arguments.length === 0) {
        list = t('No arguments');
    }
    document.getElementById('triggerActionScriptArguments').innerHTML = list;
}

function showListTrigger() {
    removeEnterPinFooter(document.getElementById('modalTrigger').getElementsByClassName('enterPinFooter')[0]);
    document.getElementById('listTrigger').classList.add('active');
    document.getElementById('newTrigger').classList.remove('active');
    document.getElementById('listTriggerFooter').classList.remove('hide');
    document.getElementById('newTriggerFooter').classList.add('hide');
    sendAPI("MYMPD_API_TRIGGER_LIST", {}, parseTriggerList, false);
}

function deleteTrigger(id) {
    sendAPI("MYMPD_API_TRIGGER_RM", {"id": id}, function() {
        sendAPI("MYMPD_API_TRIGGER_LIST", {}, parseTriggerList, false);
    }, true);
}

function parseTriggerList(obj) {
    if (obj.result.returnedEntities > 0) {
        let triggerList = '';
        for (let i = 0; i < obj.result.returnedEntities; i++) {
            triggerList += '<tr data-trigger-id="' + encodeURI(obj.result.data[i].id) + '"><td class="' +
                (obj.result.data[i].name === settings.trigger ? 'font-weight-bold' : '') +
                '">' + e(obj.result.data[i].name) + 
                '</td>' +
                '<td>' + t(obj.result.data[i].eventName) + '</td>' +
                '<td>' + e(obj.result.data[i].script) + '</td>' +
                '<td data-col="Action">' +
                (obj.result.data[i].name === 'default' || obj.result.data[i].name === settings.trigger  ? '' : 
                    '<a href="#" title="' + t('Delete') + '" data-action="delete" class="mi color-darkgrey">delete</a>') +
                '</td></tr>';
        }
        document.getElementById('listTriggerList').innerHTML = triggerList;
    }
    else {
        document.getElementById('listTriggerList').innerHTML = '<tr class="not-clickable">' +
            '<td colspan="3"><span class="mi">info</span>&nbsp;&nbsp;' + t('Empty list') + '</td></tr>';
    }
}
