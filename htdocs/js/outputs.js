"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function initOutputs() {
    //do not hide volume menu on click on volume change buttons
    for (const elName of ['btnChVolumeDown', 'btnChVolumeUp', 'volumeBar']) {
        document.getElementById(elName).addEventListener('click', function(event) {
            event.stopPropagation();
        }, false);
    }

    domCache.volumeBar.addEventListener('change', function() {
        setVolume();
    }, false);

    document.getElementById('volumeMenu').parentNode.addEventListener('show.bs.dropdown', function () {
        sendAPI("MYMPD_API_PLAYER_OUTPUT_LIST", {}, parseOutputs, true);
    });

    document.getElementById('outputs').addEventListener('click', function(event) {
        if (event.target.nodeName === 'A') {
            event.preventDefault();
            BSN.Dropdown.getInstance(document.getElementById('volumeMenu')).toggle();
            showListOutputAttributes(getData(event.target.parentNode, 'output-name'));
        }
        else {
            const target = event.target.nodeName === 'BUTTON' ? event.target : event.target.parentNode;
            event.stopPropagation();
            event.preventDefault();
            sendAPI("MYMPD_API_PLAYER_OUTPUT_TOGGLE", {
                "outputId": Number(getData(target, 'output-id')),
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
        outputList.appendChild(
            elCreateText('div', {"class": ["list-group-item", "alert", "alert-danger"]}, tn(obj.error.message))
        );
        return;
    }
    if (obj.result.numOutputs === 0) {
        outputList.appendChild(
            elCreateText('div', {"class": ["list-group-item", "alert", "alert-secondary"]}, tn('No outputs found'))
        );
        return;
    }

    for (let i = 0; i < obj.result.numOutputs; i++) {
        if (obj.result.data[i].plugin === 'dummy') {
            continue;
        }
        const btn = elCreateNodes('button', {"class": ["btn", "btn-secondary", "d-flex", "justify-content-between"], "id": "btnOutput" + obj.result.data[i].id}, [
            elCreateText('span', {"class": ["mi", "align-self-center"]}, (obj.result.data[i].plugin === 'httpd' ? 'cast' : 'volume_up')),
            elCreateText('span', {"class": ["mx-2", "align-self-center"]}, obj.result.data[i].name),
            elCreateText('a', {"class": ["mi", "text-light", "align-self-center"],
                "title": (Object.keys(obj.result.data[i].attributes).length > 0 ? tn('Edit attributes') : tn('Show attributes'))}, 'settings')
        ]);
        setData(btn, 'output-name', obj.result.data[i].name);
        setData(btn, 'output-id', obj.result.data[i].id);
        if (obj.result.data[i].state === 1) {
            btn.classList.add('active');
        }
        outputList.appendChild(btn);
    }
    //prevent overflow of dropup
    const outputsEl = document.getElementById('outputs');
    const posY = getYpos(document.getElementById('outputsDropdown'));
    if (posY < 0) {
        outputsEl.style.maxHeight = (outputsEl.offsetHeight + posY) + 'px';
    }
    else {
        outputsEl.style.maxHeight = 'none';
    }
}

function showListOutputAttributes(outputName) {
    cleanupModalId('modalOutputAttributes');
    uiElements.modalOutputAttributes.show();
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
}

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
                elCreateText('td', {}, tn(n)),
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

//eslint-disable-next-line no-unused-vars
function saveOutputAttributes() {
    cleanupModalId('modalOutputAttributes');
    const params = {};
    params.outputId = Number(document.getElementById('modalOutputAttributesId').value);
    params.attributes = {};
    const els = document.getElementById('outputAttributesList').getElementsByTagName('input');
    for (let i = 0, j = els.length; i < j; i++) {
        params.attributes[els[i].name] = els[i].value;
    }
    sendAPI('MYMPD_API_PLAYER_OUTPUT_ATTRIBUTS_SET', params, saveOutputAttributesClose, true);
}

function saveOutputAttributesClose(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        uiElements.modalOutputAttributes.hide();
    }
}

function parseVolume(obj) {
    if (obj.result.volume === -1) {
        document.getElementById('volumePrct').textContent = tn('Volumecontrol disabled');
        elHideId('volumeControl');
    }
    else {
        elShowId('volumeControl');
        document.getElementById('volumePrct').textContent = obj.result.volume + ' %';
        document.getElementById('volumeMenu').firstChild.textContent =
            obj.result.volume === 0 ? 'volume_off' :
                obj.result.volume < 50 ? 'volume_down' : 'volume_up';
    }
    domCache.volumeBar.value = obj.result.volume;
}

//eslint-disable-next-line no-unused-vars
function volumeStep(dir) {
    const step = dir === 'up' ? settings.volumeStep : 0 - settings.volumeStep;
    const curValue = Number(domCache.volumeBar.value);
    let newValue = curValue + step;

    if (newValue < settings.volumeMin) {
        newValue = settings.volumeMin;
        domCache.volumeBar.value = newValue;
        setVolume();
    }
    else if (newValue > settings.volumeMax) {
        newValue = settings.volumeMax;
        domCache.volumeBar.value = newValue;
        setVolume();
    }
    else {
        sendAPI("MYMPD_API_PLAYER_VOLUME_CHANGE", {
            "volume": step
        });
        domCache.volumeBar.value = newValue;
    }
}

function setVolume() {
    sendAPI("MYMPD_API_PLAYER_VOLUME_SET", {
        "volume": Number(domCache.volumeBar.value)
    });
}
