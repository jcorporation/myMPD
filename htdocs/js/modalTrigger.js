"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalTrigger_js */

/**
 * Initialization function for trigger elements
 * @returns {void}
 */
function initModalTrigger() {
    elGetById('modalTriggerList').addEventListener('click', function(event) {
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

        const target = event.target.closest('TR');
        if (checkTargetClick(target) === true) {
            showEditTrigger(getData(target, 'trigger-id'));
        }
    }, false);

    elGetById('modalTriggerScriptInput').addEventListener('change', function() {
        selectTriggerActionChange();
    }, false);

    elGetById('modalTrigger').addEventListener('show.bs.modal', function () {
        showListTrigger();
    });
}

/**
 * Saves a trigger
 * @param {Element} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function saveTrigger(target) {
    cleanupModalId('modalTrigger');
    btnWaiting(target, true);

    const args = {};
    const argEls = document.querySelectorAll('#modalTriggerScriptArgumentsInput input');
    for (let i = 0, j = argEls.length; i < j; i ++) {
        args[getData(argEls[i], 'name')] = argEls[i].value;
    }

    let partition = getBtnGroupValueId('modalTriggerPartitionInput');
    partition = partition === '!all!'
        ? partition
        : localSettings.partition;

    sendAPI("MYMPD_API_TRIGGER_SAVE", {
        "id": getDataId('modalTriggerEditTab', 'id'),
        "name": elGetById('modalTriggerNameInput').value,
        "event": Number(getSelectValueId('modalTriggerEventInput')),
        "script": getSelectValueId('modalTriggerScriptInput'),
        "partition": partition,
        "arguments": args
    }, saveTriggerCheckError, true);
}

/**
 * Handler for the MYMPD_API_TRIGGER_SAVE jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function saveTriggerCheckError(obj) {
    if (modalApply(obj) === true) {
        showListTrigger();
    }
}

/**
 * Shows the edit trigger tab
 * @param {number} id trigger id
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showEditTrigger(id) {
    cleanupModalId('modalTrigger');
    elGetById('modalTriggerListTab').classList.remove('active');
    elGetById('modalTriggerEditTab').classList.add('active');
    elHideId('modalTriggerListFooter');
    elShowId('modalTriggerEditFooter');

    const nameEl = elGetById('modalTriggerNameInput');
    setFocus(nameEl);

    if (id > -1) {
        sendAPI("MYMPD_API_TRIGGER_GET", {
            "id": id
        }, parseTriggerEdit, false);
    }
    else {
        nameEl.value = '';
        setDataId('modalTriggerEditTab', 'id', -1);
        elGetById('modalTriggerEventInput').selectedIndex = 0;
        elGetById('modalTriggerScriptInput').selectedIndex = 0;
        toggleBtnGroupValueId('modalTriggerPartitionInput', 'this');
        selectTriggerActionChange();
    }
}

/**
 * Parses the MYMPD_API_TRIGGER_GET jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parseTriggerEdit(obj) {
    setDataId('modalTriggerEditTab', 'id', obj.result.id);
    elGetById('modalTriggerNameInput').value = obj.result.name;
    elGetById('modalTriggerEventInput').value = obj.result.event;
    elGetById('modalTriggerScriptInput').value = obj.result.script;
    const partition = obj.result.partition === '!all!'
        ? obj.result.partition
        : 'this';
    toggleBtnGroupValueId('modalTriggerPartitionInput', partition);
    selectTriggerActionChange(obj.result.arguments);
}

/**
 * Calls showTriggerScriptArgs for the selected script
 * @param {object} [values] array of values for the script arguments
 * @returns {void}
 */
function selectTriggerActionChange(values) {
    const el = elGetById('modalTriggerScriptInput');
    if (el.selectedIndex > -1) {
        showTriggerScriptArgs(el.options[el.selectedIndex], values);
    }
}

/**
 * Shows the list of arguments and values for the selected script
 * @param {HTMLElement} option selected option from script select
 * @param {object} values array of values for the script arguments
 * @returns {void}
 */
function showTriggerScriptArgs(option, values) {
    if (values === undefined) {
        values = {};
    }
    const args = getData(option, 'arguments');
    const list = elGetById('modalTriggerScriptArgumentsInput');
    elClear(list);
    for (let i = 0, j = args.arguments.length; i < j; i++) {
        const input = elCreateEmpty('input', {"class": ["form-control"], "type": "text", "name": "modalTriggerScriptArgumentsInput" + i,
            "value": (values[args.arguments[i]] ? values[args.arguments[i]] : '')});
        setData(input, 'name', args.arguments[i]);
        const fg = elCreateNodes('div', {"class": ["form-group", "row", "mb-3"]}, [
            elCreateText('label', {"class": ["col-sm-4", "col-form-label"], "for": "modalTriggerScriptArgumentsInput" + i}, args.arguments[i]),
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
 * @returns {void}
 */
function showListTrigger() {
    cleanupModalId('modalTrigger');
    elGetById('modalTriggerListTab').classList.add('active');
    elGetById('modalTriggerEditTab').classList.remove('active');
    elShowId('modalTriggerListFooter');
    elHideId('modalTriggerEditFooter');
    sendAPI("MYMPD_API_TRIGGER_LIST", {}, parseTriggerList, true);
}

/**
 * Parses the MYMPD_API_TRIGGER_LIST jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parseTriggerList(obj) {
    const table = elGetById('modalTriggerList');
    const tbody = table.querySelector('tbody');
    elClear(tbody);
    if (checkResult(obj, table, 'table') === false) {
        return;
    }

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
 * @returns {void}
 */
function deleteTrigger(el, id) {
    showConfirmInline(el.parentNode.previousSibling, tn('Do you really want to delete the trigger?'), tn('Yes, delete it'), function() {
        sendAPI("MYMPD_API_TRIGGER_RM", {
            "id": id
        }, saveTriggerCheckError, true);
    });
}

/**
 * Populates the trigger event select
 * @returns {void}
 */
function populateTriggerEvents() {
    const triggerEventList = elGetById('modalTriggerEventInput');
    elClear(triggerEventList);
    for (const event in settings.triggerEvents) {
        triggerEventList.appendChild(
            elCreateTextTn('option', {"value": settings.triggerEvents[event]}, event)
        );
    }
}
