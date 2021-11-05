"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function initOutputs() {
    //do not hide volume menu on click on volume change buttons
    for (const elName of ['btnChVolumeDown', 'btnChVolumeUp', 'volumeBar']) {
        document.getElementById(elName).addEventListener('click', function(event) {
            event.stopPropagation();
        }, false);
    }

    document.getElementById('volumeMenu').parentNode.addEventListener('show.bs.dropdown', function () {
        sendAPI("MYMPD_API_PLAYER_OUTPUT_LIST", {
            "partition": ""
        }, parseOutputs, true);
    });

    document.getElementById('outputs').addEventListener('click', function(event) {
        if (event.target.nodeName === 'A') {
            event.preventDefault();
            showListOutputAttributes(getData(event.target.parentNode, 'data-output-name'));
        }
        else {
            const target = event.target.nodeName === 'BUTTON' ? event.target : event.target.parentNode;
            event.stopPropagation();
            event.preventDefault();
            sendAPI("MYMPD_API_PLAYER_OUTPUT_TOGGLE", {
                "outputId": Number(getData(target, 'data-output-id')),
                "state": (target.classList.contains('active') ? 0 : 1)
            });
            toggleBtn(target);
        }
    }, false);
}

function parseOutputs(obj) {
    const outputList = document.getElementById('outputs');
    elClear(outputList);
    if (obj.error) {
        const div = elCreateEmpty('div', {"class": ["list-group-item"]});
        addIconLine(div, 'error_outline', tn(obj.error.message));
        outputList.appendChild(div);
        return;
    }
    if (obj.result.numOutputs === 0) {
        const div = elCreateEmpty('div', {"class": ["list-group-item"]});
        addIconLine(div, 'info', tn('Empty list'));
        outputList.appendChild(div);
        return;
    }

    for (let i = 0; i < obj.result.numOutputs; i++) {
        if (obj.result.data[i].plugin === 'dummy') {
            continue;
        }
        const btn = elCreateEmpty('button', {"class": ["btn", "btn-secondary", "d-flex", "justify-content-between"], "id": "btnOutput" + obj.result.data[i].id});
        setData(btn, 'data-output-name', obj.result.data[i].name);
        setData(btn, 'data-output-id', obj.result.data[i].id);
        if (obj.result.data[i].state === 1) {
            btn.classList.add('active');
        }
        btn.appendChild(elCreateText('span', {"class": ["mi", "align-self-center"]}, (obj.result.data[i].plugin === 'httpd' ? 'cast' : 'volume_up')));
        btn.appendChild(elCreateText('span', {"class": ["mx-2", "align-self-center"]}, obj.result.data[i].name));
        btn.appendChild(elCreateText('a', {"class": ["mi", "text-light", "align-self-center"],
            "title": (Object.keys(obj.result.data[i].attributes).length > 0 ? tn('Edit attributes') : tn('Show attributes'))}, 'settings'));
        outputList.appendChild(btn);
    }
}

function showListOutputAttributes(outputName) {
    sendAPI("MYMPD_API_PLAYER_OUTPUT_LIST", {
        "partition": ""
    }, function(obj) {
        uiElements.modalOutputAttributes.show();
        let output;
        for (let i = 0; i < obj.result.numOutputs; i++) {
            if (obj.result.data[i].name === outputName) {
                output = obj.result.data[i];
                break;
            }
        }
        document.getElementById('modalOutputAttributesId').value = output.id;
        const list = document.getElementById('outputAttributesList');
        elClear(list);
        for (const n of ['name', 'state', 'plugin']) {
            list.appendChild(
                elCreateNodes('tr', {},
                    [
                        elCreateText('td', {}, tn(ucFirst(n))),
                        elCreateText('td', {}, output[n])
                    ]
                )
            );
        }
        let i = 0;
        for (const key in output.attributes) {
            i++;
            list.appendChild(
                elCreateNodes('tr', {},
                    [
                        elCreateText('td', {}, key),
                        elCreateNode('td', {},
                            elCreateEmpty('input', {"name": key, "class": ["form-control"], "type": "text", "value": output.attributes[key]})
                        )
                    ]
                )
            );
        }
        if (i > 0) {
            elEnableId('btnOutputAttributesSave');
        }
        else {
            elDisableId('btnOutputAttributesSave');
        }
    });
}

//eslint-disable-next-line no-unused-vars
function saveOutputAttributes() {
    const params = {};
    params.outputId =  Number(document.getElementById('modalOutputAttributesId').value);
    params.attributes = {};
    const els = document.getElementById('outputAttributesList').getElementsByTagName('input');
    for (let i = 0, j = els.length; i < j; i++) {
        params.attributes[els[i].name] = els[i].value;
    }
    sendAPI('MYMPD_API_PLAYER_OUTPUT_ATTRIBUTS_SET', params, saveOutputAttributesClose, true);
}

function saveOutputAttributesClose(obj) {
    removeEnterPinFooter();
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        hideModalAlert();
        uiElements.modalOutputAttributes.hide();
    }
}

function parseVolume(obj) {
    if (obj.result.volume === -1) {
        document.getElementById('volumePrct').textContent = tn('Volumecontrol disabled');
        elHide(document.getElementById('volumeControl'));
    } 
    else {
        elShow(document.getElementById('volumeControl'));
        document.getElementById('volumePrct').textContent = obj.result.volume + ' %';
        document.getElementById('volumeMenu').firstChild.textContent =
            obj.result.volume === 0 ? 'volume_off' :
                obj.result.volume < 50 ? 'volume_down' : 'volume_up';
    }
    document.getElementById('volumeBar').value = obj.result.volume;
}

//eslint-disable-next-line no-unused-vars
function volumeStep(dir) {
    chVolume(dir === 'up' ? settings.volumeStep : 0 - settings.volumeStep);
}

function chVolume(increment) {
    const volumeBar = document.getElementById('volumeBar');
    let newValue = Number(volumeBar.value) + increment;
    if (newValue < settings.volumeMin)  {
        newValue = settings.volumeMin;
    }
    else if (newValue > settings.volumeMax) {
        newValue = settings.volumeMax;
    }
    volumeBar.value = newValue;
    sendAPI("MYMPD_API_PLAYER_VOLUME_SET", {
        "volume": newValue
    });
}
