"use strict";
// SPDX-License-Identifier: GPL-2.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function initMounts() {
    document.getElementById('listMountsList').addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (event.target.nodeName === 'TD') {
            if (getAttDec(event.target.parentNode, 'data-point') === '') {
                return false;
            }
            showEditMount(getAttDec(event.target.parentNode, 'data-url'), getAttDec(event.target.parentNode, 'data-point'));
        }
        else if (event.target.nodeName === 'A') {
            let action = event.target.getAttribute('data-action');
            let mountPoint = getAttDec(event.target.parentNode.parentNode, 'data-point');
            if (action === 'unmount') {
                unmountMount(mountPoint);
            }
            else if (action === 'update') {
                updateMount(event.target, mountPoint);
            }
        }
    }, false);

    document.getElementById('btnDropdownNeighbors').parentNode.addEventListener('show.bs.dropdown', function () {
        if (settings.featNeighbors === true) {
            sendAPI("MPD_API_MOUNT_NEIGHBOR_LIST", {}, parseNeighbors, true);
        }
        else {
            document.getElementById('dropdownNeighbors').children[0].innerHTML = 
                '<div class="list-group-item"><span class="mi">warning</span> ' + t('Neighbors are disabled') + '</div>';
        }
    }, false);
    
    document.getElementById('dropdownNeighbors').children[0].addEventListener('click', function (event) {
        event.preventDefault();
        if (event.target.nodeName === 'A') {
            const ec = getAttDec(event.target, 'data-value');
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

//eslint-disable-next-line no-unused-vars
function unmountMount(mountPoint) {
    sendAPI("MPD_API_MOUNT_UNMOUNT", {"mountPoint": mountPoint}, showListMounts);
}

//eslint-disable-next-line no-unused-vars
function mountMount() {
    let formOK = true;
    document.getElementById('errorMount').classList.add('hide');
    
    if (formOK === true) {
        sendAPI("MPD_API_MOUNT_MOUNT", {
            "mountUrl": getSelectValue('selectMountUrlhandler') + document.getElementById('inputMountUrl').value,
            "mountPoint": document.getElementById('inputMountPoint').value,
            }, showListMounts, true);
    }
}

//eslint-disable-next-line no-unused-vars
function updateMount(el, uri) {
    let parent = el.parentNode;
    for (let i = 0; i < parent.children.length; i++) {
        parent.children[i].classList.add('hide');
    }
    let spinner = document.createElement('div');
    spinner.setAttribute('id', 'spinnerUpdateProgress');
    spinner.classList.add('spinner-border', 'spinner-border-sm');
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

    let c = uri.match(/^(\w+:\/\/)(.+)$/);
    if (c !== null && c.length > 2) {
        document.getElementById('selectMountUrlhandler').value = c[1];
        document.getElementById('inputMountUrl').value = c[2];
        document.getElementById('inputMountPoint').value = storage;
    }
    else {
        document.getElementById('inputMountUrl').value = '';
        document.getElementById('inputMountPoint').value = '';
    }
    document.getElementById('inputMountUrl').focus();
    removeIsInvalid(document.getElementById('modalMounts'));
}

function showListMounts(obj) {
    if (obj && obj.error && obj.error.message) {
        let emEl = document.getElementById('errorMount');
        emEl.innerText = obj.error.message;
        emEl.classList.remove('hide');
        return;
    }
    document.getElementById('listMounts').classList.add('active');
    document.getElementById('editMount').classList.remove('active');
    document.getElementById('listMountsFooter').classList.remove('hide');
    document.getElementById('editMountFooter').classList.add('hide');
    sendAPI("MPD_API_MOUNT_LIST", {}, parseListMounts);
}

function parseListMounts(obj) {
    let tbody = document.getElementById('listMounts').getElementsByTagName('tbody')[0];
    let tr = tbody.getElementsByTagName('tr');
    
    let activeRow = 0;
    for (let i = 0; i < obj.result.returnedEntities; i++) {
        let row = document.createElement('tr');
        setAttEnc(row, 'data-url', obj.result.data[i].mountUrl);
        setAttEnc(row, 'data-point', obj.result.data[i].mountPoint);
        if (obj.result.data[i].mountPoint === '') {
            row.classList.add('not-clickable');
        }
        let tds = '<td>' + (obj.result.data[i].mountPoint === '' ? '<span class="mi">home</span>' : e(obj.result.data[i].mountPoint)) + '</td>' +
                  '<td>' + e(obj.result.data[i].mountUrl) + '</td>';
        if (obj.result.data[i].mountPoint !== '') {
            tds += '<td data-col="Action">' + 
                   '<a href="#" title="' + t('Unmount') + '" data-action="unmount" class="mi color-darkgrey">delete</a>' +
                   '<a href="#" title="' + t('Update') + '" data-action="update"class="mi color-darkgrey">refresh</a>' +
                   '</td>';
        }
        else {
            tds += '<td>&nbsp;</td>';
        }
        row.innerHTML = tds;
        if (i < tr.length) {
            activeRow = replaceTblRow(tr[i], row) === true ? i : activeRow;
        }
        else {
            tbody.append(row);
        }
    }
    let trLen = tr.length - 1;
    for (let i = trLen; i >= obj.result.returnedEntities; i --) {
        tr[i].remove();
    }

    if (obj.result.returnedEntities === 0) {
        tbody.innerHTML = '<tr><td><span class="mi">error_outline</span></td>' +
                          '<td colspan="4">' + t('Empty list') + '</td></tr>';
    }     
}

function parseNeighbors(obj) {
    let list = '';
    if (obj.error) {
        list = '<div class="list-group-item"><span class="mi">error_outline</span> ' + t(obj.error.message) + '</div>';
    }
    else {
        for (let i = 0; i < obj.result.returnedEntities; i++) {
            list += '<a href="#" class="list-group-item list-group-item-action" data-value="' + obj.result.data[i].uri + '">' + 
                    obj.result.data[i].uri + '<br/><small>' + obj.result.data[i].displayName + '</small></a>';
        }    
        if (obj.result.returnedEntities === 0) {
            list = '<div class="list-group-item"><span class="mi">error_outline</span>&nbsp;' + t('Empty list') + '</div>';
        }
    }
    document.getElementById('dropdownNeighbors').children[0].innerHTML = list;
}

function getUrlhandlers() {
    sendAPI("MPD_API_URLHANDLERS", {}, function(obj) {
        let storagePlugins = '';
        for (let i = 0; i < obj.result.data.length; i++) {
            switch(obj.result.data[i]) {
                case 'http://':
                case 'https://':
                case 'nfs://':
                case 'smb://':
                    storagePlugins += '<option value="' + obj.result.data[i] + '">' + obj.result.data[i] + '</option>';
                    break;
            }
        }
        document.getElementById('selectMountUrlhandler').innerHTML = storagePlugins;
    }, false);
}
