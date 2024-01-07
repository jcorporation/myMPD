"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalMounts_js */

/**
 * Initializes the mounts related elements
 * @returns {void}
 */
function initModalMounts() {
    elGetById('modalMountsList').addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (event.target.nodeName === 'A') {
            const action = event.target.getAttribute('data-action');
            const mountPoint = getData(event.target.parentNode.parentNode, 'point');
            if (action === 'unmount') {
                // @ts-ignore
                unmountMount(mountPoint, event.target);
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

    elGetById('modalMountsNeighborsBtn').parentNode.addEventListener('show.bs.dropdown', function () {
        if (features.featNeighbors === true) {
            sendAPI("MYMPD_API_MOUNT_NEIGHBOR_LIST", {}, parseNeighbors, true);
        }
        else {
            elReplaceChildId('modalMountsNeighborsList',
                elCreateTextTn('div', {"class": ["list-group-item", "nowrap"]}, 'Neighbors are disabled')
            );
        }
    }, false);

    elGetById('modalMountsNeighborsList').addEventListener('click', function (event) {
        event.preventDefault();
        const target = event.target.nodeName === 'A'
            ? event.target
            : event.target.parentNode;
        if (target.nodeName === 'A') {
            elGetById('modalMountsMountUrlInput').value = getData(target, 'value');
            uiElements.modalMountsNeighborsDropdown.hide();
        }
    }, false);

    elGetById('modalMounts').addEventListener('show.bs.modal', function () {
        showListMounts();
    });
}

/**
 * Unmounts a mount point
 * @param {string} mountPoint mount point
 * @param {Element} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function unmountMount(mountPoint, target) {
    cleanupModalId('modalMounts');
    btnWaiting(target, true);
    sendAPI("MYMPD_API_MOUNT_UNMOUNT", {
        "mountPoint": mountPoint
    }, mountMountCheckError, true);
}

/**
 * Mounts a mount
 * @param {Element} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function mountMount(target) {
    cleanupModalId('modalMounts');
    btnWaiting(target, true);
    const inputMountUrl = elGetById('modalMountsMountUrlInput');
    const inputMountPoint = elGetById('modalMountsMountPointInput');
    sendAPI("MYMPD_API_MOUNT_MOUNT", {
        "mountUrl": inputMountUrl.value,
        "mountPoint": inputMountPoint.value,
    }, mountMountCheckError, true);
}

/**
 * Response handler for MYMPD_API_MOUNT_MOUNT
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function mountMountCheckError(obj) {
    if (modalApply(obj) === true) {
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
    // @ts-ignore
    updateDB(uri, false, el);
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
    elGetById('modalMountsListTab').classList.remove('active');
    elGetById('modalMountsEditTab').classList.add('active');
    elHideId('modalMountsListFooter');
    elShowId('modalMountsEditFooter');
    elGetById('modalMountsMountUrlInput').value = uri;
    elGetById('modalMountsMountPointInput').value = storage;
    setFocusId('modalMountsMountPointInput');
}

/**
 * Shows the list mount tab from the mount modal
 * @returns {void}
 */
function showListMounts() {
    cleanupModalId('modalMounts');
    elGetById('modalMountsListTab').classList.add('active');
    elGetById('modalMountsEditTab').classList.remove('active');
    elShowId('modalMountsListFooter');
    elHideId('modalMountsEditFooter');
    sendAPI("MYMPD_API_MOUNT_LIST", {}, parseListMounts, true);
}

/**
 * Parses the MYMPD_API_MOUNT_LIST response
 * @param {object} obj jsonrpc response object
 * @returns {void}
 */
function parseListMounts(obj) {
    const tbody = document.querySelector('#modalMountsList');
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
                elCreateText('a', {"href": "#", "data-title-phrase": "Unmount", "data-action": "unmount", "class": ["mi", "color-darkgrey"]}, 'eject')
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
    const dropdownNeighbors = elGetById('modalMountsNeighborsList');
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
