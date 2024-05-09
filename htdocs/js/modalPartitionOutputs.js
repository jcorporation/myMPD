"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalPartitionOutputs_js */

/**
 * Initialization function for the partition elements
 * @returns {void}
 */
function initModalPartitionOutputs() {
    elGetById('modalPartitionOutputsList').addEventListener('click', function(event) {
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

    elGetById('modalPartitionOutputs').addEventListener('show.bs.modal', function() {
        //get all outputs
        sendAPIpartition("default", "MYMPD_API_PLAYER_OUTPUT_LIST", {}, function(allOutputs) {
            const outputList = elGetById('modalPartitionOutputsList');
            if (checkResult(allOutputs, outputList, 'table') === false) {
                return;
            }
            //get partition specific outputs
            sendAPI("MYMPD_API_PLAYER_OUTPUT_LIST", {}, function(partitionOutputs) {
                parsePartitionOutputsList(allOutputs, partitionOutputs);
                uiElements.modalPartitionOutputs.show();
            }, true);
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
    const selection = document.querySelectorAll('#modalPartitionOutputsList .active');
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
 * @param {object} allOutputs jsonrpc response listing all outputs of mpd
 * @param {object} partitionOutputs jsonrpc response listing all outputs of current partition
 * @returns {void}
 */
function parsePartitionOutputsList(allOutputs, partitionOutputs) {
    const table = elGetById('modalPartitionOutputsList');
    const tbody = table.querySelector('tbody');
    elClear(tbody);
    //checkResult can not be used here because the displayed result count is determined below
    if (partitionOutputs.error) {
        tbody.appendChild(errorMsgEl(partitionOutputs, 1, 'table'));
        return;
    }
    /** @type {object} */
    const curOutputs = [];
    for (let i = 0; i < partitionOutputs.result.returnedEntities; i++) {
        if (partitionOutputs.result.data[i].plugin !== 'dummy') {
            curOutputs.push(partitionOutputs.result.data[i].name);
        }
    }

    const selBtn = elCreateText('button', {"class": ["btn", "btn-secondary", "btn-xs", "mi", "mi-sm", "me-3"]}, 'radio_button_unchecked');
    let count = 0;
    for (let i = 0; i < allOutputs.result.returnedEntities; i++) {
        if (curOutputs.includes(allOutputs.result.data[i].name) === false) {
            const tr = elCreateNode('tr', {},
                elCreateNodes('td', {}, [
                    selBtn.cloneNode(true),
                    document.createTextNode(allOutputs.result.data[i].name)
                ])
            );
            setData(tr, 'output', allOutputs.result.data[i].name);
            tbody.appendChild(tr);
            count++;
        }
    }
    if (count === 0) {
        tbody.appendChild(emptyMsgEl(1, 'table'));
    }
}
