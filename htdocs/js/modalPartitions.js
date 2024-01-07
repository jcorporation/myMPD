"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalPartitions_js */

/**
 * Initialization function for the partition elements
 * @returns {void}
 */
function initModalPartitions() {
    elGetById('modalPartitionsPartitionsList').addEventListener('click', function(event) {
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

    elGetById('modalPartitions').addEventListener('show.bs.modal', function () {
        showListPartitions();
    });
}

/**
 * Creates a new partition
 * @param {Element} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function savePartition(target) {
    cleanupModalId('modalPartitions');
    btnWaiting(target, true);
    sendAPI("MYMPD_API_PARTITION_NEW", {
        "name": elGetById('modalPartitionsNameInput').value
    }, savePartitionCheckError, true);
}

/**
 * Handler for the MYMPD_API_PARTITION_NEW jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function savePartitionCheckError(obj) {
    if (modalApply(obj) === true) {
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
    elGetById('modalPartitionsListTab').classList.remove('active');
    elGetById('modalPartitionsNewTab').classList.add('active');
    elHideId('modalPartitionsListFooter');
    elShowId('modalPartitionsNewFooter');
    const nameEl = elGetById('modalPartitionsNameInput');
    nameEl.value = '';
    setFocus(nameEl);
}

/**
 * Shows the list partition tab
 * @returns {void}
 */
function showListPartitions() {
    cleanupModalId('modalPartitions');
    elGetById('modalPartitionsListTab').classList.add('active');
    elGetById('modalPartitionsNewTab').classList.remove('active');
    elShowId('modalPartitionsListFooter');
    elHideId('modalPartitionsNewFooter');
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
    getSettings(parseSettings);
    uiElements.modalPartitions.hide();
    showNotification(tn('Partition switched'), 'general', 'info');
}

/**
 * Parses the MYMPD_API_PARTITION_LIST jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parsePartitionList(obj) {
    const partitionList = elGetById('modalPartitionsPartitionsList');
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
