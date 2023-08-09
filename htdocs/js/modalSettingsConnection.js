"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalSettingsConnection_js */

/**
 * Initialization function for the connection settings elements
 * @returns {void}
 */
function initModalSettingsConnection() {
    // cache for the form field containers
    const forms = {};
    // create the fields
    createForm(settingsConnectionFields, 'modalSettingsConnection', forms);
    initElements(document.getElementById('modalSettingsConnection'));

    document.getElementById('modalSettingsConnectionMusicDirectorySelect').addEventListener('change', function () {
        const musicDirMode = getSelectValue(this);
        const musicDirInput = document.getElementById('modalSettingsConnectionMusicDirectoryInput');
        if (musicDirMode === 'auto') {
            musicDirInput.value = settings.musicDirectoryValue;
            musicDirInput.setAttribute('readonly', 'readonly');
        }
        else if (musicDirMode === 'none') {
            musicDirInput.value = '';
            musicDirInput.setAttribute('readonly', 'readonly');
        }
        else {
            musicDirInput.value = '';
            musicDirInput.removeAttribute('readonly');
        }
    }, false);

    document.getElementById('modalSettingsConnectionPlaylistDirectorySelect').addEventListener('change', function () {
        const playlistDirMode = getSelectValue(this);
        const playlistDirInput = document.getElementById('modalSettingsConnectionPlaylistDirectoryInput');
        if (playlistDirMode === 'auto') {
            playlistDirInput.value = settings.playlistDirectoryValue;
            playlistDirInput.setAttribute('readonly', 'readonly');
        }
        else if (playlistDirMode === 'none') {
            playlistDirInput.value = '';
            playlistDirInput.setAttribute('readonly', 'readonly');
        }
        else {
            playlistDirInput.value = '';
            playlistDirInput.removeAttribute('readonly');
        }
    }, false);

    document.getElementById('modalSettingsConnection').addEventListener('shown.bs.modal', function () {
        cleanupModalId('modalSettingsConnection');
        getSettings();
    });
}

/**
 * Saves the mpd connection settings
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function saveConnection() {
    cleanupModalId('modalSettingsConnection');
    const settingsParams = {};

    const mpdLocal = document.getElementById('modalSettingsConnectionMpdHostInput').value.indexOf('/') === 0
        ? true
        : false;

    const musicDirectoryEl = document.getElementById('modalSettingsConnectionMusicDirectorySelect');
    const musicDirectory = getSelectValue(musicDirectoryEl);
    if (musicDirectory === 'auto' &&
        mpdLocal === false)
    {
        setIsInvalid(musicDirectoryEl);
        return;
    }

    const playlistDirectoryEl = document.getElementById('modalSettingsConnectionPlaylistDirectorySelect');
    const playlistDirectory = getSelectValue(playlistDirectoryEl);
    if (playlistDirectory === 'auto' &&
        mpdLocal === false)
    {
        setIsInvalid(playlistDirectoryEl);
        return;
    }

    if (formToJson('modalSettingsConnection', settingsParams, settingsConnectionFields) === true) {
        if (musicDirectory === 'custom') {
            settingsParams.musicDirectory = document.getElementById('modalSettingsConnectionMusicDirectoryInput').value;
        }
        else {
            settingsParams.musicDirectory = musicDirectory;
        }
        if (playlistDirectory === 'custom') {
            settingsParams.playlistDirectory = document.getElementById('modalSettingsConnectionPlaylistDirectoryInput').value;
        }
        else {
            settingsParams.playlistDirectory = playlistDirectory;
        }

        settingsParams.mpdBinarylimit = settingsParams.mpdBinarylimit * 1024;
        settingsParams.mpdTimeout = settingsParams.mpdTimeout * 1000;
        sendAPIpartition('default', 'MYMPD_API_CONNECTION_SAVE', settingsParams, saveConnectionClose, true);
    }
}

/**
 * Handler for the MYMPD_API_CONNECTION_SAVE jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function saveConnectionClose(obj) {
    if (obj.error) {
        if (highlightInvalidInput('modalSettingsConnection', obj) === false) {
            showModalAlert(obj);
        }
    }
    else {
        getSettings();
        uiElements.modalSettingsConnection.hide();
    }
}

/**
 * Populates the connection modal
 * @returns {void}
 */
function populateConnectionFrm() {
    jsonToForm(settings, settingsConnectionFields, 'modalSettingsConnection');
    document.getElementById('modalSettingsConnectionMpdTimeoutInput').value = settings.mpdTimeout / 1000;
    document.getElementById('modalSettingsConnectionMpdBinarylimitInput').value = settings.mpdBinarylimit / 1024;

    if (settings.musicDirectory === 'auto') {
        document.getElementById('modalSettingsConnectionMusicDirectorySelect').value = settings.musicDirectory;
        document.getElementById('modalSettingsConnectionMusicDirectoryInput').value = settings.musicDirectoryValue;
        document.getElementById('modalSettingsConnectionMusicDirectoryInput').setAttribute('readonly', 'readonly');
    }
    else if (settings.musicDirectory === 'none') {
        document.getElementById('modalSettingsConnectiontMusicDirectorySelect').value = settings.musicDirectory;
        document.getElementById('modalSettingsConnectionMusicDirectoryInput').value = '';
        document.getElementById('modalSettingsConnectionMusicDirectoryInput').setAttribute('readonly', 'readonly');
    }
    else {
        document.getElementById('modalSettingsConnectionMusicDirectorySelect').value = 'custom';
        document.getElementById('modalSettingsConnectionMusicDirectoryInput').value = settings.musicDirectoryValue;
        document.getElementById('modalSettingsConnectionMusicDirectoryInput').removeAttribute('readonly');
    }

    if (settings.musicDirectoryValue === '' &&
        settings.musicDirectory !== 'none')
    {
        elShowId('modalSettingsConnectionMusicDirectoryWarn');
    }
    else {
        elHideId('modalSettingsConnectionMusicDirectoryWarn');
    }

    if (features.featPlaylistDirAuto === false &&
        settings.playlistDirectory === 'auto')
    {
        settings.playlistDirectory = 'none';
    }

    if (settings.playlistDirectory === 'auto') {
        document.getElementById('modalSettingsConnectionPlaylistDirectorySelect').value = settings.playlistDirectory;
        document.getElementById('modalSettingsConnectionPlaylistDirectoryInput').value = settings.playlistDirectoryValue;
        document.getElementById('modalSettingsConnectionPlaylistDirectoryInput').setAttribute('readonly', 'readonly');
    }
    else if (settings.playlistDirectory === 'none') {
        document.getElementById('modalSettingsConnectionPlaylistDirectorySelect').value = settings.playlistDirectory;
        document.getElementById('modalSettingsConnectionPlaylistDirectoryInput').value = '';
        document.getElementById('modalSettingsConnectionPlaylistDirectoryInput').setAttribute('readonly', 'readonly');
    }
    else {
        document.getElementById('modalSettingsConnectionPlaylistDirectorySelect').value = 'custom';
        document.getElementById('modalSettingsConnectionPlaylistDirectoryInput').value = settings.playlistDirectoryValue;
        document.getElementById('modalSettingsConnectionPlaylistDirectoryInput').removeAttribute('readonly');
    }

    if (settings.playlistDirectoryValue === '' &&
        settings.playlistDirectory !== 'none')
    {
        elShowId('modalSettingsConnectionPlaylistDirectoryWarn');
    }
    else {
        elHideId('modalSettingsConnectionPlaylistDirectoryWarn');
    }
}
