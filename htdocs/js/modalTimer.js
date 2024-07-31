"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalTimer_js */

/**
 * Initialization function for the timer elements
 * @returns {void}
 */
function initModalTimer() {
    elGetById('modalTimerList').addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (event.target.nodeName === 'A') {
            deleteTimer(event.target, getData(event.target.parentNode.parentNode, 'id'));
            return;
        }
        if (event.target.nodeName === 'BUTTON') {
            toggleTimer(event.target, getData(event.target.parentNode.parentNode, 'id'));
            return;
        }
        const target = event.target.closest('TR');
        if (checkTargetClick(target) === true) {
            showEditTimer(getData(target, 'id'));
        }
    }, false);

    const modalTimerstartHourInputEl = elGetById('modalTimerstartHourInput');
    for (let i = 0; i < 24; i++) {
        modalTimerstartHourInputEl.appendChild(
            elCreateText('option', {"value": i}, zeroPad(i, 2))
        );
    }

    const modalTimerstartMinuteInputEl = elGetById('modalTimerstartMinuteInput');
    for (let i = 0; i < 60; i = i + 5) {
        modalTimerstartMinuteInputEl.appendChild(
            elCreateText('option', {"value": i}, zeroPad(i, 2))
        );
    }

    elGetById('modalTimerVolumeInput').addEventListener('change', function() {
        elGetById('textTimerVolume').textContent = this.value + ' %';
    }, false);

    elGetById('modalTimerActionInput').addEventListener('change', function() {
        selectTimerActionChange();
    }, false);

    elGetById('modalTimerIntervalInput').addEventListener('change', function() {
        selectTimerIntervalChange();
    }, false);

    elGetById('modalTimer').addEventListener('show.bs.modal', function () {
        showListTimer();
    });

    setDataId('modalTimerPlaylistInput', 'cb-filter', 'filterPlaylistsSelect');
    setDataId('modalTimerPlaylistInput', 'cb-filter-options', [0, 'modalTimerPlaylistInput']);
}

/**
 * Deletes a timer
 * @param {EventTarget} el triggering element
 * @param {number} timerid the timer id
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function deleteTimer(el, timerid) {
    showConfirmInline(el.parentNode.previousSibling, tn('Do you really want to delete the timer?'), tn('Yes, delete it'), function() {
        sendAPI("MYMPD_API_TIMER_RM", {
            "timerid": timerid
        }, deleteTimerCheckError, true);
    });
}

/**
 * Handler for the MYMPD_API_TIMER_RM jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function deleteTimerCheckError(obj) {
    if (modalListApply(obj) === true) {
        showListTimer();
    }
}

/**
 * Toggles the timer enabled state
 * @param {EventTarget} target triggering element
 * @param {number} timerid the timer id
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function toggleTimer(target, timerid) {
    if (target.classList.contains('active')) {
        target.classList.remove('active');
        sendAPI("MYMPD_API_TIMER_TOGGLE", {
            "timerid": timerid,
            "enabled": false
        }, showListTimer, false);
    }
    else {
        target.classList.add('active');
        sendAPI("MYMPD_API_TIMER_TOGGLE", {
            "timerid": timerid,
            "enabled": true
        }, showListTimer, false);
    }
}

/**
 * Saves the timer
 * @param {Element} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function saveTimer(target) {
    cleanupModalId('modalTimer');
    btnWaiting(target, true);

    let minOneDay = false;
    const weekdayBtns = ['modalTimerMonBtn', 'modalTimerTueBtn', 'modalTimerWedBtn', 'modalTimerThuBtn', 'modalTimerFriBtn', 'modalTimerSatBtn', 'modalTimerSunBtn'];
    const weekdays = [];
    for (let i = 0, j = weekdayBtns.length; i < j; i++) {
        const checked = elGetById(weekdayBtns[i]).classList.contains('active') ? true : false;
        weekdays.push(checked);
        if (checked === true) {
            minOneDay = true;
        }
    }
    if (minOneDay === false) {
        setIsInvalidId('modalTimerSunBtn');
        btnWaiting(target, false);
        return;
    }

    const args = {};
    const argEls = document.querySelectorAll('#modalTimerScriptActionArguments input');
    for (let i = 0, j = argEls.length; i < j; i++) {
        args[getData(argEls[i], 'name')] = argEls[i].value;
    }
    let interval = Number(getSelectValueId('modalTimerIntervalInput'));
    if (interval === -2) {
        //repeat
        interval = Number(elGetById('modalTimerIntervalRepeatInput').value);
        //convert interval to seconds
        const unit = Number(getSelectValueId('modalTimerIntervalRepeatUnit'));
        interval = interval * unit;
    }
    let preset = getSelectValueId('modalTimerPresetInput');
    if (preset === undefined) {
        //set to empty string, else the jsonrpc parameter is not set
        preset = '';
    }
    const modalTimerActionInput = elGetById('modalTimerActionInput');
    sendAPI("MYMPD_API_TIMER_SAVE", {
        "timerid": getDataId('modalTimerEditTab', 'id'),
        "name": elGetById('modalTimerNameInput').value,
        "interval": interval,
        "enabled": (elGetById('modalTimerEnabledInput').classList.contains('active') ? true : false),
        "startHour": Number(getSelectValueId('modalTimerstartHourInput')),
        "startMinute": Number(getSelectValueId('modalTimerstartMinuteInput')),
        "weekdays": weekdays,
        "action": getData(modalTimerActionInput.options[modalTimerActionInput.selectedIndex].parentNode, 'value'),
        "subaction": getSelectValue(modalTimerActionInput),
        "volume": Number(elGetById('modalTimerVolumeInput').value),
        "playlist": getDataId('modalTimerPlaylistInput', 'value'),
        "preset": preset,
        "arguments": args
    }, saveTimerCheckError, true);
}

/**
 * Handler for the MYMPD_API_TIMER_SAVE jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function saveTimerCheckError(obj) {
    if (modalApply(obj) === true) {
        showListTimer();
    }
}

/**
 * Shows the edit timer tab
 * @param {number} timerid the timer id
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showEditTimer(timerid) {
    cleanupModalId('modalTimer');
    elHideId('timerActionPlay');
    elHideId('modalTimerScriptActionGroup');
    elGetById('modalTimerListTab').classList.remove('active');
    elGetById('modalTimerEditTab').classList.add('active');
    elHideId('modalTimerListFooter');
    elShowId('modalTimerEditFooter');
    elGetById('modalTimerPlaylistInput').filterInput.value = '';

    if (timerid !== 0) {
        sendAPI("MYMPD_API_TIMER_GET", {
            "timerid": timerid
        }, parseEditTimer, false);
    }
    else {
        filterPlaylistsSelect(0, 'modalTimerPlaylistInput', '', '');
        setDataId('modalTimerEditTab', 'id', 0);
        elGetById('modalTimerNameInput').value = '';
        toggleBtnChkId('modalTimerEnabledInput', true);
        elGetById('modalTimerstartHourInput').value = '12';
        elGetById('modalTimerstartMinuteInput').value = '0';
        elGetById('modalTimerActionInput').value = 'startplay';
        elGetById('modalTimerVolumeInput').value = '50';
        elGetById('textTimerVolume').textContent = '50 %';
        selectTimerIntervalChange(86400);
        selectTimerActionChange();
        const weekdayBtns = ['modalTimerMonBtn', 'modalTimerTueBtn', 'modalTimerWedBtn', 'modalTimerThuBtn', 'modalTimerFriBtn', 'modalTimerSatBtn', 'modalTimerSunBtn'];
        for (let i = 0, j = weekdayBtns.length; i < j; i++) {
            toggleBtnId(weekdayBtns[i], false);
        }
        elShowId('timerActionPlay');
    }
    setFocusId('modalTimerNameInput');
}

/**
 * Parses the MYMPD_API_TIMER_GET response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parseEditTimer(obj) {
    filterPlaylistsSelect(0, 'modalTimerPlaylistInput', '', obj.result.playlist);
    setDataId('modalTimerEditTab', 'id', obj.result.timerid);
    elGetById('modalTimerNameInput').value = obj.result.name;
    toggleBtnChkId('modalTimerEnabledInput', obj.result.enabled);
    elGetById('modalTimerstartHourInput').value = obj.result.startHour;
    elGetById('modalTimerstartMinuteInput').value = obj.result.startMinute;
    elGetById('modalTimerActionInput').value = obj.result.subaction;
    selectTimerActionChange(obj.result.arguments);
    selectTimerIntervalChange(obj.result.interval);
    elGetById('modalTimerVolumeInput').value = obj.result.volume;
    elGetById('textTimerVolume').textContent = obj.result.volume + ' %';
    elGetById('modalTimerPresetInput').value = obj.result.preset;

    const weekdayBtns = ['modalTimerMonBtn', 'modalTimerTueBtn', 'modalTimerWedBtn', 'modalTimerThuBtn', 'modalTimerFriBtn', 'modalTimerSatBtn', 'modalTimerSunBtn'];
    for (let i = 0, j = weekdayBtns.length; i < j; i++) {
        toggleBtnId(weekdayBtns[i], obj.result.weekdays[i]);
    }
}

/**
 * Handler for the timer interval select change event
 * @param {number} [value] the timer interval
 * @returns {void}
 */
function selectTimerIntervalChange(value) {
    if (value === undefined) {
        //change event from select itself
        value = Number(getSelectValueId('modalTimerIntervalInput'));
    }
    else {
        if (value !== -1 &&
            value !== 0)
        {
            //repeat
            elGetById('modalTimerIntervalInput').value = '-2';
        }
        else {
            //one shot
            elGetById('modalTimerIntervalInput').value = value;
        }
    }
    if (value !== -1 &&
        value !== 0)
    {
        //repeat
        elShowId('modalTimerIntervalRepeatGroup');
        if (value === -2) {
            //default interval is one day
            value = 86400;
        }
    }
    else {
        //one shot
        elHideId('modalTimerIntervalRepeatGroup');
    }

    const modalTimerIntervalRepeatInput = elGetById('modalTimerIntervalRepeatInput');
    const modalTimerIntervalRepeatUnit = elGetById('modalTimerIntervalRepeatUnit');
    for (const unit of [604800, 86400, 3600, 60, 1]) {
        if (value >= unit &&
            value % unit === 0)
        {
            modalTimerIntervalRepeatInput.value = value / unit;
            modalTimerIntervalRepeatUnit.value = unit;
            break;
        }
    }
}

/**
 * Handler for the timer action change event
 * @param {object} [values] argument values object
 * @returns {void}
 */
function selectTimerActionChange(values) {
    const el = elGetById('modalTimerActionInput');

    if (getSelectValue(el) === 'startplay') {
        elShowId('timerActionPlay');
        elHideId('modalTimerScriptActionGroup');
    }
    else if (el.selectedIndex > -1 &&
             getData(el.options[el.selectedIndex].parentNode, 'value') === 'script')
    {
        elShowId('modalTimerScriptActionGroup');
        elHideId('timerActionPlay');
        showTimerScriptArgs(el.options[el.selectedIndex], values);
    }
    else {
        elHideId('timerActionPlay');
        elHideId('modalTimerScriptActionGroup');
    }
}

/**
 * Shows the arguments for a timer script
 * @param {HTMLElement} optionEl the selected timer script option element
 * @param {object} values argument values object
 * @returns {void}
 */
function showTimerScriptArgs(optionEl, values) {
    if (values === undefined) {
        values = {};
    }
    const args = getData(optionEl, 'arguments');
    const list = elGetById('modalTimerScriptActionArguments');
    elClear(list);
    for (let i = 0, j = args.arguments.length; i < j; i++) {
        const input = elCreateEmpty('input', {"class": ["form-control"], "type": "text", "name": "modalTimerScriptActionArguments" + i,
            "value": (values[args.arguments[i]] ? values[args.arguments[i]] : '')});
        setData(input, 'name', args.arguments[i]);
        const fg = elCreateNodes('div', {"class": ["form-group", "row", "mb-3"]}, [
            elCreateText('label', {"class": ["col-sm-4", "col-form-label"], "for": "modalTimerScriptActionArguments" + i}, args.arguments[i]),
            elCreateNode('div', {"class": ["col-sm-8"]}, input)
        ]);
        list.appendChild(fg);
    }
    if (args.arguments.length === 0) {
        list.textContent = tn('No arguments');
    }
}

/**
 * Shows the list timer tab
 * @returns {void}
 */
function showListTimer() {
    cleanupModalId('modalTimer');
    elGetById('modalTimerListTab').classList.add('active');
    elGetById('modalTimerEditTab').classList.remove('active');
    elShowId('modalTimerListFooter');
    elHideId('modalTimerEditFooter');
    sendAPI("MYMPD_API_TIMER_LIST", {}, parseListTimer, true);
}

/**
 * Parses the MYMPD_API_TIMER_LIST jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parseListTimer(obj) {
    const table = elGetById('modalTimerList');
    const tbody = table.querySelector('tbody');
    if (checkResult(obj, table, 'table') === false) {
        return;
    }
    elClear(tbody);
    const weekdays = ['Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat', 'Sun'];
    for (let i = 0; i < obj.result.returnedEntities; i++) {
        const btn = elCreateEmpty('button', {"name": "enabled", "class": ["btn", "btn-secondary", "btn-xs", "mi", "mi-sm"]});
        if (obj.result.data[i].enabled === true) {
            btn.classList.add('active');
            btn.textContent = 'check';
        }
        else {
            btn.textContent = 'radio_button_unchecked';
        }

        const days = [];
        for (let j = 0; j < 7; j++) {
            if (obj.result.data[i].weekdays[j] === true) {
                days.push(tn(weekdays[j]));
            }
        }

        let interval = '';
        switch (obj.result.data[i].interval) {
            case -1: interval = tn('One shot and delete'); break;
            case 0: interval = tn('One shot and disable'); break;
            default:
                for (const unit of [604800, 86400, 3600, 60, 1]) {
                    if (obj.result.data[i].interval >= unit && obj.result.data[i].interval % unit === 0) {
                        interval = tn('Each ' + unit, {"smartCount": obj.result.data[i].interval / unit});
                        break;
                    }
                }
        }

        const row = elCreateNodes('tr', {"title": tn('Edit')}, [
            elCreateText('td', {}, obj.result.data[i].name),
            elCreateNode('td', {}, btn),
            elCreateText('td', {}, zeroPad(obj.result.data[i].startHour, 2) + ':' + zeroPad(obj.result.data[i].startMinute, 2) +
                ' ' + tn('on') + ' ' + days.join(', ')),
            elCreateText('td', {}, interval),
            elCreateText('td', {}, prettyTimerAction(obj.result.data[i].action, obj.result.data[i].subaction)),
            elCreateNode('td', {"data-col": "Action"},
                elCreateText('a', {"class": ["mi", "color-darkgrey"], "href": "#"}, 'delete')
            )
        ]);
        setData(row, 'id', obj.result.data[i].timerid);
        tbody.append(row);
    }
}

/**
 * Pretty prints the timer action
 * @param {string} action the action
 * @param {string} subaction the sub action
 * @returns {string} the translated action
 */
function prettyTimerAction(action, subaction) {
    if (action === 'player' &&
        subaction === 'startplay')
    {
        return tn('Start playback');
    }
    if (action === 'player' &&
        subaction === 'stopplay')
    {
        return tn('Stop playback');
    }
    if (action === 'script') {
        return tn('Script') + ': ' + subaction;
    }
    return action + ': ' + subaction;
}
