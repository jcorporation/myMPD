"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalOutputAttributes_js */
/**
 * Shows the output attributes modal 
 * @param {string} outputName the output name
 * @returns {void}
 */
function showModalOutputAttributes(outputName) {
    cleanupModalId('modalOutputAttributes');
    sendAPI("MYMPD_API_PLAYER_OUTPUT_GET", {
        "outputName": outputName
    }, parseOutputAttributes, false);
}

/**
 * Creates the output attributes table content
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parseOutputAttributes(obj) {
    setDataId('modalOutputAttributes', 'outputId', obj.result.id);
    const table = elGetById('modalOutputAttributesList');
    const tbody = table.querySelector('tbody');
    elClear(tbody);
    if (checkResult(obj, table, 'table') === false) {
        return;
    }

    for (const n of ['name', 'enabled', 'plugin']) {
        if (n === 'enabled') {
            obj.result[n] = obj.result[n] === true
                ? tn('Enabled')
                : tn('Disabled');
        }
        tbody.appendChild(
            elCreateNodes('tr', {}, [
                elCreateTextTn('td', {}, n),
                elCreateText('td', {}, obj.result[n])
            ])
        );
    }
    let i = 0;
    for (const key in obj.result.attributes) {
        i++;
        tbody.appendChild(
            elCreateNodes('tr', {}, [
                elCreateText('td', {}, key),
                elCreateNode('td', {},
                    elCreateEmpty('input', {"name": key, "class": ["form-control"], "type": "text", "value": obj.result.attributes[key]})
                )
            ])
        );
    }
    if (i > 0) {
        elEnableId('modalOutputAttributesSaveBtn');
    }
    else {
        elDisableId('modalOutputAttributesSaveBtn');
    }
    uiElements.modalOutputAttributes.show();
}

/**
 * Saves the output attributes
 * @param {Element} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function saveOutputAttributes(target) {
    const modal = elGetById('modalOutputAttributes');
    cleanupModal(modal);
    const params = {};
    params.outputId = getData(modal, 'outputId');
    params.attributes = {};
    const els = document.querySelectorAll('#modalOutputAttributesList input');
    for (let i = 0, j = els.length; i < j; i++) {
        params.attributes[els[i].name] = els[i].value;
    }
    btnWaiting(target, true);
    sendAPI('MYMPD_API_PLAYER_OUTPUT_ATTRIBUTES_SET', params, modalClose, true);
}
