"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module settingsConnection_js */

/**
 * Initialization function for the connection settings elements
 * @returns {void}
 */
function initSettingsConnection() {
    initElements(document.getElementById('modalConnection'));

    document.getElementById('selectMusicDirectory').addEventListener('change', function () {
        const musicDirMode = getSelectValue(this);
        if (musicDirMode === 'auto') {
            document.getElementById('inputMusicDirectory').value = settings.musicDirectoryValue;
            document.getElementById('inputMusicDirectory').setAttribute('readonly', 'readonly');
        }
        else if (musicDirMode === 'none') {
            document.getElementById('inputMusicDirectory').value = '';
            document.getElementById('inputMusicDirectory').setAttribute('readonly', 'readonly');
        }
        else {
            document.getElementById('inputMusicDirectory').value = '';
            document.getElementById('inputMusicDirectory').removeAttribute('readonly');
        }
    }, false);

    document.getElementById('selectPlaylistDirectory').addEventListener('change', function () {
        const playlistDirMode = getSelectValue(this);
        if (playlistDirMode === 'auto') {
            document.getElementById('inputPlaylistDirectory').value = settings.playlistDirectoryValue;
            document.getElementById('inputPlaylistDirectory').setAttribute('readonly', 'readonly');
        }
        else if (playlistDirMode === 'none') {
            document.getElementById('inputPlaylistDirectory').value = '';
            document.getElementById('inputPlaylistDirectory').setAttribute('readonly', 'readonly');
        }
        else {
            document.getElementById('inputPlaylistDirectory').value = '';
            document.getElementById('inputPlaylistDirectory').removeAttribute('readonly');
        }
    }, false);

    document.getElementById('modalConnection').addEventListener('shown.bs.modal', function () {
        getSettings();
        cleanupModalId('modalConnection');
    });
}

/**
 * Saves the mpd connection settings
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function saveConnection() {
    cleanupModalId('modalConnection');
    let formOK = true;
    const mpdHostEl = document.getElementById('inputMpdHost');
    const mpdPortEl = document.getElementById('inputMpdPort');
    const mpdPassEl = document.getElementById('inputMpdPass');
    const mpdBinarylimitEl = document.getElementById('inputMpdBinarylimit');
    const mpdTimeoutEl = document.getElementById('inputMpdTimeout');

    const musicDirectoryEl = document.getElementById('selectMusicDirectory');
    let musicDirectory = getSelectValue(musicDirectoryEl);
    if (musicDirectory === 'auto' &&
        mpdHostEl.value.indexOf('/') !== 0)
    {
        formOK = false;
        setIsInvalid(musicDirectoryEl);
    }
    else if (musicDirectory === 'custom') {
        const musicDirectoryValueEl = document.getElementById('inputMusicDirectory');
        if (validatePathEl(musicDirectoryValueEl) === false) {
            formOK = false;
        }
        musicDirectory = musicDirectoryValueEl.value;
    }

    const playlistDirectoryEl = document.getElementById('selectPlaylistDirectory');
    let playlistDirectory = getSelectValue(playlistDirectoryEl);
    if (playlistDirectory === 'auto' &&
        mpdHostEl.value.indexOf('/') !== 0)
    {
        formOK = false;
        setIsInvalid(playlistDirectoryEl);
    }
    else if (playlistDirectory === 'custom') {
        const playlistDirectoryValueEl = document.getElementById('inputPlaylistDirectory');
        if (validatePathEl(playlistDirectoryValueEl) === false) {
            formOK = false;
        }
        playlistDirectory = playlistDirectoryValueEl.value;
    }

    if (mpdPortEl.value === '') {
        mpdPortEl.value = '6600';
    }
    if (validateIntRangeEl(mpdPortEl, 1024, 65535) === false) {
        formOK = false;
    }
    if (mpdHostEl.value.indexOf('/') !== 0) {
        if (validateIntEl(mpdPortEl) === false) {
            formOK = false;
        }
        if (validateHostEl(mpdHostEl) === false) {
            formOK = false;
        }
    }
    if (validateIntRangeEl(mpdBinarylimitEl, 4, 256) === false) {
        formOK = false;
    }
    if (validateIntRangeEl(mpdTimeoutEl, 1, 1000) === false) {
        formOK = false;
    }
    if (formOK === true) {
        sendAPIpartition('default', 'MYMPD_API_CONNECTION_SAVE', {
            "mpdHost": mpdHostEl.value,
            "mpdPort": Number(mpdPortEl.value),
            "mpdPass": mpdPassEl.value,
            "musicDirectory": musicDirectory,
            "playlistDirectory": playlistDirectory,
            "mpdBinarylimit": Number(mpdBinarylimitEl.value) * 1024,
            "mpdTimeout": Number(mpdTimeoutEl.value) * 1000,
            "mpdKeepalive": getBtnChkValueId('btnMpdKeepalive')
        }, saveConnectionClose, true);
    }
}

/**
 * Handler for the MYMPD_API_CONNECTION_SAVE jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function saveConnectionClose(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        getSettings();
        uiElements.modalConnection.hide();
    }
}

/**
 * Populates the connection modal
 * @returns {void}
 */
function populateConnectionFrm() {
    document.getElementById('inputMpdHost').value = settings.mpdHost;
    document.getElementById('inputMpdPort').value = settings.mpdPort;
    document.getElementById('inputMpdPass').value = settings.mpdPass;
    document.getElementById('inputMpdBinarylimit').value = settings.mpdBinarylimit / 1024;
    document.getElementById('inputMpdTimeout').value = settings.mpdTimeout / 1000;
    toggleBtnChkId('btnMpdKeepalive', settings.mpdKeepalive);

    if (settings.musicDirectory === 'auto') {
        document.getElementById('selectMusicDirectory').value = settings.musicDirectory;
        document.getElementById('inputMusicDirectory').value = settings.musicDirectoryValue !== undefined ? settings.musicDirectoryValue : '';
        document.getElementById('inputMusicDirectory').setAttribute('readonly', 'readonly');
    }
    else if (settings.musicDirectory === 'none') {
        document.getElementById('selectMusicDirectory').value = settings.musicDirectory;
        document.getElementById('inputMusicDirectory').value = '';
        document.getElementById('inputMusicDirectory').setAttribute('readonly', 'readonly');
    }
    else {
        document.getElementById('selectMusicDirectory').value = 'custom';
        document.getElementById('inputMusicDirectory').value = settings.musicDirectoryValue;
        document.getElementById('inputMusicDirectory').removeAttribute('readonly');
    }

    if (settings.musicDirectoryValue === '' &&
        settings.musicDirectory !== 'none')
    {
        elShowId('warnMusicDirectory');
    }
    else {
        elHideId('warnMusicDirectory');
    }

    if (features.featPlaylistDirAuto === false &&
        settings.playlistDirectory === 'auto')
    {
        settings.playlistDirectory = 'none';
    }

    if (settings.playlistDirectory === 'auto') {
        document.getElementById('selectPlaylistDirectory').value = settings.playlistDirectory;
        document.getElementById('inputPlaylistDirectory').value = settings.playlistDirectoryValue !== undefined ? settings.playlistDirectoryValue : '';
        document.getElementById('inputPlaylistDirectory').setAttribute('readonly', 'readonly');
    }
    else if (settings.playlistDirectory === 'none') {
        document.getElementById('selectPlaylistDirectory').value = settings.playlistDirectory;
        document.getElementById('inputPlaylistDirectory').value = '';
        document.getElementById('inputPlaylistDirectory').setAttribute('readonly', 'readonly');
    }
    else {
        document.getElementById('selectPlaylistDirectory').value = 'custom';
        document.getElementById('inputPlaylistDirectory').value = settings.playlistDirectoryValue;
        document.getElementById('inputPlaylistDirectory').removeAttribute('readonly');
    }

    if (settings.playlistDirectoryValue === '' &&
        settings.playlistDirectory !== 'none')
    {
        elShowId('warnPlaylistDirectory');
    }
    else {
        elHideId('warnPlaylistDirectory');
    }
}
