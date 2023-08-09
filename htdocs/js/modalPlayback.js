"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalPlayback_js */

/**
 * Initialization function for the playback settings elements
 * @returns {void}
 */
function initModalSettingsPlayback() {
    // cache for the form field containers
    const forms = {};
    // create the fields
    createForm(settingsPlaybackFields, 'modalPlayback', forms);
    initElements(document.getElementById('modalPlayback'));

    uiElements.modalPlaybackJukeboxCollapse = BSN.Collapse.getInstance(document.getElementById('modalPlaybackJukeboxCollapse'));

    document.getElementById('modalPlayback').addEventListener('shown.bs.modal', function () {
        cleanupModalId('modalPlayback');
        getSettings();
    });

    document.getElementById('modalPlaybackJukeboxModeGroup').addEventListener('mouseup', function () {
        setTimeout(function() {
            toggleJukeboxSettings();
            checkConsume();
        }, 100);
    });

    document.getElementById('modalPlaybackConsumeGroup').addEventListener('mouseup', function() {
        setTimeout(function() {
            checkConsume();
        }, 100);
    });

    document.getElementById('modalPlaybackPresetsList').addEventListener('click', function(event) {
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
    const presetsEl = document.getElementById('modalPlaybackNameInput');
    presetsEl.value = '';
    setData(presetsEl, 'value', '');
    elClear(presetsEl.filterResult);
    const presetsList = document.getElementById('modalPlaybackPresetsList');
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
    const stateConsume = getBtnGroupValueId('modalPlaybackConsumeGroup');
    const stateJukeboxMode = getBtnGroupValueId('modalPlaybackJukeboxModeGroup');
    if (stateJukeboxMode !== 'off' &&
        stateConsume !== '1')
    {
        elShowId('modalPlaybackConsumeWarn');
    }
    else {
        elHideId('modalPlaybackConsumeWarn');
    }
}

/**
 * Toggle jukebox setting elements
 * @returns {void}
 */
function toggleJukeboxSettings() {
    const value = getBtnGroupValueId('modalPlaybackJukeboxModeGroup');
    if (value === 'off') {
        elDisableId('modalPlaybackJukeboxQueueLengthInput');
        elDisableId('modalPlaybackJukeboxPlaylistInput');
        elDisableId('modalPlaybackJukeboxIgnoreHatedInput');
    }
    else if (value === 'album') {
        elDisableId('modalPlaybackJukeboxQueueLengthInput');
        document.getElementById('modalPlaybackJukeboxQueueLengthInput').value = '1';
        elDisableId('modalPlaybackJukeboxPlaylistInput');
        elDisableId('modalPlaybackJukeboxIgnoreHatedInput');
        toggleBtnChkId('modalPlaybackJukeboxIgnoreHatedInput', false);
        elDisable(document.getElementById('modalPlaybackJukeboxPlaylistInput').nextElementSibling);
        document.getElementById('modalPlaybackJukeboxPlaylistInput').value = tn('Database');
        setDataId('modalPlaybackJukeboxPlaylistInput', 'value', 'Database');
    }
    else if (value === 'song') {
        elEnableId('modalPlaybackJukeboxQueueLengthInput');
        elEnableId('modalPlaybackJukeboxPlaylistInput');
        elEnableId('modalPlaybackJukeboxIgnoreHatedInput');
        elEnable(document.getElementById('modalPlaybackJukeboxPlaylistInput').nextElementSibling);
    }
    if (value !== 'off') {
        toggleBtnGroupValueId('modalPlaybackConsumeGroup', '1');
        toggleBtnGroupValueId('modalPlaybackSingleGroup', '0');
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
    jsonToForm(settings.partition, settingsPlaybackFields, 'modalPlayback');

    toggleBtnGroupValueCollapse(document.getElementById('modalPlaybackJukeboxModeGroup'), 'modalPlaybackJukeboxCollapse', settings.partition.jukeboxMode);
    addTagListSelect('modalPlaybackJukeboxUniqueTagInput', 'tagListBrowse');
    toggleJukeboxSettings();

    populateListPresets();

    if (settings.partition.mpdConnected === true) {
        if (features.featPlaylists === true) {
            filterPlaylistsSelect(0, 'modalPlaybackJukeboxPlaylistInput', '', settings.partition.jukeboxPlaylist);
            setDataId('modalPlaybackJukeboxPlaylistInput', 'value', settings.partition.jukeboxPlaylist);
        }
        else {
            document.getElementById('modalPlaybackJukeboxPlaylistInput').value = tn('Database');
            setDataId('modalPlaybackJukeboxPlaylistInput', 'value', 'Database');
        }
        toggleBtnGroupValueId('modalPlaybackConsumeGroup', settings.partition.consume);
        toggleBtnGroupValueId('modalPlaybackSingleGroup', settings.partition.single);
        toggleBtnGroupValueId('modalPlaybackReplaygainGroup', settings.partition.replaygain);
        if (features.featStickers === false) {
            elShowId('modalPlaybackPlaybackStatisticsWarn');
            elDisableId('modalPlaybackJukeboxLastPlayedInput');
        }
        else {
            elHideId('modalPlaybackPlaybackStatisticsWarn');
            elEnableId('modalPlaybackJukeboxLastPlayedInput');
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
    cleanupModalId('modalPlayback');
    const params = {};
    if (formToJson('modalPlayback', params, settingsPlaybackFields) === true) {
        params.jukeboxMode = getBtnGroupValueId('modalPlaybackJukeboxModeGroup');
        params.consume = getBtnGroupValueId('modalPlaybackConsumeGroup');
        params.single = getBtnGroupValueId('modalPlaybackSingleGroup');
        params.replaygain = getBtnGroupValueId('modalPlaybackReplaygainGroup');
        // enforce uniq tag for jukebox album mode
        params.jukeboxUniqueTag = params.jukeboxMode === 'album'
            ? 'Album'
            : getSelectValueId('modalPlaybackJukeboxUniqueTagInput');
        //set preset name to blank string if not defined, else it is not send to the api
        params.name = getDataId('modalPlaybackNameInput', 'value');
        if (params.name === undefined) {
            params.name = '';
        }
        btnWaitingId('modalPlaybackSaveBtn', true);
        sendAPI("MYMPD_API_PLAYER_OPTIONS_SET", params, saveSettingsPlaybackClose, true);
    }
}

/**
 * Handler for the MYMPD_API_PLAYER_OPTIONS_SET jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function saveSettingsPlaybackClose(obj) {
    btnWaitingId('modalPlaybackSaveBtn', false);
    if (obj.error) {
        if (highlightInvalidInput('modalPlayback', obj) === false) {
            showModalAlert(obj);
        }
    }
    else {
        uiElements.modalPlayback.hide();
    }
}
