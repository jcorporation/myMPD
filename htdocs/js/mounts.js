"use strict";
// SPDX-License-Identifier: GPL-2.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function initMounts() {
    document.getElementById('listMountsList').addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (event.target.nodeName === 'TD') {
            if (getCustomDomProperty(event.target.parentNode, 'data-point') === '') {
                return false;
            }
            showEditMount(getCustomDomProperty(event.target.parentNode, 'data-url'), getCustomDomProperty(event.target.parentNode, 'data-point'));
        }
        else if (event.target.nodeName === 'A') {
            const action = event.target.getAttribute('data-action');
            const mountPoint = getCustomDomProperty(event.target.parentNode.parentNode, 'data-point');
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
            const dropdownNeighbors = document.getElementById('dropdownNeighbors').children[0];
            elClear(dropdownNeighbors);
            const div = elCreate('div', {"class": ["list-group-item", "nowrap"]}, '');
            addIconLine(div, 'warning', tn('Neighbors are disabled'));
            dropdownNeighbors.appendChild(div);
        }
    }, false);
    
    document.getElementById('dropdownNeighbors').children[0].addEventListener('click', function (event) {
        event.preventDefault();
        if (event.target.nodeName === 'A') {
            const ec = getCustomDomProperty(event.target, 'data-value');
            const c = ec.match(/^(\w+:\/\/)(.+)$/);
            document.getElementById('selectMountUrlhandler').value = c[1];
            document.getElementById('inputMountUrl').value = c[2];
        }
    }, false);

    document.getElementById('modalMounts').addEventListener('shown.bs.modal', function () {
        showListMounts();
        getUrlhandlers();
        removeIsInvalid(document.getElementById('modalMounts'));
    });
}

//eslint-disable-next-line no-unused-vars
function unmountMount(mountPoint) {
    sendAPI("MYMPD_API_MOUNT_UNMOUNT", {"mountPoint": mountPoint}, showListMounts);
}

//eslint-disable-next-line no-unused-vars
function mountMount() {
    document.getElementById('errorMount').classList.add('hide');
    removeIsInvalid(document.getElementById('modalMounts'));
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
            "mountUrl": getSelectValue('selectMountUrlhandler') + inputMountUrl.value,
            "mountPoint": inputMountPoint.value,
            }, showListMounts, true);
    }
}

//eslint-disable-next-line no-unused-vars
function updateMount(el, uri) {
    const parent = el.parentNode;
    for (let i = 0, j = parent.children.length; i < j; i++) {
        parent.children[i].classList.add('hide');
    }
    const spinner = elCreate('div', {"id": "spinnerUpdateProgress", "class": ["spinner-border", "spinner-border-sm"]}, '');
    el.parentNode.insertBefore(spinner, el);
    updateDB(uri, false);    
}

//eslint-disable-next-line no-unused-vars
function showEditMount(uri, storage) {
    document.getElementById('listMounts').classList.remove('active');
    document.getElementById('editMount').classList.add('active');
    document.getElementById('listMountsFooter').classList.add('hide');
    document.getElementById('editMountFooter').classList.remove('hide');
    document.getElementById('errorMount').classList.add('hide');

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
    document.getElementById('inputMountPoint').focus();
    removeIsInvalid(document.getElementById('modalMounts'));
}

function showListMounts(obj) {
    if (obj && obj.error && obj.error.message) {
        const emEl = document.getElementById('errorMount');
        elClear(emEl);
        addIconLine(emEl, 'error_outline', tn(obj.error.message));
        emEl.classList.remove('hide');
        return;
    }
    document.getElementById('listMounts').classList.add('active');
    document.getElementById('editMount').classList.remove('active');
    document.getElementById('listMountsFooter').classList.remove('hide');
    document.getElementById('editMountFooter').classList.add('hide');
    sendAPI("MYMPD_API_MOUNT_LIST", {}, parseListMounts);
}

function parseListMounts(obj) {
    const tbody = document.getElementById('listMounts').getElementsByTagName('tbody')[0];
    const tr = tbody.getElementsByTagName('tr');

    if (obj.result.returnedEntities === 0) {
        elClear(tbody);
        tbody.appendChild(emptyRow(5));
        return;
    }

    let activeRow = 0;
    for (let i = 0; i < obj.result.returnedEntities; i++) {
        const row = document.createElement('tr');
        setCustomDomProperty(row, 'data-url', obj.result.data[i].mountUrl);
        setCustomDomProperty(row, 'data-point', obj.result.data[i].mountPoint);
        if (obj.result.data[i].mountPoint === '') {
            row.classList.add('not-clickable');
        }
        const td1 = elCreate('td', {}, '');
        if (obj.result.data[i].mountPoint === '') {
            td1.appendChild(elCreate('span', {"class": ["mi"]}, 'home'));
        }
        else {
            td1.textContent = obj.result.data[i].mountPoint;
        }
        row.appendChild(td1);
        row.appendChild(elCreate('td', {}, obj.result.data[i].mountUrl));
        const actionTd = elCreate('td', {"data-col": "Action"}, '');
        
        if (obj.result.data[i].mountPoint !== '') {
            const a1 = elCreate('a', {"href": "#", "title": tn('Unmount'), "data-action": "unmount", "class": ["mi", "color-darkgrey"]}, 'delete');
            const a2 = elCreate('a', {"href": "#", "title": tn('Update'), "data-action": "update", "class": ["mi", "color-darkgrey"]}, 'refresh');
            actionTd.appendChild(a1);
            actionTd.appendChild(a2);
        }
        row.appendChild(actionTd);
        if (i < tr.length) {
            activeRow = replaceTblRow(tr[i], row) === true ? i : activeRow;
        }
        else {
            tbody.append(row);
        }
    }
    for (let i = tr.length - 1; i >= obj.result.returnedEntities; i--) {
        tr[i].remove();
    }
}

function parseNeighbors(obj) {
    const dropdownNeighbors = document.getElementById('dropdownNeighbors').children[0];
    elClear(dropdownNeighbors);

    if (obj.error) {
        const div = elCreate('div', {"class": ["list-group-item"]}, '');
        addIconLine(div, 'error_outline', tn(obj.error.message));
        dropdownNeighbors.appendChild(div);
        return;
    }
    if (obj.result.returnedEntities === 0) {
        const div = elCreate('div', {"class": ["list-group-item"]}, '');
        addIconLine(div, 'info', tn('Empty list'));
        dropdownNeighbors.appendChild(div);
        return;
    }

    for (let i = 0; i < obj.result.returnedEntities; i++) {
        const a = elCreate('a', {"href": "#", "class": ["list-group-item", "list-group-item-action"]}, '');
        setCustomDomProperty(a, 'data-value', obj.result.data[i].uri);
        const span = elCreate('span', {}, obj.result.data[i].uri);
        const br = elCreate('br', {}, '');
        const small = elCreate('small', {}, obj.result.data[i].displayName);
        a.appendChild(span);
        a.appendChild(br);
        a.appendChild(small);
        dropdownNeighbors.appendChild(a);
    }    
}

function getUrlhandlers() {
    sendAPI("MYMPD_API_URLHANDLERS", {}, function(obj) {
        const selectMountUrlhandler = document.getElementById('selectMountUrlhandler');
        elClear(selectMountUrlhandler);
        for (let i = 0; i < obj.result.returnedEntities; i++) {
            //smb is disabled because it is default disabled in mpd because of libmpdclient bug
            switch(obj.result.data[i]) {
                case 'http://':
                case 'https://':
                case 'nfs://':
                    selectMountUrlhandler.appendChild(elCreate('option', {"value": obj.result.data[i]}, obj.result.data[i]));
                    break;
            }
        }
    }, false);
}
