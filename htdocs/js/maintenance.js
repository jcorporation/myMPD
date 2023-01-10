"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module maintenance_js */

/**
 * Initializes the maintenance elements
 */
function initMaintenance() {
    document.getElementById('modalMaintenance').addEventListener('shown.bs.modal', function () {
        document.getElementById('selectSetLoglevel').value = settings.loglevel;
        cleanupModalId('modalMaintenance');
    });
}

/**
 * Sets the myMPD loglevel
 */
//eslint-disable-next-line no-unused-vars
function setLoglevel() {
    const loglevel = Number(getSelectValueId('selectSetLoglevel'));
    sendAPI("MYMPD_API_LOGLEVEL", {
        "loglevel": loglevel
    }, function() {
        settings.loglevel = loglevel
    }, false);
}

/**
 * Clears the covercache
 */
//eslint-disable-next-line no-unused-vars
function clearCovercache() {
    sendAPI("MYMPD_API_COVERCACHE_CLEAR", {}, null, false);
}

/**
 * Crops the covercache
 */
//eslint-disable-next-line no-unused-vars
function cropCovercache() {
    sendAPI("MYMPD_API_COVERCACHE_CROP", {}, null, false);
}

/**
 * Removes all playlists by condition
 */
//eslint-disable-next-line no-unused-vars
function deletePlaylists() {
    sendAPI("MYMPD_API_PLAYLIST_RM_ALL", {
        "type": getSelectValueId('selectDeletePlaylists')
    }, null, false);
}

/**
 * Updates the myMPD caches
 * @param {boolean} force true=forces an update
 */
//eslint-disable-next-line no-unused-vars
function updateCaches(force) {
    sendAPI("MYMPD_API_CACHES_CREATE", {
        "force": force
    }, null, false);
}

/**
 * Updates or rescans the database
 * @param {string} uri baseuri
 * @param {boolean} rescan true = rescan, false = update
 */
//eslint-disable-next-line no-unused-vars
function updateDB(uri, rescan) {
    const method = rescan === true ? "MYMPD_API_DATABASE_RESCAN" : "MYMPD_API_DATABASE_UPDATE";
    sendAPI(method, {"uri": uri}, null, false);
}

/**
 * Update database finished handler
 * @param {string} idleEvent mpd idle event
 */
function updateDBfinished(idleEvent) {
    //spinner in mounts modal
    const el = document.getElementById('spinnerUpdateProgress');
    if (el) {
        const parent = el.parentNode;
        el.remove();
        for (let i = 0, j = parent.children.length; i < j; i++) {
            elShow(parent.children[i]);
        }
    }

    const text = idleEvent === 'update_database' ?
        tn('Database successfully updated') : tn('Database update finished');
    showNotification(text, '', 'database', 'info');
}
