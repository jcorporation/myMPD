"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalMaintenance_js */

/**
 * Initializes the maintenance elements
 * @returns {void}
 */
function initModalMaintenance() {
    elGetById('modalMaintenance').addEventListener('show.bs.modal', function () {
        elGetById('modalMaintenanceLoglevelInput').value = settings.loglevel;
        cleanupModalId('modalMaintenance');
    });
}

/**
 * Sets the myMPD loglevel
 * @param {Node} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function setLoglevel(target) {
    if (target) {
        btnWaiting(target, true);
    }
    const loglevel = Number(getSelectValueId('modalMaintenanceLoglevelInput'));
    sendAPI("MYMPD_API_LOGLEVEL", {
        "loglevel": loglevel
    }, function() {
        settings.loglevel = loglevel;
        if (target) {
            btnWaiting(target, false);
        }
    }, true);
}

/**
 * Clears the covercache
 * @param {Node} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function clearDiskcache(target) {
    if (target) {
        btnWaiting(target, true);
    }
    sendAPI("MYMPD_API_CACHE_DISK_CLEAR", {}, function() {
        if (target) {
            btnWaiting(target, false);
        }
    }, true);
}

/**
 * Crops the covercache
 * @param {Node} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function cropDiskcache(target) {
    if (target) {
        btnWaiting(target, true);
    }
    sendAPI("MYMPD_API_CACHE_DISK_CROP", {}, function() {
        if (target) {
            btnWaiting(target, false);
        }
    }, true);
}

/**
 * Removes all playlists by condition
 * @param {Node} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function deletePlaylists(target) {
    if (target) {
        btnWaiting(target, true);
    }
    sendAPI("MYMPD_API_PLAYLIST_RM_ALL", {
        "plistType": getSelectValueId('modalMaintenancePlistTypeInput')
    }, function() {
        if (target) {
            btnWaiting(target, false);
        }
    }, true);
}

/**
 * Validates all playlists
 * @param {boolean} remove remove invalid elements?
 * @param {Node} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function validatePlaylists(remove, target) {
    if (target) {
        btnWaiting(target, true);
    }
    sendAPI("MYMPD_API_PLAYLIST_CONTENT_VALIDATE_ALL", {
        "remove": remove
    }, function() {
        if (target) {
            btnWaiting(target, false);
        }
    }, true);
}

/**
 * Deduplicates all playlists
 * @param {boolean} remove remove duplicate elements?
 * @param {Node} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function dedupPlaylists(remove, target) {
    if (target) {
        btnWaiting(target, true);
    }
    sendAPI("MYMPD_API_PLAYLIST_CONTENT_DEDUP_ALL", {
        "remove": remove
    }, function() {
        if (target) {
            btnWaiting(target, false);
        }
    }, true);
}

/**
 * Updates all smart playlists
 * @param {boolean} force true = forces update of all smart playlists,
 *                        false = updates only outdated smart playlists
 * @param {Node} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function updateSmartPlaylists(force, target) {
    if (target) {
        btnWaiting(target, true);
    }
    sendAPI("MYMPD_API_SMARTPLS_UPDATE_ALL", {
        "force": force
    }, function() {
        if (target) {
            btnWaiting(target, false);
        }
    }, true);
}

/**
 * Updates the myMPD caches
 * @param {boolean} force true=forces an update
 * @param {Node} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function updateCaches(force, target) {
    if (target) {
        btnWaiting(target, true);
    }
    sendAPI("MYMPD_API_CACHES_CREATE", {
        "force": force
    }, function() {
        if (target) {
            btnWaiting(target, false);
        }
    }, true);
}

/**
 * Updates or rescans the database
 * @param {string} uri baseuri
 * @param {boolean} rescan true = rescan, false = update
 * @param {Node} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function updateDB(uri, rescan, target) {
    if (target) {
        btnWaiting(target, true);
    }
    const method = rescan === true
        ? "MYMPD_API_DATABASE_RESCAN"
        : "MYMPD_API_DATABASE_UPDATE";
    sendAPI(method, {
        "uri": uri
    }, function() {
        if (target) {
            btnWaiting(target, false);
        }
    }, true); 
}

/**
 * Update database finished handler
 * @param {string} idleEvent mpd idle event
 * @returns {void}
 */
function updateDBfinished(idleEvent) {
    const text = idleEvent === 'update_database'
        ? tn('Database successfully updated')
        : tn('Database update finished');
    showNotification(text, 'database', 'info');
}
