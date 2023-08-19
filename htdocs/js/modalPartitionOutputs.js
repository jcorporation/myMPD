"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
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
}

/**
 * Shows the partition outputs modal
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showPartitionOutputsModal() {
    //get all outputs
    sendAPIpartition("default", "MYMPD_API_PLAYER_OUTPUT_LIST", {}, function(obj) {
        const outputList = elGetById('modalPartitionOutputsList');
        if (checkResult(obj, outputList) === false) {
            return;
        }
        allOutputs = obj.result.data;
        //get partition specific outputs
        sendAPI("MYMPD_API_PLAYER_OUTPUT_LIST", {}, function() {
            parsePartitionOutputsList(obj);
            uiElements.modalPartitionOutputs.show();
        }, true);
    }, true);
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
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parsePartitionOutputsList(obj) {
    const outputList = elGetById('modalPartitionOutputsList');
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
