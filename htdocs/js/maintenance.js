"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

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
