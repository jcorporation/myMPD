"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/**
 * Initializes the mounts related elements
 */
function initMounts() {
    document.getElementById('listMountsList').addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (event.target.nodeName === 'TD') {
            if (getData(event.target.parentNode, 'point') === '') {
                return false;
            }
            showEditMount(getData(event.target.parentNode, 'url'), getData(event.target.parentNode, 'point'));
        }
        else if (event.target.nodeName === 'A') {
            const action = event.target.getAttribute('data-action');
            const mountPoint = getData(event.target.parentNode.parentNode, 'point');
            if (action === 'unmount') {
                unmountMount(mountPoint);
            }
            else if (action === 'update') {
                updateMount(event.target, mountPoint);
            }
        }
    }, false);

    document.getElementById('btnDropdownNeighbors').parentNode.addEventListener('show.bs.dropdown', function () {
        if (features.featNeighbors === true) {
            sendAPI("MYMPD_API_MOUNT_NEIGHBOR_LIST", {}, parseNeighbors, true);
        }
        else {
            const dropdownNeighbors = document.getElementById('dropdownNeighbors').firstElementChild;
            elReplaceChild(dropdownNeighbors,
                elCreateTextTn('div', {"class": ["list-group-item", "nowrap"]}, 'Neighbors are disabled')
            );
        }
    }, false);

    document.getElementById('dropdownNeighbors').children[0].addEventListener('click', function (event) {
        event.preventDefault();
        if (event.target.nodeName === 'A') {
            const ec = getData(event.target, 'value');
            const c = ec.match(/^(\w+:\/\/)(.+)$/);
            document.getElementById('selectMountUrlhandler').value = c[1];
            document.getElementById('inputMountUrl').value = c[2];
        }
    }, false);

    document.getElementById('modalMounts').addEventListener('shown.bs.modal', function () {
        showListMounts();
        getUrlhandlers();
    });
}

/**
 * Unmounts a mount point
 * @param {string} mountPoint 
 */
//eslint-disable-next-line no-unused-vars
function unmountMount(mountPoint) {
    sendAPI("MYMPD_API_MOUNT_UNMOUNT", {
        "mountPoint": mountPoint
    }, mountMountCheckError, true);
}

/**
 * Mounts a mount
 */
//eslint-disable-next-line no-unused-vars
function mountMount() {
    cleanupModalId('modalMounts');
    let formOK = true;
    const inputMountUrl = document.getElementById('inputMountUrl');
    const inputMountPoint = document.getElementById('inputMountPoint');
    if (!validateNotBlank(inputMountUrl)) {
        formOK = false;
    }
    if (!validateNotBlank(inputMountPoint)) {
        formOK = false;
    }
    if (formOK === true) {
        sendAPI("MYMPD_API_MOUNT_MOUNT", {
            "mountUrl": getSelectValueId('selectMountUrlhandler') + inputMountUrl.value,
            "mountPoint": inputMountPoint.value,
        }, mountMountCheckError, true);
    }
}

/**
 * Response handler for MYMPD_API_MOUNT_MOUNT
 * @param {object} obj 
 */
function mountMountCheckError(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        showListMounts();
    }
}

/**
 * Updates a mount point
 * @param {HTMLElement | EventTarget} el 
 * @param {string} uri 
 */
//eslint-disable-next-line no-unused-vars
function updateMount(el, uri) {
    //hide action items
    const parent = el.parentNode;
    for (let i = 0, j = parent.children.length; i < j; i++) {
        elHide(parent.children[i]);
    }
    //add spinner
    const spinner = elCreateEmpty('div', {"id": "spinnerUpdateProgress", "class": ["spinner-border", "spinner-border-sm"]});
    el.parentNode.insertBefore(spinner, el);
    //update
    updateDB(uri, false, false, false);
}

/**
 * Shows the edit mount tab from the mount modal
 * @param {string} uri 
 * @param {string} storage 
 */
//eslint-disable-next-line no-unused-vars
function showEditMount(uri, storage) {
    cleanupModalId('modalMounts');
    document.getElementById('listMounts').classList.remove('active');
    document.getElementById('editMount').classList.add('active');
    elHideId('listMountsFooter');
    elShowId('editMountFooter');

    const c = uri.match(/^(\w+:\/\/)(.+)$/);
    if (c !== null && c.length > 2) {
        document.getElementById('selectMountUrlhandler').value = c[1];
        document.getElementById('inputMountUrl').value = c[2];
        document.getElementById('inputMountPoint').value = storage;
    }
    else {
        document.getElementById('inputMountUrl').value = '';
        document.getElementById('inputMountPoint').value = '';
    }
    setFocusId('inputMountPoint');
}

/**
 * Shows the list mount tab from the mount modal
 */
function showListMounts() {
    cleanupModalId('modalMounts');
    document.getElementById('listMounts').classList.add('active');
    document.getElementById('editMount').classList.remove('active');
    elShowId('listMountsFooter');
    elHideId('editMountFooter');
    sendAPI("MYMPD_API_MOUNT_LIST", {}, parseListMounts, true);
}

/**
 * Parses the MYMPD_API_MOUNT_LIST response
 * @param {object} obj jsonrpc response object
 */
function parseListMounts(obj) {
    const tbody = document.querySelector('#listMountsList');
    elClear(tbody);

    if (checkResult(obj, tbody) === false) {
        return;
    }

    for (let i = 0; i < obj.result.returnedEntities; i++) {
        const td1 = elCreateEmpty('td', {});
        if (obj.result.data[i].mountPoint === '') {
            td1.appendChild(
                elCreateText('span', {"class": ["mi"]}, 'home')
            );
        }
        else {
            td1.textContent = obj.result.data[i].mountPoint;
        }
        const mountActionTd = elCreateEmpty('td', {"data-col": "Action"});
        if (obj.result.data[i].mountPoint !== '') {
            mountActionTd.appendChild(
                elCreateText('a', {"href": "#", "data-title-phrase": "Unmount", "data-action": "unmount", "class": ["mi", "color-darkgrey"]}, 'delete')
            );
            mountActionTd.appendChild(
                elCreateText('a', {"href": "#", "data-title-phrase": "Update", "data-action": "update", "class": ["mi", "color-darkgrey"]}, 'refresh')
            );
        }
        const row = elCreateNodes('tr', {}, [
            td1,
            elCreateText('td', {}, obj.result.data[i].mountUrl),
            mountActionTd

        ]);
        setData(row, 'url', obj.result.data[i].mountUrl);
        setData(row, 'point', obj.result.data[i].mountPoint);
        if (obj.result.data[i].mountPoint === '') {
            row.classList.add('not-clickable');
        }
        tbody.append(row);
    }
}

/**
 * Parses the MYMPD_API_MOUNT_NEIGHBOR_LIST response
 * @param {object} obj jsonrpc response object
 */
function parseNeighbors(obj) {
    const dropdownNeighbors = document.getElementById('dropdownNeighbors').children[0];
    elClear(dropdownNeighbors);

    if (obj.error) {
        dropdownNeighbors.appendChild(
            elCreateTextTn('div', {"class": ["list-group-item", "alert", "alert-danger"]}, obj.error.message, obj.error.data)
        );
        return;
    }
    if (obj.result.returnedEntities === 0) {
        dropdownNeighbors.appendChild(
            elCreateTextTn('div', {"class": ["list-group-item", "alert", "alert-secondary"]}, 'Empty list')
        );
        return;
    }

    for (let i = 0; i < obj.result.returnedEntities; i++) {
        const a = elCreateNodes('a', {"href": "#", "class": ["list-group-item", "list-group-item-action"]}, [
            elCreateText('span', {}, obj.result.data[i].uri),
            elCreateEmpty('br', {}),
            elCreateText('small', {}, obj.result.data[i].displayName)
        ]);
        setData(a, 'value', obj.result.data[i].uri);
        dropdownNeighbors.appendChild(a);
    }
}

/**
 * Populates the urlhandler select in the mount modal
 */
function getUrlhandlers() {
    sendAPI("MYMPD_API_MOUNT_URLHANDLER_LIST", {}, function(obj) {
        const selectMountUrlhandler = document.getElementById('selectMountUrlhandler');
        elClear(selectMountUrlhandler);
        for (let i = 0; i < obj.result.returnedEntities; i++) {
            //smb is disabled because it is default disabled in mpd because of libmpdclient bug
            switch(obj.result.data[i]) {
                case 'http://':
                case 'https://':
                case 'nfs://':
                    selectMountUrlhandler.appendChild(
                        elCreateText('option', {"value": obj.result.data[i]}, obj.result.data[i])
                    );
                    break;
            }
        }
    }, false);
}
