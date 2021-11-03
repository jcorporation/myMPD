"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

//eslint-disable-next-line no-unused-vars
function updateDB(uri, showModal, rescan) {
    const method = rescan === true ? "MYMPD_API_DATABASE_RESCAN" : "MYMPD_API_DATABASE_UPDATE";
    sendAPI(method, {"uri": uri}, function(obj) {
        if (obj.error !== undefined) {
            updateDBerror(true, obj.error.message);
        }
        else {
            updateDBstarted(showModal);
        }
    }, true);
}

function updateDBerror(showModal, message) {
    const msg = tn('Database update failed') + ': ' + tn(message);
    if (showModal === true) {
        document.getElementById('updateDBfinished').textContent = '';
        elShowId('updateDBfooter');
        const updateDBprogress = document.getElementById('updateDBprogress');
        updateDBprogress.classList.remove('updateDBprogressAnimate');
        updateDBprogress.style.width = '0';
        updateDBprogress.style.marginLeft = '0px';
        const errorUpdateDB = document.getElementById('errorUpdateDB');
        elShow(errorUpdateDB);
        errorUpdateDB.textContent = msg;
        uiElements.modalUpdateDB.show();
    }
    showNotification(msg, '', 'database', 'error');
}

function updateDBstarted(showModal) {
    if (showModal === true) {
        document.getElementById('updateDBfinished').textContent = '';
        elHideId('updateDBfooter');
        const updateDBprogress = document.getElementById('updateDBprogress');
        updateDBprogress.style.width = '20px';
        updateDBprogress.style.marginLeft = '-20px';
        const errorUpdateDB = document.getElementById('errorUpdateDB');
        elHide(errorUpdateDB);
        errorUpdateDB.textContent = '';
        uiElements.modalUpdateDB.show();
        updateDBprogress.classList.add('updateDBprogressAnimate');
    }
    showNotification(tn('Database update started'), '', 'database', 'info');
}

function updateDBfinished(idleEvent) {
    if (document.getElementById('modalUpdateDB').classList.contains('show')) {
        _updateDBfinished(idleEvent);
    }
    else {
        //on small databases the modal opens after the finish event
        setTimeout(function() {
            _updateDBfinished(idleEvent);
        }, 100);
    }
}

function _updateDBfinished(idleEvent) {
    //spinner in mounts modal
    const el = document.getElementById('spinnerUpdateProgress');
    if (el) {
        const parent = el.parentNode;
        el.remove();
        for (let i = 0, j = parent.children.length; i < j; i++) {
            elShow(parent.children[i]);
        }
    }

    //update database modal
    if (document.getElementById('modalUpdateDB').classList.contains('show')) {
        if (idleEvent === 'update_database') {
            document.getElementById('updateDBfinished').textContent = tn('Database successfully updated');
        }
        else if (idleEvent === 'update_finished') {
            document.getElementById('updateDBfinished').textContent = tn('Database update finished');
        }
        const updateDBprogress = document.getElementById('updateDBprogress');
        updateDBprogress.classList.remove('updateDBprogressAnimate');
        updateDBprogress.style.width = '100%';
        updateDBprogress.style.marginLeft = '0px';
        elShowId('updateDBfooter');
    }

    //general notification
    if (idleEvent === 'update_database') {
        showNotification(tn('Database successfully updated'), '', 'database', 'info');
    }
    else if (idleEvent === 'update_finished') {
        showNotification(tn('Database update finished'), '', 'database', 'info');
    }
}
