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
            switch(action) {
                case 'delete':
                    deletePartition(event.target, partition);
                    break;
                case 'edit':
                    uiElements.modalPartitionSettings.show();
                    break;
            }
        }
        else if (event.target.nodeName === 'TD') {
            const partition = getData(event.target.parentNode, 'partition');
            if (partition === localSettings.partition) {
                return;
            }
            switchPartition(partition);
        }
    }, false);

    document.getElementById('partitionOutputsList').addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (event.target.nodeName === 'BUTTON') {
            toggleBtnChk(event.target);
        }
        else if (event.target.nodeName === 'TD') {
            const target = event.target.parentNode.firstChild.firstChild;
            toggleBtnChk(target);
        }
    }, false);

    document.getElementById('modalPartitions').addEventListener('shown.bs.modal', function () {
        showListPartitions();
    });

    document.getElementById('modalPartitionSettings').addEventListener('show.bs.modal', function () {
        cleanupModalId('modalPartitionSettings');
        document.getElementById('inputPartitionColor').value = settings.partition.color;
        document.getElementById('inputMpdStreamPort').value = settings.partition.mpdStreamPort;
        document.getElementById('inputStreamUri').value = settings.partition.streamUri;
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

//eslint-disable-next-line no-unused-vars
function moveOutputs() {
    const outputs = [];
    const selection = document.getElementById('partitionOutputsList').getElementsByClassName('active');
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

function moveOutputsCheckError(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        uiElements.modalPartitionOutputs.hide();
        showNotification(tn('Outputs moved to current partition'), '', 'general', 'info');
    }
}

function parsePartitionOutputsList(obj) {
    const outputList = document.getElementById('partitionOutputsList');
    if (checkResult(obj, outputList) === false) {
        return;
    }

    elClear(outputList);
    const curOutputs = [];
    for (let i = 0; i < obj.result.numOutputs; i++) {
        if (obj.result.data[i].plugin !== 'dummy') {
            curOutputs.push(obj.result.data[i].name);
        }
    }

    const selBtn = elCreateText('button', {"class": ["btn", "btn-secondary", "btn-xs", "mi", "mi-small", "me-3"]}, 'radio_button_unchecked');

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

function deletePartition(el, partition) {
    showConfirmInline(el.parentNode.previousSibling, tn('Do you really want to delete the partition?', {"partition": partition}), tn('Yes, delete it'), function() {
        sendAPI("MYMPD_API_PARTITION_RM", {
            "name": partition
        }, savePartitionCheckError, true);
    });  
}

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
    getSettings(true);
    BSN.Modal.getInstance(document.getElementById('modalPartitions')).hide();
    showNotification(tn('Partition switched'), '', 'general', 'info');
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
        if (obj.result.data[i].name !== localSettings.partition) {
            tr.setAttribute('title', tn('Switch to'));
        }
        else {
            tr.classList.add('not-clickable');
            tr.setAttribute('title', tn('Active partition'));
        }
        const tdColor = elCreateText('span', {"class": ["mi", "me-2"]}, 'dashboard');
        tdColor.style.color = obj.result.data[i].color;
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
                elCreateText('a', {"href": "#", "title": tn('Delete'), "data-action": "delete", "class": ["mi", "color-darkgrey", "me-2"]}, 'delete')
            );
        }
        else if (obj.result.data[i].name === localSettings.partition) {
            partitionActionTd.appendChild(
                elCreateText('a', {"href": "#", "title": tn('Edit'), "data-action": "edit", "class": ["mi", "color-darkgrey", "me-2"]}, 'edit')
            );
        }
        tr.appendChild(partitionActionTd);
        partitionList.appendChild(tr);
    }
}

//eslint-disable-next-line no-unused-vars
function savePartitionSettings() {
    cleanupModalId('modalPartitionSettings');
    let formOK = true;
    const mpdStreamPortEl = document.getElementById('inputMpdStreamPort');
    const streamUriEl = document.getElementById('inputStreamUri');
    if (validateIntRange(mpdStreamPortEl, 0, 65535) === false) {
        formOK = false;
    }
    if (streamUriEl.value.length > 0 &&
        validateStream(streamUriEl) === false)
    {
        formOK = false;
    }
    if (formOK === true) {
        sendAPI('MYMPD_API_PARTITION_SAVE', {
            "color": document.getElementById('inputPartitionColor').value,
            "mpdStreamPort": Number(mpdStreamPortEl.value),
            "streamUri": streamUriEl.value
        }, savePartitionSettingsClose, true);
    }
}

function savePartitionSettingsClose(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        uiElements.modalPartitionSettings.hide();
        getSettings(true);
    }
}
