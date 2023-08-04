"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module settingsPlayback_js */

/**
 * Initialization function for the playback settings elements
 * @returns {void}
 */
function initPresets() {
    for (let i = 1; i <= 3; i++) {
        document.getElementById('selectPresetDropdown' + i).addEventListener('click', function(event) {
            event.preventDefault();
            if (event.target.nodeName === 'BUTTON') {
                applyPreset(event.target.textContent);
                const d = event.target.parentNode.parentNode.previousElementSibling;
                BSN.Dropdown.getInstance(d).hide();
            }
        }, false);
    }
}

/**
 * Applies a preset
 * @param {string} name preset name to load
 * @returns {void}
 */
function applyPreset(name) {
    sendAPI("MYMPD_API_PRESET_APPLY", {
        "name": name
    }, applyPresetCheckError, true);
}

/**
 * Handler for the MYMPD_API_PRESET_APPLY jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
 function applyPresetCheckError(obj) {
    if (obj.error) {
        if (getOpenModal() !== null) {
            showModalAlert(obj);
        }
    }
    else {
        getSettings();
    }
}

/**
 * Deletes a preset
 * @param {EventTarget} el triggering element
 * @param {string} name the preset name
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function deletePreset(el, name) {
    showConfirmInline(el.parentNode.previousSibling, tn('Do you really want to delete the preset?'), tn('Yes, delete it'), function() {
        sendAPI("MYMPD_API_PRESET_RM", {
            "name": name
        }, deletePresetCheckError, true);
    });
}

/**
 * Handler for the MYMPD_API_PRESET_RM jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
 function deletePresetCheckError(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        getSettings();
    }
}

/**
 * Populates the preset dropdowns
 * @returns {void}
 */
function populatePresetDropdowns() {
    const presetDropdowns = [
        document.getElementById('selectPresetDropdown1').lastElementChild,
        document.getElementById('selectPresetDropdown2').lastElementChild,
        document.getElementById('selectPresetDropdown3').lastElementChild
    ];
    const selectTimerPreset = document.getElementById('selectTimerPreset');
    for (const d of presetDropdowns) {
        elClear(d);
    }
    elClear(selectTimerPreset);
    selectTimerPreset.appendChild(
        elCreateTextTn('option', {"value": ""}, 'No preset')
    );
    for (const preset of settings.partition.presets) {
        const a = elCreateText('button', {"type":"button", "class": ["btn", "btn-secondary", "btn-sm"]}, preset);
        for (const d of presetDropdowns) {
            d.appendChild(a.cloneNode(true));
        }
        selectTimerPreset.appendChild(
            elCreateText('option', {"value": preset}, preset)
        );
    }
}
