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
    // cache for the form field containers
    const forms = {};
    // create the fields
    createForm(settingsPlaybackFields, 'modalSettingsPlayback', forms);
    initElements(document.getElementById('modalSettingsPlayback'));

    uiElements.modalSettingsPlaybackJukeboxCollapse = BSN.Collapse.getInstance(document.getElementById('modalSettingsPlaybackJukeboxCollapse'));

    document.getElementById('modalSettingsPlayback').addEventListener('shown.bs.modal', function () {
        cleanupModalId('modalSettingsPlayback');
        getSettings();
    });

    document.getElementById('modalSettingsPlaybackJukeboxModeGroup').addEventListener('mouseup', function () {
        setTimeout(function() {
            toggleJukeboxSettings();
            checkConsume();
        }, 100);
    });

    document.getElementById('modalSettingsPlaybackConsumeGroup').addEventListener('mouseup', function() {
        setTimeout(function() {
            checkConsume();
        }, 100);
    });

    document.getElementById('modalSettingsPlaybackPresetsList').addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (event.target.nodeName === 'TD') {
            if (event.target.parentNode.classList.contains('not-clickable') === false) {
                // @ts-ignore
                btnWaiting(event.target, true);
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
    const presetsEl = document.getElementById('modalSettingsPlaybackNameInput');
    presetsEl.value = '';
    setData(presetsEl, 'value', '');
    elClear(presetsEl.filterResult);
    const presetsList = document.getElementById('modalSettingsPlaybackPresetsList');
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
    const stateConsume = getBtnGroupValueId('modalSettingsPlaybackConsumeGroup');
    const stateJukeboxMode = getBtnGroupValueId('modalSettingsPlaybackJukeboxModeGroup');
    if (stateJukeboxMode !== 'off' &&
        stateConsume !== '1')
    {
        elShowId('modalSettingsPlaybackConsumeWarn');
    }
    else {
        elHideId('modalSettingsPlaybackConsumeWarn');
    }
}

/**
 * Toggle jukebox setting elements
 * @returns {void}
 */
function toggleJukeboxSettings() {
    const value = getBtnGroupValueId('modalSettingsPlaybackJukeboxModeGroup');
    if (value === 'off') {
        elDisableId('modalSettingsPlaybackJukeboxQueueLengthInput');
        elDisableId('modalSettingsPlaybackJukeboxPlaylistInput');
        elDisableId('modalSettingsPlaybackJukeboxIgnoreHatedInput');
    }
    else if (value === 'album') {
        elDisableId('modalSettingsPlaybackJukeboxQueueLengthInput');
        document.getElementById('modalSettingsPlaybackJukeboxQueueLengthInput').value = '1';
        elDisableId('modalSettingsPlaybackJukeboxPlaylistInput');
        elDisableId('modalSettingsPlaybackJukeboxIgnoreHatedInput');
        toggleBtnChkId('modalSettingsPlaybackJukeboxIgnoreHatedInput', false);
        elDisable(document.getElementById('modalSettingsPlaybackJukeboxPlaylistInput').nextElementSibling);
        document.getElementById('modalSettingsPlaybackJukeboxPlaylistInput').value = tn('Database');
        setDataId('modalSettingsPlaybackJukeboxPlaylistInput', 'value', 'Database');
    }
    else if (value === 'song') {
        elEnableId('modalSettingsPlaybackJukeboxQueueLengthInput');
        elEnableId('modalSettingsPlaybackJukeboxPlaylistInput');
        elEnableId('modalSettingsPlaybackJukeboxIgnoreHatedInput');
        elEnable(document.getElementById('modalSettingsPlaybackJukeboxPlaylistInput').nextElementSibling);
    }
    if (value !== 'off') {
        toggleBtnGroupValueId('modalSettingsPlaybackConsumeGroup', '1');
        toggleBtnGroupValueId('modalSettingsPlaybackSingleGroup', '0');
    }
}

/**
 * Creates a row for the presets tables
 * @param {string} preset preset name
 * @returns {HTMLElement} the row
 */
function createPresetsListRow(preset) {
    const row = elCreateNodes('tr', {"data-title-phrase": "Load preset", "title": tn("Load preset")}, [
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
    jsonToForm(settings.partition, settingsPlaybackFields, 'modalSettingsPlayback');

    toggleBtnGroupValueCollapse(document.getElementById('modalSettingsPlaybackJukeboxModeGroup'), 'modalSettingsPlaybackJukeboxCollapse', settings.partition.jukeboxMode);
    addTagListSelect('modalSettingsPlaybackJukeboxUniqueTagInput', 'tagListBrowse');
    toggleJukeboxSettings();

    populateListPresets();

    if (settings.partition.mpdConnected === true) {
        if (features.featPlaylists === true) {
            filterPlaylistsSelect(0, 'modalSettingsPlaybackJukeboxPlaylistInput', '', settings.partition.jukeboxPlaylist);
            setDataId('modalSettingsPlaybackJukeboxPlaylistInput', 'value', settings.partition.jukeboxPlaylist);
        }
        else {
            document.getElementById('modalSettingsPlaybackJukeboxPlaylistInput').value = tn('Database');
            setDataId('modalSettingsPlaybackJukeboxPlaylistInput', 'value', 'Database');
        }
        toggleBtnGroupValueId('modalSettingsPlaybackConsumeGroup', settings.partition.consume);
        toggleBtnGroupValueId('modalSettingsPlaybackSingleGroup', settings.partition.single);
        toggleBtnGroupValueId('modalSettingsPlaybackReplaygainGroup', settings.partition.replaygain);
        if (features.featStickers === false) {
            elShowId('modalSettingsPlaybackPlaybackStatisticsWarn');
            elDisableId('modalSettingsPlaybackJukeboxLastPlayedInput');
        }
        else {
            elHideId('modalSettingsPlaybackPlaybackStatisticsWarn');
            elEnableId('modalSettingsPlaybackJukeboxLastPlayedInput');
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
    const params = {};
    if (formToJson('modalSettingsPlayback', params, settingsPlaybackFields) === true) {
        params.jukeboxMode = getBtnGroupValueId('modalSettingsPlaybackJukeboxModeGroup');
        params.consume = getBtnGroupValueId('modalSettingsPlaybackConsumeGroup');
        params.single = getBtnGroupValueId('modalSettingsPlaybackSingleGroup');
        params.replaygain = getBtnGroupValueId('modalSettingsPlaybackReplaygainGroup');
        // enforce uniq tag for jukebox album mode
        params.jukeboxUniqueTag = params.jukeboxMode === 'album'
            ? 'Album'
            : getSelectValueId('modalSettingsPlaybackJukeboxUniqueTagInput');
        //set preset name to blank string if not defined, else it is not send to the api
        params.name = getDataId('modalSettingsPlaybackNameInput', 'value');
        if (params.name === undefined) {
            params.name = '';
        }
        btnWaitingId('modalSettingsPlaybackSaveBtn', true);
        sendAPI("MYMPD_API_PLAYER_OPTIONS_SET", params, saveSettingsPlaybackClose, true);
    }
}

/**
 * Handler for the MYMPD_API_PLAYER_OPTIONS_SET jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function saveSettingsPlaybackClose(obj) {
    btnWaitingId('modalSettingsPlaybackSaveBtn', false);
    if (obj.error) {
        if (highlightInvalidInput('modalSettingsPlayback', obj) === false) {
            showModalAlert(obj);
        }
    }
    else {
        uiElements.modalSettingsPlayback.hide();
    }
}
