"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalPartitions_js */

/**
 * Initialization function for the partition elements
 * @returns {void}
 */
function initModalPartitions() {
    document.getElementById('listPartitionsList').addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (event.target.nodeName === 'A') {
            const action = getData(event.target, 'action');
            const partition = getData(event.target.parentNode.parentNode, 'partition');
            switch(action) {
                case 'delete':
                    deletePartition(event.target, partition);
                    break;
                default:
                    logError('Invalid action: ' + action);
            }
            return;
        }
        const target = event.target.closest('TR');
        if (checkTargetClick(target) === true) {
            switchPartition(getData(target, 'partition'));
        }
    }, false);

    document.getElementById('partitionOutputsList').addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (event.target.nodeName === 'BUTTON') {
            toggleBtnChk(event.target, undefined);
        }
        else if (event.target.nodeName === 'TD') {
            const target = event.target.parentNode.firstChild.firstChild;
            toggleBtnChk(target, undefined);
        }
    }, false);

    document.getElementById('modalPartitions').addEventListener('shown.bs.modal', function () {
        showListPartitions();
    });

    document.getElementById('modalPartitionOutputs').addEventListener('shown.bs.modal', function () {
        //get all outputs
        sendAPIpartition("default", "MYMPD_API_PLAYER_OUTPUT_LIST", {}, function(obj) {
            const outputList = document.getElementById('partitionOutputsList');
            if (checkResult(obj, outputList) === false) {
                return;
            }
            allOutputs = obj.result.data;
            //get partition specific outputs
            sendAPI("MYMPD_API_PLAYER_OUTPUT_LIST", {}, parsePartitionOutputsList, true);
        }, true);
    });
}

/**
 * Moves the selected outputs to the current partition
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function moveOutputs() {
    const outputs = [];
    const selection = document.querySelectorAll('#partitionOutputsList .active');
    if (selection.length === 0) {
        return;
    }
    for (let i = 0, j = selection.length; i < j; i++) {
        outputs.push(getData(selection[i].parentNode.parentNode, 'output'));
    }
    sendAPI("MYMPD_API_PARTITION_OUTPUT_MOVE", {
        "outputs": outputs
    }, moveOutputsCheckError, true);
}

/**
 * Handler for the MYMPD_API_PARTITION_OUTPUT_MOVE jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function moveOutputsCheckError(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        uiElements.modalPartitionOutputs.hide();
        showNotification(tn('Outputs moved to current partition'), 'general', 'info');
    }
}

/**
 * Parses the MYMPD_API_PLAYER_OUTPUT_LIST jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parsePartitionOutputsList(obj) {
    const outputList = document.getElementById('partitionOutputsList');
    if (checkResult(obj, outputList) === false) {
        return;
    }

    elClear(outputList);
    /** @type {object} */
    const curOutputs = [];
    for (let i = 0; i < obj.result.numOutputs; i++) {
        if (obj.result.data[i].plugin !== 'dummy') {
            curOutputs.push(obj.result.data[i].name);
        }
    }

    const selBtn = elCreateText('button', {"class": ["btn", "btn-secondary", "btn-xs", "mi", "mi-sm", "me-3"]}, 'radio_button_unchecked');

    let nr = 0;
    for (let i = 0, j = allOutputs.length; i < j; i++) {
        if (curOutputs.includes(allOutputs[i].name) === false) {
            const tr = elCreateNode('tr', {},
                elCreateNodes('td', {}, [
                    selBtn.cloneNode(true),
                    document.createTextNode(allOutputs[i].name)
                ])
            );
            setData(tr, 'output', allOutputs[i].name);
            outputList.appendChild(tr);
            nr++;
        }
    }
    if (nr === 0) {
        outputList.appendChild(emptyRow(1));
    }
}

/**
 * Creates a new partition
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function savePartition() {
    cleanupModalId('modalPartitions');
    let formOK = true;

    const nameEl = document.getElementById('inputPartitionName');
    if (!validatePlistEl(nameEl)) {
        formOK = false;
    }

    if (formOK === true) {
        sendAPI("MYMPD_API_PARTITION_NEW", {
            "name": nameEl.value
        }, savePartitionCheckError, true);
    }
}

/**
 * Handler for the MYMPD_API_PARTITION_NEW jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function savePartitionCheckError(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        showListPartitions();
    }
}

/**
 * Shows the new partition tab
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showNewPartition() {
    cleanupModalId('modalPartitions');
    document.getElementById('listPartitions').classList.remove('active');
    document.getElementById('newPartition').classList.add('active');
    elHideId('listPartitionsFooter');
    elShowId('newPartitionFooter');
    const nameEl = document.getElementById('inputPartitionName');
    nameEl.value = '';
    setFocus(nameEl);
}

/**
 * Shows the list partition tab
 * @returns {void}
 */
function showListPartitions() {
    cleanupModalId('modalPartitions');
    document.getElementById('listPartitions').classList.add('active');
    document.getElementById('newPartition').classList.remove('active');
    elShowId('listPartitionsFooter');
    elHideId('newPartitionFooter');
    sendAPI("MYMPD_API_PARTITION_LIST", {}, parsePartitionList, true);
}

/**
 * Deletes a partition
 * @param {EventTarget} el triggering element
 * @param {string} partition partition name to delete
 * @returns {void}
 */
function deletePartition(el, partition) {
    showConfirmInline(el.parentNode.previousSibling, tn('Do you really want to delete the partition?', {"partition": partition}), tn('Yes, delete it'), function() {
        sendAPIpartition("default", "MYMPD_API_PARTITION_RM", {
            "name": partition
        }, savePartitionCheckError, true);
    });  
}

/**
 * Switches the current browser session to a partition
 * @param {string} partition partition name to switch to
 * @returns {void}
 */
function switchPartition(partition) {
    //save localSettings in browsers localStorage
    localSettings.partition = partition;
    try {
        localStorage.setItem('partition', partition);
    }
    catch(err) {
        const obj = {
            "error": {
                "message": "Can not save settings to localStorage: %{error}",
                "data": {
                    "error": err.message
                }
            }
        };
        showModalAlert(obj);
        return;
    }
    //reconnect websocket to new ws endpoint
    setTimeout(function() {
        webSocketClose();
        webSocketConnect();
    }, 0);
    getSettings();
    BSN.Modal.getInstance(document.getElementById('modalPartitions')).hide();
    showNotification(tn('Partition switched'), 'general', 'info');
}

/**
 * Parses the MYMPD_API_PARTITION_LIST jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parsePartitionList(obj) {
    const partitionList = document.getElementById('listPartitionsList');
    if (checkResult(obj, partitionList) === false) {
        return;
    }

    elClear(partitionList);

    for (let i = 0, j = obj.result.data.length; i < j; i++) {
        const tr = elCreateEmpty('tr', {});
        setData(tr, 'partition', obj.result.data[i].name);
        if (obj.result.data[i].name !== localSettings.partition) {
            tr.setAttribute('title', tn('Switch to'));
        }
        else {
            tr.classList.add('not-clickable');
            tr.setAttribute('title', tn('Active partition'));
        }
        const tdColor = elCreateText('span', {"class": ["mi", "me-2"]}, 'dashboard');
        tdColor.style.color = obj.result.data[i].highlightColor;
        const td = elCreateEmpty('td', {});
        td.appendChild(tdColor);
        if (obj.result.data[i].name === localSettings.partition) {
            td.classList.add('fw-bold');
            td.appendChild(document.createTextNode(obj.result.data[i].name + ' (' + tn('current') + ')'));
        }
        else {
            td.appendChild(document.createTextNode(obj.result.data[i].name));
        }
        tr.appendChild(td);
        const partitionActionTd = elCreateEmpty('td', {"data-col": "Action"});
        if (obj.result.data[i].name !== 'default' &&
            obj.result.data[i].name !== localSettings.partition)
        {
            partitionActionTd.appendChild(
                elCreateText('a', {"href": "#", "data-title-phrase": "Delete", "data-action": "delete", "class": ["mi", "color-darkgrey", "me-2"]}, 'delete')
            );
        }
        tr.appendChild(partitionActionTd);
        partitionList.appendChild(tr);
    }
}
