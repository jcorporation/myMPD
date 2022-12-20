"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module trigger_js */

/**
 * Initialization function for trigger elements
 */
function initTrigger() {
    document.getElementById('listTriggerList').addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (event.target.nodeName === 'A') {
            const action = getData(event.target, 'action');
            const id = getData(event.target.parentNode.parentNode, 'trigger-id');
            if (action === 'delete') {
                deleteTrigger(event.target, id);
            }
            return;
        }

        const target = getParent(event.target, 'TR');
        if (checkTargetClick(target) === true) {
            showEditTrigger(getData(target, 'trigger-id'));
        }
    }, false);

    document.getElementById('selectTriggerScript').addEventListener('change', function() {
        selectTriggerActionChange();
    }, false);

    document.getElementById('modalTrigger').addEventListener('shown.bs.modal', function () {
        showListTrigger();
    });
}

/**
 * Saves a trigger
 */
//eslint-disable-next-line no-unused-vars
function saveTrigger() {
    cleanupModalId('modalTrigger');
    let formOK = true;

    const nameEl = document.getElementById('inputTriggerName');
    if (!validatePlistEl(nameEl)) {
        formOK = false;
    }

    const scriptEl = document.getElementById('selectTriggerScript');
    if (!validateSelectEl(scriptEl)) {
        formOK = false;
    }

    if (formOK === true) {
        const args = {};
        const argEls = document.querySelectorAll('#triggerActionScriptArguments input');
        for (let i = 0, j = argEls.length; i < j; i ++) {
            args[getData(argEls[i], 'name')] = argEls[i].value;
        }

        let partition = getBtnGroupValueId('btnTriggerPartitionGroup');
        partition = partition === '!all!' ? partition : localSettings.partition;

        sendAPI("MYMPD_API_TRIGGER_SAVE", {
            "id": Number(document.getElementById('inputTriggerId').value),
            "name": nameEl.value,
            "event": Number(getSelectValueId('selectTriggerEvent')),
            "script": getSelectValueId('selectTriggerScript'),
            "partition": partition,
            "arguments": args
        }, saveTriggerCheckError, true);
    }
}

/**
 * Handler for the MYMPD_API_TRIGGER_SAVE jsonrpc response
 * @param {object} obj jsonrpc response
 */
function saveTriggerCheckError(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        showListTrigger();
    }
}

/**
 * Shows the edit trigger tab
 * @param {number} id trigger id
 */
//eslint-disable-next-line no-unused-vars
function showEditTrigger(id) {
    cleanupModalId('modalTrigger');
    document.getElementById('listTrigger').classList.remove('active');
    document.getElementById('newTrigger').classList.add('active');
    elHideId('listTriggerFooter');
    elShowId('newTriggerFooter');

    const nameEl = document.getElementById('inputTriggerName');
    setFocus(nameEl);

    if (id > -1) {
        sendAPI("MYMPD_API_TRIGGER_GET", {
            "id": id
        }, parseTriggerEdit, false);
    }
    else {
        nameEl.value = '';
        document.getElementById('inputTriggerId').value = '-1';
        document.getElementById('selectTriggerEvent').selectedIndex = 0;
        document.getElementById('selectTriggerScript').selectedIndex = 0;
        toggleBtnGroupValueId('btnTriggerPartitionGroup', 'this');
        selectTriggerActionChange();
    }
}

/**
 * Parses the MYMPD_API_TRIGGER_GET jsonrpc response
 * @param {object} obj jsonrpc response
 */
function parseTriggerEdit(obj) {
    document.getElementById('inputTriggerId').value = obj.result.id;
    document.getElementById('inputTriggerName').value = obj.result.name;
    document.getElementById('selectTriggerEvent').value = obj.result.event;
    document.getElementById('selectTriggerScript').value = obj.result.script;
    const partition = obj.result.partition === '!all!' ? obj.result.partition : 'this';
    toggleBtnGroupValueId('btnTriggerPartitionGroup', partition);
    selectTriggerActionChange(obj.result.arguments);
}

/**
 * Calls showTriggerScriptArgs for the selected script
 * @param {object} [values] array of values for the script arguments
 */
function selectTriggerActionChange(values) {
    const el = document.getElementById('selectTriggerScript');
    if (el.selectedIndex > -1) {
        showTriggerScriptArgs(el.options[el.selectedIndex], values);
    }
}

/**
 * Shows the list of arguments and values for the selected script
 * @param {HTMLElement} option selected option from script select
 * @param {object} values array of values for the script arguments
 */
function showTriggerScriptArgs(option, values) {
    if (values === undefined) {
        values = {};
    }
    const args = getData(option, 'arguments');
    const list = document.getElementById('triggerActionScriptArguments');
    elClear(list);
    for (let i = 0, j = args.arguments.length; i < j; i++) {
        const input = elCreateEmpty('input', {"class": ["form-control"], "type": "text", "name": "triggerActionScriptArguments" + i,
            "value": (values[args.arguments[i]] ? values[args.arguments[i]] : '')});
        setData(input, 'name', args.arguments[i]);
        const fg = elCreateNodes('div', {"class": ["form-group", "row", "mb-3"]}, [
            elCreateText('label', {"class": ["col-sm-4", "col-form-label"], "for": "triggerActionScriptArguments" + i}, args.arguments[i]),
            elCreateNode('div', {"class": ["col-sm-8"]}, input)
        ]);
        list.appendChild(fg);
    }
    if (args.arguments.length === 0) {
        list.textContent = tn('No arguments');
    }
}

/**
 * Shows the trigger list tab
 */
function showListTrigger() {
    cleanupModalId('modalTrigger');
    document.getElementById('listTrigger').classList.add('active');
    document.getElementById('newTrigger').classList.remove('active');
    elShowId('listTriggerFooter');
    elHideId('newTriggerFooter');
    sendAPI("MYMPD_API_TRIGGER_LIST", {}, parseTriggerList, true);
}

/**
 * Parses the MYMPD_API_TRIGGER_LIST jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parseTriggerList(obj) {
    const tbody = document.getElementById('listTriggerList');
    if (checkResult(obj, tbody) === false) {
        return;
    }
    elClear(tbody);
    for (let i = 0; i < obj.result.returnedEntities; i++) {
        const row = elCreateNodes('tr', {"title": tn('Edit')}, [
            elCreateText('td', {}, obj.result.data[i].name + 
                (obj.result.data[i].partition === '!all!' ? ' (' + tn('All partitions') + ')' : '')
            ),
            elCreateTextTn('td', {}, obj.result.data[i].eventName),
            elCreateText('td', {}, obj.result.data[i].script),
            elCreateNode('td', {"data-col": "Action"},
                elCreateText('a', {"href": "#", "data-title-phrase": "Delete", "data-action": "delete", "class": ["mi", "color-darkgrey"]}, 'delete')
            )
        ]);
        setData(row, 'trigger-id', obj.result.data[i].id);
        tbody.appendChild(row);
    }
}

/**
 * Deletes a trigger after confirmation
 * @param {EventTarget} el triggering element
 * @param {number} id trigger id
 */
function deleteTrigger(el, id) {
    showConfirmInline(el.parentNode.previousSibling, tn('Do you really want to delete the trigger?'), tn('Yes, delete it'), function() {
        sendAPI("MYMPD_API_TRIGGER_RM", {
            "id": id
        }, saveTriggerCheckError, true);
    });
}
