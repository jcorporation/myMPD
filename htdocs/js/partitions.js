"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function initPartitions() {
    document.getElementById('listPartitionsList').addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (event.target.nodeName === 'A') {
            const action = getData(event.target, 'action');
            const partition = getData(event.target.parentNode.parentNode, 'partition');
            if (action === 'delete') {
                deletePartition(partition);
            }
            else if (action === 'switch') {
                switchPartition(partition);
            }
        }
    }, false);

    document.getElementById('partitionOutputsList').addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (event.target.nodeName === 'TD') {
            const outputName = getData(event.target.parentNode, 'output');
            moveOutput(outputName);
            uiElements.modalPartitionOutputs.hide();
        }
    }, false);

    document.getElementById('modalPartitions').addEventListener('shown.bs.modal', function () {
        showListPartitions();
    });

    document.getElementById('modalPartitionOutputs').addEventListener('shown.bs.modal', function () {
        sendAPI("MYMPD_API_PLAYER_OUTPUT_LIST", {
            "partition": "default"
        }, parsePartitionOutputsList, true);
    });
}

function moveOutput(output) {
    sendAPI("MYMPD_API_PARTITION_OUTPUT_MOVE", {
        "name": output
    });
}

function parsePartitionOutputsList(obj) {
    const outputList = document.getElementById('partitionOutputsList');
    if (checkResult(obj, outputList) === false) {
        return;
    }

    elClear(outputList);
    const outputs = document.getElementById('outputs').getElementsByTagName('button');
    const outputIds = [];
    for (let i = 0, j= outputs.length; i < j; i++) {
        outputIds.push(getData(outputs[i], 'output-id'));
    }

    let nr = 0;
    for (let i = 0, j = obj.result.data.length; i < j; i++) {
        if (outputIds.includes(obj.result.data[i].id) === false) {
            const tr = elCreateNode('tr', {},
                elCreateText('td', {}, obj.result.data[i].name)
            );
            setData(tr, 'output', obj.result.data[i].name);
            outputList.appendChild(tr);
            nr++;
        }
    }
    if (nr === 0) {
        outputList.appendChild(emptyRow(1));
    }
}

//eslint-disable-next-line no-unused-vars
function savePartition() {
    cleanupModalId('modalPartitions');
    let formOK = true;

    const nameEl = document.getElementById('inputPartitionName');
    if (!validatePlnameEl(nameEl)) {
        formOK = false;
    }

    if (formOK === true) {
        sendAPI("MYMPD_API_PARTITION_NEW", {
            "name": nameEl.value
        }, savePartitionCheckError, true);
    }
}

function savePartitionCheckError(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        showListPartitions();
    }
}

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

function showListPartitions() {
    cleanupModalId('modalPartitions');
    document.getElementById('listPartitions').classList.add('active');
    document.getElementById('newPartition').classList.remove('active');
    elShowId('listPartitionsFooter');
    elHideId('newPartitionFooter');
    sendAPI("MYMPD_API_PARTITION_LIST", {}, parsePartitionList, true);
}

function deletePartition(partition) {
    sendAPI("MYMPD_API_PARTITION_RM", {
        "name": partition
    }, savePartitionCheckError, true);
}

function switchPartition(partition) {
    sendAPI("MYMPD_API_PARTITION_SWITCH", {
        "name": partition
    }, function(obj) {
        savePartitionCheckError(obj);
        sendAPI("MYMPD_API_PLAYER_STATE", {}, parseState);
    }, true);
}

function parsePartitionList(obj) {
    const partitionList = document.getElementById('listPartitionsList');
    if (checkResult(obj, partitionList) === false) {
        return;
    }

    elClear(partitionList);

    for (let i = 0, j = obj.result.data.length; i < j; i++) {
        const tr = elCreateEmpty('tr', {});
        setData(tr, 'partition', obj.result.data[i].name);
        const td = elCreateEmpty('td', {});
        if (obj.result.data[i].name === settings.partition) {
            td.classList.add('font-weight-bold');
            td.textContent = obj.result.data[i].name + ' (' + tn('current') + ')';
        }
        else {
            td.textContent = obj.result.data[i].name;
        }
        tr.appendChild(td);
        const partitionActionTd = elCreateEmpty('td', {"data-col": "Action"});
        if (obj.result.data[i].name !== 'default' && obj.result.data[i].name !== settings.partition) {
            partitionActionTd.appendChild(
                elCreateText('a', {"href": "#", "title": tn('Delete'), "data-action": "delete", "class": ["mi", "color-darkgrey", "me-2"]}, 'delete')
            );
        }
        if (obj.result.data[i].name !== settings.partition) {
            partitionActionTd.appendChild(
                elCreateText('a', {"href": "#", "title": tn('Switch to'), "data-action": "switch", "class": ["mi", "color-darkgrey"]}, 'check_circle')
            );
        }
        tr.appendChild(partitionActionTd);
        partitionList.appendChild(tr);
    }
}
