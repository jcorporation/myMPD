"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
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
    initElements(elGetById('modalPlayback'));

    uiElements.modalPlaybackJukeboxCollapse = BSN.Collapse.getInstance(elGetById('modalPlaybackJukeboxCollapse'));

    elGetById('modalPlaybackJukeboxModeGroup').addEventListener('mouseup', function() {
        setTimeout(function() {
            toggleJukeboxSettings();
            checkConsume();
        }, 100);
    });

    elGetById('modalPlaybackConsumeGroup').addEventListener('mouseup', function() {
        setTimeout(function() {
            checkConsume();
        }, 100);
    });

    elGetById('modalPlaybackPresetsList').addEventListener('click', function(event) {
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

    elGetById('modalPlayback').addEventListener('show.bs.modal', function() {
        getSettings(function(obj) {
            if (parseSettings(obj) === true) {
                cleanupModalId('modalPlayback');
                populatePlaybackFrm();
            }
        });
    });
}

/**
 * Populates the presets lists
 * @returns {void}
 */
function populateListPresets() {
    const presetsEl = elGetById('modalPlaybackNameInput');
    presetsEl.value = '';
    setData(presetsEl, 'value', '');
    elClear(presetsEl.filterResult);
    const table = elGetById('modalPlaybackPresetsList');
    const presetsList = table.querySelector('tbody');
    elClear(presetsList);
    for (const preset of settings.partition.presets) {
        presetsEl.addFilterResultPlain(preset);
        presetsList.appendChild(createPresetsListRow(preset));
    }
    if (settings.partition.presets.length === 0) {
        presetsList.appendChild(emptyMsgEl(2, 'table'));
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
        default:
            logError('Invalid playmode option: ' + option);
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
    const songOnlyInputs = elGetById('modalPlaybackJukeboxCollapse').querySelectorAll('.jukeboxSongOnly');
    if (value === 'album') {
        elGetById('modalPlaybackJukeboxQueueLengthInput').value = '1';
        toggleBtnChkId('modalPlaybackJukeboxIgnoreHatedInput', false);
        elGetById('modalPlaybackJukeboxPlaylistInput').value = tn('Database');
        setDataId('modalPlaybackJukeboxPlaylistInput', 'value', 'Database');
        const select = elGetById('modalPlaybackJukeboxUniqTagInput');
        elClear(select);
        for (const tag of ['Album', tagAlbumArtist]) {
            select.appendChild(
                elCreateTextTn('option', {"value": tag}, tag)
            );
        }
        // hide rows that are not configurable in jukebox album mode
        for (const input of songOnlyInputs) {
            elHide(input.closest('.row'));
        }
    }
    else if (value === 'song') {
        addTagListSelect('modalPlaybackJukeboxUniqTagInput', 'tagListBrowse');
        // show rows that are configurable in jukebox song mode only
        for (const input of songOnlyInputs) {
            elShow(input.closest('.row'));
        }
    }
    if (value !== 'off') {
        toggleBtnGroupValueId('modalPlaybackConsumeGroup', '1');
        toggleBtnGroupValueId('modalPlaybackSingleGroup', '0');
    }

    if (features.featStickers === false) {
        elShowId('modalPlaybackPlaybackStatisticsWarn');
        elHide(elGetById('modalPlaybackJukeboxLastPlayedInput').closest('.row'));
        elHide(elGetById('modalPlaybackJukeboxIgnoreHatedInput').closest('.row'));
        toggleBtnChkId('modalPlaybackJukeboxIgnoreHatedInput', false);
    }
    else {
        elHideId('modalPlaybackPlaybackStatisticsWarn');
        elShow(elGetById('modalPlaybackJukeboxLastPlayedInput').closest('.row'));
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
function populatePlaybackFrm() {
    jsonToForm(settings.partition, settingsPlaybackFields, 'modalPlayback');

    toggleBtnGroupValueCollapse(elGetById('modalPlaybackJukeboxModeGroup'), 'modalPlaybackJukeboxCollapse', settings.partition.jukeboxMode);
    toggleJukeboxSettings();
    elGetById('modalPlaybackJukeboxUniqTagInput').value = settings.partition.jukeboxUniqTag;

    populateListPresets();

    if (settings.partition.mpdConnected === true) {
        if (features.featPlaylists === true) {
            filterPlaylistsSelect(0, 'modalPlaybackJukeboxPlaylistInput', '', settings.partition.jukeboxPlaylist);
            setDataId('modalPlaybackJukeboxPlaylistInput', 'value', settings.partition.jukeboxPlaylist);
        }
        else {
            elGetById('modalPlaybackJukeboxPlaylistInput').value = tn('Database');
            setDataId('modalPlaybackJukeboxPlaylistInput', 'value', 'Database');
        }
        toggleBtnGroupValueId('modalPlaybackConsumeGroup', settings.partition.consume);
        toggleBtnGroupValueId('modalPlaybackSingleGroup', settings.partition.single);
        toggleBtnGroupValueId('modalPlaybackReplaygainGroup', settings.partition.replaygain);
        if (features.featTags === false) {
            elGetById('modalPlaybackJukeboxModeGroup').children[1].classList.add('rounded-end');
        }
        else {
            elGetById('modalPlaybackJukeboxModeGroup').children[1].classList.remove('rounded-end');
        }
    }
    checkConsume();
}

/**
 * Saves the playback settings
 * @param {Element} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function saveSettingsPlayback(target) {
    cleanupModalId('modalPlayback');
    const params = {};
    if (formToJson('modalPlayback', params, settingsPlaybackFields) === true) {
        params.jukeboxMode = getBtnGroupValueId('modalPlaybackJukeboxModeGroup');
        params.consume = getBtnGroupValueId('modalPlaybackConsumeGroup');
        params.single = getBtnGroupValueId('modalPlaybackSingleGroup');
        params.replaygain = getBtnGroupValueId('modalPlaybackReplaygainGroup');
        //set preset name to blank string if not defined, else it is not send to the api
        params.name = getDataId('modalPlaybackNameInput', 'value');
        if (params.name === undefined) {
            params.name = '';
        }
        btnWaiting(target, true);
        sendAPI("MYMPD_API_PLAYER_OPTIONS_SET", params, modalClose, true);
    }
}
