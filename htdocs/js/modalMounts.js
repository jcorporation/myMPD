"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalMounts_js */

/**
 * Initializes the mounts related elements
 * @returns {void}
 */
function initMounts() {
    document.getElementById('listMountsList').addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (event.target.nodeName === 'A') {
            const action = event.target.getAttribute('data-action');
            const mountPoint = getData(event.target.parentNode.parentNode, 'point');
            if (action === 'unmount') {
                unmountMount(mountPoint);
            }
            else if (action === 'update') {
                updateMount(event.target, mountPoint);
            }
            return;
        }
        const target = event.target.closest('TR');
        if (checkTargetClick(target) === true) {
            showEditMount(getData(target, 'url'), getData(target, 'point'));
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
        const target = event.target.nodeName === 'A'
            ? event.target
            : event.target.parentNode;
        if (target.nodeName === 'A') {
            document.getElementById('inputMountUrl').value = getData(target, 'value');
            uiElements.dropdownNeighbors.hide();
        }
    }, false);

    document.getElementById('modalMounts').addEventListener('shown.bs.modal', function () {
        showListMounts();
    });
}

/**
 * Unmounts a mount point
 * @param {string} mountPoint mount point
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function unmountMount(mountPoint) {
    sendAPI("MYMPD_API_MOUNT_UNMOUNT", {
        "mountPoint": mountPoint
    }, mountMountCheckError, true);
}

/**
 * Mounts a mount
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function mountMount() {
    cleanupModalId('modalMounts');
    let formOK = true;
    const inputMountUrl = document.getElementById('inputMountUrl');
    const inputMountPoint = document.getElementById('inputMountPoint');
    if (!validateNotBlankEl(inputMountUrl)) {
        formOK = false;
    }
    if (!validateNotBlankEl(inputMountPoint)) {
        formOK = false;
    }
    if (formOK === true) {
        sendAPI("MYMPD_API_MOUNT_MOUNT", {
            "mountUrl": inputMountUrl.value,
            "mountPoint": inputMountPoint.value,
        }, mountMountCheckError, true);
    }
}

/**
 * Response handler for MYMPD_API_MOUNT_MOUNT
 * @param {object} obj jsonrpc response
 * @returns {void}
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
 * @param {HTMLElement | EventTarget} el event target
 * @param {string} uri mount point
 * @returns {void}
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
    updateDB(uri, false);
}

/**
 * Shows the edit mount tab from the mount modal
 * @param {string} uri mounted uri
 * @param {string} storage mount point
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showEditMount(uri, storage) {
    cleanupModalId('modalMounts');
    document.getElementById('listMounts').classList.remove('active');
    document.getElementById('editMount').classList.add('active');
    elHideId('listMountsFooter');
    elShowId('editMountFooter');
    document.getElementById('inputMountUrl').value = uri;
    document.getElementById('inputMountPoint').value = storage;
    setFocusId('inputMountPoint');
}

/**
 * Shows the list mount tab from the mount modal
 * @returns {void}
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
 * @returns {void}
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
        const row = elCreateNodes('tr', {"title": tn('Edit')}, [
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
 * @returns {void}
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
