"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalVariables_js */

/**
 * Initialization functions for the script elements
 * @returns {void}
 */
function initModalVariables() {
    elGetById('modalVariablesList').addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (event.target.nodeName === 'A') {
            const action = getData(event.target, 'action');
            const key = getData(event.target.parentNode.parentNode, 'key');
            if (action === 'delete') {
                deleteVariables(key);
            }
            return;
        }

        const target = event.target.closest('TR');
        if (checkTargetClick(target) === true) {
            showVariablesEdit(target, true);
        }
    }, false);
}

/**
 * Opens the variables modal and shows the list tab
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showListVariablesModal() {
    uiElements.modalVariables.show();
    showVariablesList();
}

/**
 * Shows the scripts variables tab
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showVariablesList() {
    cleanupModalId('modalScripts');
    elGetById('modalVariablesListTab').classList.add('active');
    elGetById('modalVariablesEditTab').classList.remove('active');
    elShowId('modalVariablesListFooter');
    elHideId('modalVariablesEditFooter');
    getVariablesList();
}

/**
 * Edits a scripts variable
 * @param {EventTarget} el row with the data
 * @param {boolean} editVar edit variable?
 * @returns {void}
 */
function showVariablesEdit(el, editVar) {
    cleanupModalId('modalVariables');
    elGetById('modalVariablesEditTab').classList.add('active');
    elGetById('modalVariablesListTab').classList.remove('active');
    elShowId('modalVariablesEditFooter');
    elHideId('modalVariablesListFooter');
    if (editVar === true) {
        elGetById('modalVariablesKeyInput').setAttribute('readonly', 'readonly');
        elGetById('modalVariablesKeyInput').value = getData(el,'key');
        elGetById('modalVariablesValueInput').value = getData(el,'value');
        elGetById('modalVariablesValueInput').focus();
    }
    else {
        elGetById('modalVariablesKeyInput').removeAttribute('readonly');
        elGetById('modalVariablesKeyInput').value = '';
        elGetById('modalVariablesValueInput').value = '';
        elGetById('modalVariablesKeyInput').focus();
    }
}

/**
 * Deletes a scripts variable
 * @param {string} key script variable to delete
 * @returns {void}
 */
function deleteVariables(key) {
    cleanupModalId('modalVariables');
    sendAPI("MYMPD_API_SCRIPT_VAR_DELETE", {
        "key": key
    }, deleteVariablesCheckError, true);
}

/**
 * Handler for the MYMPD_API_SCRIPT_VAR_DELETE jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function deleteVariablesCheckError(obj) {
    if (modalListApply(obj) === true) {
        getVariablesList();
    }
}

/**
 * Saves a scripts variable
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function setVariables() {
    cleanupModalId('modalVariables');
    sendAPI("MYMPD_API_SCRIPT_VAR_SET", {
        "key": elGetById('modalVariablesKeyInput').value,
        "value": elGetById('modalVariablesValueInput').value
    }, setVariablesCheckError, true);
}

/**
 * Handler for the MYMPD_API_SCRIPT_VAR_DELETE jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function setVariablesCheckError(obj) {
    if (modalListApply(obj) === true) {
        showVariablesList();
    }
}

/**
 * Gets the list of scripts
 * @returns {void}
 */
function getVariablesList() {
    sendAPI("MYMPD_API_SCRIPT_VAR_LIST", {}, parseVariablesList, true);
}

/**
 * Parses the MYMPD_API_SCRIPT_LIST jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parseVariablesList(obj) {
    const table = document.querySelector('#modalVariablesList');
    const tbody = table.querySelector('tbody');
    elClear(tbody);
    if (checkResult(obj, table, 'table') === false) {
        return;
    }
    for (let i = 0; i < obj.result.returnedEntities; i++) {
        const row = elCreateNodes('tr', {"title": tn('Edit')}, [
            elCreateTextTn('td', {}, obj.result.data[i].key),
            elCreateText('td', {}, obj.result.data[i].value),
            elCreateNode('td', {"data-col": "Action"},
                elCreateText('a', {"href": "#", "data-title-phrase": "Delete", "data-action": "delete", "class": ["mi", "color-darkgrey"]}, 'delete')
            )
        ]);
        setData(row, 'key', obj.result.data[i].key);
        setData(row, 'value', obj.result.data[i].value);
        tbody.append(row);
    }
}
