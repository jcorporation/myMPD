"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalOutputAttributes_js */
/**
 * Shows the output attributes modal 
 * @param {string} outputName the output name
 * @returns {void}
 */
function showListOutputAttributes(outputName) {
    cleanupModalId('modalOutputAttributes');
    sendAPI("MYMPD_API_PLAYER_OUTPUT_LIST", {}, function(obj) {
        const tbody = document.getElementById('outputAttributesList');
        if (checkResult(obj, tbody) === false) {
            return;
        }
        //we get all outputs, filter by outputName
        for (const output of obj.result.data) {
            if (output.name === outputName) {
                parseOutputAttributes(output);
                break;
            }
        }
    }, false);
    uiElements.modalOutputAttributes.show();
}

/**
 * Creates the output attributes table content
 * @param {object} output output object
 * @returns {void}
 */
function parseOutputAttributes(output) {
    document.getElementById('modalOutputAttributesId').value = output.id;
    const tbody = document.getElementById('outputAttributesList');
    elClear(tbody);
    for (const n of ['name', 'state', 'plugin']) {
        if (n === 'state') {
            output[n] = output[n] === 1 ? tn('Enabled') : tn('Disabled');
        }
        tbody.appendChild(
            elCreateNodes('tr', {}, [
                elCreateTextTn('td', {}, n),
                elCreateText('td', {}, output[n])
            ])
        );
    }
    let i = 0;
    for (const key in output.attributes) {
        i++;
        tbody.appendChild(
            elCreateNodes('tr', {}, [
                elCreateText('td', {}, key),
                elCreateNode('td', {},
                    elCreateEmpty('input', {"name": key, "class": ["form-control"], "type": "text", "value": output.attributes[key]})
                )
            ])
        );
    }
    if (i > 0) {
        elEnableId('btnOutputAttributesSave');
    }
    else {
        elDisableId('btnOutputAttributesSave');
    }
}

/**
 * Saves the output attributes
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function saveOutputAttributes() {
    cleanupModalId('modalOutputAttributes');
    const params = {};
    params.outputId = Number(document.getElementById('modalOutputAttributesId').value);
    params.attributes = {};
    const els = document.querySelectorAll('#outputAttributesList input');
    for (let i = 0, j = els.length; i < j; i++) {
        params.attributes[els[i].name] = els[i].value;
    }
    sendAPI('MYMPD_API_PLAYER_OUTPUT_ATTRIBUTES_SET', params, saveOutputAttributesClose, true);
}

/**
 * Handler for MYMPD_API_PLAYER_OUTPUT_ATTRIBUTES_SET response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function saveOutputAttributesClose(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        uiElements.modalOutputAttributes.hide();
    }
}
