"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalConnection_js */

/**
 * Initialization function for the connection settings elements
 * @returns {void}
 */
function initModalSettingsConnection() {
    // cache for the form field containers
    const forms = {};
    // create the fields
    createForm(settingsConnectionFields, 'modalConnection', forms);
    initElements(document.getElementById('modalConnection'));

    document.getElementById('modalConnectionMusicDirectorySelect').addEventListener('change', function () {
        const musicDirMode = getSelectValue(this);
        const musicDirInput = document.getElementById('modalConnectionMusicDirectoryInput');
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

    document.getElementById('modalConnectionPlaylistDirectorySelect').addEventListener('change', function () {
        const playlistDirMode = getSelectValue(this);
        const playlistDirInput = document.getElementById('modalConnectionPlaylistDirectoryInput');
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

    document.getElementById('modalConnection').addEventListener('shown.bs.modal', function () {
        cleanupModalId('modalConnection');
        getSettings();
    });
}

/**
 * Saves the mpd connection settings
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function saveConnection() {
    cleanupModalId('modalConnection');
    const settingsParams = {};

    const mpdLocal = document.getElementById('modalConnectionMpdHostInput').value.indexOf('/') === 0
        ? true
        : false;

    const musicDirectoryEl = document.getElementById('modalConnectionMusicDirectorySelect');
    const musicDirectory = getSelectValue(musicDirectoryEl);
    if (musicDirectory === 'auto' &&
        mpdLocal === false)
    {
        setIsInvalid(musicDirectoryEl);
        return;
    }

    const playlistDirectoryEl = document.getElementById('modalConnectionPlaylistDirectorySelect');
    const playlistDirectory = getSelectValue(playlistDirectoryEl);
    if (playlistDirectory === 'auto' &&
        mpdLocal === false)
    {
        setIsInvalid(playlistDirectoryEl);
        return;
    }

    if (formToJson('modalConnection', settingsParams, settingsConnectionFields) === true) {
        if (musicDirectory === 'custom') {
            settingsParams.musicDirectory = document.getElementById('modalConnectionMusicDirectoryInput').value;
        }
        else {
            settingsParams.musicDirectory = musicDirectory;
        }
        if (playlistDirectory === 'custom') {
            settingsParams.playlistDirectory = document.getElementById('modalConnectionPlaylistDirectoryInput').value;
        }
        else {
            settingsParams.playlistDirectory = playlistDirectory;
        }

        settingsParams.mpdBinarylimit = settingsParams.mpdBinarylimit * 1024;
        settingsParams.mpdTimeout = settingsParams.mpdTimeout * 1000;
        btnWaitingId('modalConnectionApplyBtn', true);
        sendAPIpartition('default', 'MYMPD_API_CONNECTION_SAVE', settingsParams, saveConnectionClose, true);
    }
}

/**
 * Handler for the MYMPD_API_CONNECTION_SAVE jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function saveConnectionClose(obj) {
    btnWaitingId('modalConnectionApplyBtn', false);
    if (obj.error) {
        if (highlightInvalidInput('modalConnection', obj) === false) {
            showModalAlert(obj);
        }
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
    jsonToForm(settings, settingsConnectionFields, 'modalConnection');
    document.getElementById('modalConnectionMpdTimeoutInput').value = settings.mpdTimeout / 1000;
    document.getElementById('modalConnectionMpdBinarylimitInput').value = settings.mpdBinarylimit / 1024;

    if (settings.musicDirectory === 'auto') {
        document.getElementById('modalConnectionMusicDirectorySelect').value = settings.musicDirectory;
        document.getElementById('modalConnectionMusicDirectoryInput').value = settings.musicDirectoryValue;
        document.getElementById('modalConnectionMusicDirectoryInput').setAttribute('readonly', 'readonly');
    }
    else if (settings.musicDirectory === 'none') {
        document.getElementById('modalConnectiontMusicDirectorySelect').value = settings.musicDirectory;
        document.getElementById('modalConnectionMusicDirectoryInput').value = '';
        document.getElementById('modalConnectionMusicDirectoryInput').setAttribute('readonly', 'readonly');
    }
    else {
        document.getElementById('modalConnectionMusicDirectorySelect').value = 'custom';
        document.getElementById('modalConnectionMusicDirectoryInput').value = settings.musicDirectoryValue;
        document.getElementById('modalConnectionMusicDirectoryInput').removeAttribute('readonly');
    }

    if (settings.musicDirectoryValue === '' &&
        settings.musicDirectory !== 'none')
    {
        elShowId('modalConnectionMusicDirectoryWarn');
    }
    else {
        elHideId('modalConnectionMusicDirectoryWarn');
    }

    if (features.featPlaylistDirAuto === false &&
        settings.playlistDirectory === 'auto')
    {
        settings.playlistDirectory = 'none';
    }

    if (settings.playlistDirectory === 'auto') {
        document.getElementById('modalConnectionPlaylistDirectorySelect').value = settings.playlistDirectory;
        document.getElementById('modalConnectionPlaylistDirectoryInput').value = settings.playlistDirectoryValue;
        document.getElementById('modalConnectionPlaylistDirectoryInput').setAttribute('readonly', 'readonly');
    }
    else if (settings.playlistDirectory === 'none') {
        document.getElementById('modalConnectionPlaylistDirectorySelect').value = settings.playlistDirectory;
        document.getElementById('modalConnectionPlaylistDirectoryInput').value = '';
        document.getElementById('modalConnectionPlaylistDirectoryInput').setAttribute('readonly', 'readonly');
    }
    else {
        document.getElementById('modalConnectionPlaylistDirectorySelect').value = 'custom';
        document.getElementById('modalConnectionPlaylistDirectoryInput').value = settings.playlistDirectoryValue;
        document.getElementById('modalConnectionPlaylistDirectoryInput').removeAttribute('readonly');
    }

    if (settings.playlistDirectoryValue === '' &&
        settings.playlistDirectory !== 'none')
    {
        elShowId('modalConnectionPlaylistDirectoryWarn');
    }
    else {
        elHideId('modalConnectionPlaylistDirectoryWarn');
    }
}
