"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalSettingsPlayback_js */

/**
 * Initialization function for the playback settings elements
 * @returns {void}
 */
function initModalSettingsPlayback() {
    initElements(document.getElementById('modalSettingsPlayback'));

    document.getElementById('modalSettingsPlayback').addEventListener('shown.bs.modal', function () {
        cleanupModalId('modalSettingsPlayback');
        getSettings();
    });

    document.getElementById('btnJukeboxModeGroup').addEventListener('mouseup', function () {
        setTimeout(function() {
            toggleJukeboxSettings();
            checkConsume();
        }, 100);
    });

    document.getElementById('btnConsumeGroup').addEventListener('mouseup', function() {
        setTimeout(function() {
            checkConsume();
        }, 100);
    });

    setDataId('selectJukeboxPlaylist', 'cb-filter', 'filterPlaylistsSelect');
    setDataId('selectJukeboxPlaylist', 'cb-filter-options', [0, 'selectJukeboxPlaylist']);

    document.getElementById('listPresetsList').addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (event.target.nodeName === 'TD') {
            if (!event.target.parentNode.classList.contains('not-clickable')) {
                applyPreset(getData(event.target.parentNode, 'name'));
            }
        }
        else if (event.target.nodeName === 'A') {
            deletePreset(event.target, getData(event.target.parentNode.parentNode, 'name'));
        }
    }, false);
}

/**
 * Populates the presets lists
 * @returns {void}
 */
function populateListPresets() {
    const presetsEl = document.getElementById('inputPresetName');
    presetsEl.value = '';
    setData(presetsEl, 'value', '');
    elClear(presetsEl.filterResult);
    const presetsList = document.getElementById('listPresetsList');
    elClear(presetsList);
    for (const preset of settings.partition.presets) {
        presetsEl.addFilterResultPlain(preset);
        presetsList.appendChild(createPresetsListRow(preset));
    }
    if (settings.partition.presets.length === 0) {
        presetsList.appendChild(emptyRow(2));
    }
    populatePresetDropdowns();
}

/**
 * Toggles the mpd playback mode
 * @param {string} option playback option to toggle
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function togglePlaymode(option) {
    let value;
    let title;
    switch(option) {
        case 'random':
        case 'repeat':
            if (settings.partition[option] === true) {
                value = false;
                title = 'Disable ' + option;
            }
            else {
                value = true;
                title = 'Enable ' + option;
            }
            break;
        case 'consume':
            if (settings.partition.consume === '0') {
                if (features.featConsumeOneshot === true) {
                    value = 'oneshot';
                    title = 'Enable consume oneshot';
                }
                else {
                    value = '1';
                    title = 'Enable consume mode';
                }
            }
            else if (settings.partition.consume === 'oneshot') {
                value = '1';
                title = 'Enable consume mode';
            }
            else if (settings.partition.consume === '1') {
                value = '0';
                title = 'Disable consume mode';
            }
            break;
        case 'single':
            if (settings.partition.single === '0') {
                value = 'oneshot';
                title = 'Enable single oneshot';
            }
            else if (settings.partition.single === 'oneshot') {
                value = '1';
                title = 'Enable single mode';
            }
            else if (settings.partition.single === '1') {
                value = '0';
                title = 'Disable single mode';
            }
            break;
    }
    const params = {};
    params[option] = value;
    sendAPI('MYMPD_API_PLAYER_OPTIONS_SET', params, null, false);
    showNotification(tn(title), 'queue', 'info');
}

/**
 * Checks the state of the consume mode
 * @returns {void}
 */
function checkConsume() {
    const stateConsume = getBtnGroupValueId('btnConsumeGroup');
    const stateJukeboxMode = getBtnGroupValueId('btnJukeboxModeGroup');
    if (stateJukeboxMode !== 'off' &&
        stateConsume !== '1')
    {
        elShowId('warnConsume');
    }
    else {
        elHideId('warnConsume');
    }
}

/**
 * Toggle jukebox setting elements
 * @returns {void}
 */
function toggleJukeboxSettings() {
    const value = getBtnGroupValueId('btnJukeboxModeGroup');
    if (value === 'off') {
        elDisableId('inputJukeboxQueueLength');
        elDisableId('selectJukeboxPlaylist');
        elDisableId('btnJukeboxIgnoreHated');
    }
    else if (value === 'album') {
        elDisableId('inputJukeboxQueueLength');
        document.getElementById('inputJukeboxQueueLength').value = '1';
        elDisableId('selectJukeboxPlaylist');
        elDisableId('btnJukeboxIgnoreHated');
        toggleBtnChkId('btnJukeboxIgnoreHated', false);
        elDisable(document.getElementById('selectJukeboxPlaylist').nextElementSibling);
        document.getElementById('selectJukeboxPlaylist').value = 'Database';
        setDataId('selectJukeboxPlaylist', 'value', 'Database');
    }
    else if (value === 'song') {
        elEnableId('inputJukeboxQueueLength');
        elEnableId('selectJukeboxPlaylist');
        elEnableId('btnJukeboxIgnoreHated');
        elEnable(document.getElementById('selectJukeboxPlaylist').nextElementSibling);
    }
    if (value !== 'off') {
        toggleBtnGroupValueId('btnConsumeGroup', '1');
        toggleBtnGroupValueId('btnSingleGroup', '0');
    }
}

/**
 * Creates a row for the presets tables
 * @param {string} preset preset name
 * @returns {HTMLElement} the row
 */
function createPresetsListRow(preset) {
    const row = elCreateNodes('tr', {"data-title-phrase": "Load preset", "phrase": tn('Load preset')}, [
        elCreateText('td', {}, preset),
        elCreateNode('td', {"data-col": "Action"},
            elCreateText('a', {"class": ["mi", "color-darkgrey"], "href": "#", "data-title-phrase": "Delete", "phrase": tn('Delete')}, 'delete')
        )
    ]);
    setData(row, 'name', preset);
    return row;
}

/**
 * Populates the playback settings modal
 * @returns {void}
 */
function populateSettingsPlaybackFrm() {
    toggleBtnGroupValueCollapse(document.getElementById('btnJukeboxModeGroup'), 'collapseJukeboxMode', settings.partition.jukeboxMode);
    addTagListSelect('selectJukeboxUniqueTag', 'tagListBrowse');

    document.getElementById('selectJukeboxUniqueTag').value = settings.partition.jukeboxUniqueTag;
    document.getElementById('inputJukeboxQueueLength').value = settings.partition.jukeboxQueueLength;
    document.getElementById('inputJukeboxLastPlayed').value = settings.partition.jukeboxLastPlayed;
    toggleJukeboxSettings();
    document.getElementById('selectJukeboxPlaylist').filterInput.value = '';
    toggleBtnChkId('btnJukeboxIgnoreHated', settings.partition.jukeboxIgnoreHated);

    populateListPresets();

    if (settings.partition.mpdConnected === true) {
        if (features.featPlaylists === true) {
            filterPlaylistsSelect(0, 'selectJukeboxPlaylist', '', settings.partition.jukeboxPlaylist);
            setDataId('selectJukeboxPlaylist', 'value', settings.partition.jukeboxPlaylist);
        }
        else {
            document.getElementById('selectJukeboxPlaylist').value = tn('Database');
            setDataId('selectJukeboxPlaylist', 'value', 'Database');
        }
        toggleBtnChkId('btnRandom', settings.partition.random);
        toggleBtnChkId('btnRepeat', settings.partition.repeat);
        toggleBtnChkId('btnAutoPlay', settings.partition.autoPlay);
        toggleBtnGroupValueId('btnConsumeGroup', settings.partition.consume);
        toggleBtnGroupValueId('btnSingleGroup', settings.partition.single);
        toggleBtnGroupValueId('btnReplaygainGroup', settings.partition.replaygain);
        document.getElementById('inputCrossfade').value = settings.partition.crossfade;
        document.getElementById('inputMixrampDb').value = settings.partition.mixrampDb;
        document.getElementById('inputMixrampDelay').value = settings.partition.mixrampDelay;
        if (features.featStickers === false) {
            elShowId('warnPlaybackStatistics');
            elDisableId('inputJukeboxLastPlayed');
        }
        else {
            elHideId('warnPlaybackStatistics');
            elEnableId('inputJukeboxLastPlayed');
        }
    }
    checkConsume();
}

/**
 * Saves the playback settings
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function saveSettingsPlayback() {
    cleanupModalId('modalSettingsPlayback');
    let formOK = true;

    for (const inputId of ['inputCrossfade', 'inputJukeboxQueueLength', 'inputJukeboxLastPlayed']) {
        const inputEl = document.getElementById(inputId);
        if (validateIntEl(inputEl) === false) {
            formOK = false;
        }
    }

    const mixrampDbEl = document.getElementById('inputMixrampDb');
    if (validateFloatRangeEl(mixrampDbEl, -100, 0) === false) {
        formOK = false;
    }
    const mixrampDelayEl = document.getElementById('inputMixrampDelay');
    if (validateFloatRangeEl(mixrampDelayEl, -1, 100) === false) {
        formOK = false;
    }

    const jukeboxMode = getBtnGroupValueId('btnJukeboxModeGroup');
    const jukeboxUniqueTag = jukeboxMode === 'album'
        ? 'Album'
        : getSelectValueId('selectJukeboxUniqueTag');

    let presetName = getDataId('inputPresetName', 'value');
    if (presetName === undefined) {
        //set preset name to blank string, else it is not send to the api
        presetName = '';
    }

    if (formOK === true) {
        const params = {
            "name": presetName,
            "random": getBtnChkValueId('btnRandom'),
            "single": getBtnGroupValueId('btnSingleGroup'),
            "consume": getBtnGroupValueId('btnConsumeGroup'),
            "repeat": getBtnChkValueId('btnRepeat'),
            "replaygain": getBtnGroupValueId('btnReplaygainGroup'),
            "crossfade": Number(document.getElementById('inputCrossfade').value),
            "mixrampDb": Number(mixrampDbEl.value),
            "mixrampDelay": Number(mixrampDelayEl.value),
            "jukeboxMode": jukeboxMode,
            "jukeboxPlaylist": getDataId('selectJukeboxPlaylist', 'value'),
            "jukeboxQueueLength": Number(document.getElementById('inputJukeboxQueueLength').value),
            "jukeboxLastPlayed": Number(document.getElementById('inputJukeboxLastPlayed').value),
            "jukeboxUniqueTag": jukeboxUniqueTag,
            "jukeboxIgnoreHated": getBtnChkValueId('btnJukeboxIgnoreHated'),
            "autoPlay": getBtnChkValueId('btnAutoPlay')
        };
        btnWaitingId('btnSaveSettingsPlayback', true);
        sendAPI("MYMPD_API_PLAYER_OPTIONS_SET", params, saveSettingsPlaybackClose, true);
    }
}

/**
 * Handler for the MYMPD_API_PLAYER_OPTIONS_SET jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function saveSettingsPlaybackClose(obj) {
    btnWaitingId('btnSaveSettingsPlayback', false);
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        uiElements.modalSettingsPlayback.hide();
    }
}
