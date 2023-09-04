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
    initElements(elGetById('modalConnection'));

    elGetById('modalConnectionMusicDirectorySelect').addEventListener('change', function() {
        const musicDirMode = getSelectValue(this);
        const musicDirInput = elGetById('modalConnectionMusicDirectoryInput');
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

    elGetById('modalConnectionPlaylistDirectorySelect').addEventListener('change', function() {
        const playlistDirMode = getSelectValue(this);
        const playlistDirInput = elGetById('modalConnectionPlaylistDirectoryInput');
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

    elGetById('modalConnection').addEventListener('show.bs.modal', function() {
        getSettings(function(obj) {
            if (parseSettings(obj) === true) {
                cleanupModalId('modalConnection');
                populateConnectionFrm();
                uiElements.modalConnection.show();
            }
        });
    });
}

/**
 * Saves the mpd connection settings
 * @param {Element} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function saveConnection(target) {
    cleanupModalId('modalConnection');
    const settingsParams = {};

    const mpdLocal = elGetById('modalConnectionMpdHostInput').value.indexOf('/') === 0
        ? true
        : false;

    const musicDirectoryEl = elGetById('modalConnectionMusicDirectorySelect');
    const musicDirectory = getSelectValue(musicDirectoryEl);
    if (musicDirectory === 'auto' &&
        mpdLocal === false)
    {
        setIsInvalid(musicDirectoryEl);
        return;
    }

    const playlistDirectoryEl = elGetById('modalConnectionPlaylistDirectorySelect');
    const playlistDirectory = getSelectValue(playlistDirectoryEl);
    if (playlistDirectory === 'auto' &&
        mpdLocal === false)
    {
        setIsInvalid(playlistDirectoryEl);
        return;
    }

    if (formToJson('modalConnection', settingsParams, settingsConnectionFields) === true) {
        if (musicDirectory === 'custom') {
            settingsParams.musicDirectory = elGetById('modalConnectionMusicDirectoryInput').value;
        }
        else {
            settingsParams.musicDirectory = musicDirectory;
        }
        if (playlistDirectory === 'custom') {
            settingsParams.playlistDirectory = elGetById('modalConnectionPlaylistDirectoryInput').value;
        }
        else {
            settingsParams.playlistDirectory = playlistDirectory;
        }

        settingsParams.mpdBinarylimit = settingsParams.mpdBinarylimit * 1024;
        settingsParams.mpdTimeout = settingsParams.mpdTimeout * 1000;
        btnWaiting(target, true);
        sendAPIpartition('default', 'MYMPD_API_CONNECTION_SAVE', settingsParams, saveConnectionClose, true);
    }
}

/**
 * Handler for the MYMPD_API_CONNECTION_SAVE jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function saveConnectionClose(obj) {
    if (modalClose(obj) === true) {
        getSettings(parseSettings);
    }
}

/**
 * Populates the connection modal
 * @returns {void}
 */
function populateConnectionFrm() {
    jsonToForm(settings, settingsConnectionFields, 'modalConnection');
    elGetById('modalConnectionMpdTimeoutInput').value = settings.mpdTimeout / 1000;
    elGetById('modalConnectionMpdBinarylimitInput').value = settings.mpdBinarylimit / 1024;

    if (settings.musicDirectory === 'auto') {
        elGetById('modalConnectionMusicDirectorySelect').value = settings.musicDirectory;
        elGetById('modalConnectionMusicDirectoryInput').value = settings.musicDirectoryValue;
        elGetById('modalConnectionMusicDirectoryInput').setAttribute('readonly', 'readonly');
    }
    else if (settings.musicDirectory === 'none') {
        elGetById('modalConnectiontMusicDirectorySelect').value = settings.musicDirectory;
        elGetById('modalConnectionMusicDirectoryInput').value = '';
        elGetById('modalConnectionMusicDirectoryInput').setAttribute('readonly', 'readonly');
    }
    else {
        elGetById('modalConnectionMusicDirectorySelect').value = 'custom';
        elGetById('modalConnectionMusicDirectoryInput').value = settings.musicDirectoryValue;
        elGetById('modalConnectionMusicDirectoryInput').removeAttribute('readonly');
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
        elGetById('modalConnectionPlaylistDirectorySelect').value = settings.playlistDirectory;
        elGetById('modalConnectionPlaylistDirectoryInput').value = settings.playlistDirectoryValue;
        elGetById('modalConnectionPlaylistDirectoryInput').setAttribute('readonly', 'readonly');
    }
    else if (settings.playlistDirectory === 'none') {
        elGetById('modalConnectionPlaylistDirectorySelect').value = settings.playlistDirectory;
        elGetById('modalConnectionPlaylistDirectoryInput').value = '';
        elGetById('modalConnectionPlaylistDirectoryInput').setAttribute('readonly', 'readonly');
    }
    else {
        elGetById('modalConnectionPlaylistDirectorySelect').value = 'custom';
        elGetById('modalConnectionPlaylistDirectoryInput').value = settings.playlistDirectoryValue;
        elGetById('modalConnectionPlaylistDirectoryInput').removeAttribute('readonly');
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
