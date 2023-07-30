"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalTimer_js */

/**
 * Initialization function for the timer elements
 * @returns {void}
 */
function initModalTimer() {
    document.getElementById('listTimerList').addEventListener('click', function(event) {
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

    const selectTimerHourEl = document.getElementById('selectTimerHour');
    for (let i = 0; i < 24; i++) {
        selectTimerHourEl.appendChild(
            elCreateText('option', {"value": i}, zeroPad(i, 2))
        );
    }

    const selectTimerMinuteEl = document.getElementById('selectTimerMinute');
    for (let i = 0; i < 60; i = i + 5) {
        selectTimerMinuteEl.appendChild(
            elCreateText('option', {"value": i}, zeroPad(i, 2))
        );
    }

    document.getElementById('inputTimerVolume').addEventListener('change', function() {
        document.getElementById('textTimerVolume').textContent = this.value + ' %';
    }, false);

    document.getElementById('selectTimerAction').addEventListener('change', function() {
        selectTimerActionChange();
    }, false);

    document.getElementById('selectTimerInterval').addEventListener('change', function() {
        selectTimerIntervalChange();
    }, false);

    document.getElementById('modalTimer').addEventListener('shown.bs.modal', function () {
        showListTimer();
    });

    setDataId('selectTimerPlaylist', 'cb-filter', 'filterPlaylistsSelect');
    setDataId('selectTimerPlaylist', 'cb-filter-options', [0, 'selectTimerPlaylist']);
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
        }, saveTimerCheckError, true);
    });
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
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function saveTimer() {
    cleanupModalId('modalTimer');
    let formOK = true;
    const nameEl = document.getElementById('inputTimerName');
    if (!validateNotBlankEl(nameEl)) {
        formOK = false;
    }
    let minOneDay = false;
    const weekdayBtns = ['btnTimerMon', 'btnTimerTue', 'btnTimerWed', 'btnTimerThu', 'btnTimerFri', 'btnTimerSat', 'btnTimerSun'];
    const weekdays = [];
    for (let i = 0, j = weekdayBtns.length; i < j; i++) {
        const checked = document.getElementById(weekdayBtns[i]).classList.contains('active') ? true : false;
        weekdays.push(checked);
        if (checked === true) {
            minOneDay = true;
        }
    }
    if (minOneDay === false) {
        formOK = false;
        setIsInvalidId('btnTimerSun');
    }

    const selectTimerAction = document.getElementById('selectTimerAction');
    if (selectTimerAction.selectedIndex === -1) {
        formOK = false;
        setIsInvalid(selectTimerAction);
    }

    const inputTimerIntervalEl = document.getElementById('inputTimerInterval');
    if (!validateIntEl(inputTimerIntervalEl)) {
        formOK = false;
    }

    if (formOK === true) {
        const args = {};
        const argEls = document.querySelectorAll('#timerActionScriptArguments input');
        for (let i = 0, j = argEls.length; i < j; i++) {
            args[getData(argEls[i], 'name')] = argEls[i].value;
        }
        let interval = Number(getSelectValueId('selectTimerInterval'));
        if (interval === -2) {
            //repeat
            interval = Number(inputTimerIntervalEl.value);
            //convert interval to seconds
            const unit = Number(getSelectValueId('selectTimerIntervalUnit'));
            interval = interval * unit;
        }
        let preset = getSelectValueId('selectTimerPreset');
        if (preset === undefined) {
            //set to empty string, else the jsonrpc parameter is not set
            preset = '';
        }
        sendAPI("MYMPD_API_TIMER_SAVE", {
            "timerid": Number(document.getElementById('inputTimerId').value),
            "name": nameEl.value,
            "interval": interval,
            "enabled": (document.getElementById('btnTimerEnabled').classList.contains('active') ? true : false),
            "startHour": Number(getSelectValueId('selectTimerHour')),
            "startMinute": Number(getSelectValueId('selectTimerMinute')),
            "weekdays": weekdays,
            "action": getData(selectTimerAction.options[selectTimerAction.selectedIndex].parentNode, 'value'),
            "subaction": getSelectValue(selectTimerAction),
            "volume": Number(document.getElementById('inputTimerVolume').value),
            "playlist": getDataId('selectTimerPlaylist', 'value'),
            "preset": preset,
            "arguments": args
        }, saveTimerCheckError, true);
    }
}

/**
 * Handler for the MYMPD_API_TIMER_SAVE jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function saveTimerCheckError(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
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
    elHideId('timerActionScript');
    document.getElementById('listTimer').classList.remove('active');
    document.getElementById('editTimer').classList.add('active');
    elHideId('listTimerFooter');
    elShowId('editTimerFooter');
    document.getElementById('selectTimerPlaylist').filterInput.value = '';

    if (timerid !== 0) {
        sendAPI("MYMPD_API_TIMER_GET", {
            "timerid": timerid
        }, parseEditTimer, false);
    }
    else {
        filterPlaylistsSelect(0, 'selectTimerPlaylist', '', '');
        document.getElementById('inputTimerId').value = '0';
        document.getElementById('inputTimerName').value = '';
        toggleBtnChkId('btnTimerEnabled', true);
        document.getElementById('selectTimerHour').value = '12';
        document.getElementById('selectTimerMinute').value = '0';
        document.getElementById('selectTimerAction').value = 'startplay';
        document.getElementById('inputTimerVolume').value = '50';
        document.getElementById('textTimerVolume').textContent = '50 %';
        selectTimerIntervalChange(86400);
        selectTimerActionChange();
        const weekdayBtns = ['btnTimerMon', 'btnTimerTue', 'btnTimerWed', 'btnTimerThu', 'btnTimerFri', 'btnTimerSat', 'btnTimerSun'];
        for (let i = 0, j = weekdayBtns.length; i < j; i++) {
            toggleBtnId(weekdayBtns[i], false);
        }
        elShowId('timerActionPlay');
    }
    setFocusId('inputTimerName');
}

/**
 * Parses the MYMPD_API_TIMER_GET response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parseEditTimer(obj) {
    filterPlaylistsSelect(0, 'selectTimerPlaylist', '', obj.result.playlist);

    document.getElementById('inputTimerId').value = obj.result.timerid;
    document.getElementById('inputTimerName').value = obj.result.name;
    toggleBtnChkId('btnTimerEnabled', obj.result.enabled);
    document.getElementById('selectTimerHour').value = obj.result.startHour;
    document.getElementById('selectTimerMinute').value = obj.result.startMinute;
    document.getElementById('selectTimerAction').value = obj.result.subaction;
    selectTimerActionChange(obj.result.arguments);
    selectTimerIntervalChange(obj.result.interval);
    document.getElementById('inputTimerVolume').value = obj.result.volume;
    document.getElementById('textTimerVolume').textContent = obj.result.volume + ' %';
    document.getElementById('selectTimerPreset').value = obj.result.preset;

    const weekdayBtns = ['btnTimerMon', 'btnTimerTue', 'btnTimerWed', 'btnTimerThu', 'btnTimerFri', 'btnTimerSat', 'btnTimerSun'];
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
        value = Number(getSelectValueId('selectTimerInterval'));
    }
    else {
        if (value !== -1 &&
            value !== 0)
        {
            //repeat
            document.getElementById('selectTimerInterval').value = '-2';
        }
        else {
            //one shot
            document.getElementById('selectTimerInterval').value = value;
        }
    }
    if (value !== -1 &&
        value !== 0)
    {
        //repeat
        elShowId('groupTimerInterval');
        if (value === -2) {
            //default interval is one day
            value = 86400;
        }
    }
    else {
        //one shot
        elHideId('groupTimerInterval');
    }

    const inputTimerInterval = document.getElementById('inputTimerInterval');
    const selectTimerIntervalUnit = document.getElementById('selectTimerIntervalUnit');
    for (const unit of [604800, 86400, 3600, 60, 1]) {
        if (value >= unit &&
            value % unit === 0)
        {
            inputTimerInterval.value = value / unit;
            selectTimerIntervalUnit.value = unit;
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
    const el = document.getElementById('selectTimerAction');

    if (getSelectValue(el) === 'startplay') {
        elShowId('timerActionPlay');
        elHideId('timerActionScript');
    }
    else if (el.selectedIndex > -1 &&
             getData(el.options[el.selectedIndex].parentNode, 'value') === 'script')
    {
        elShowId('timerActionScript');
        elHideId('timerActionPlay');
        showTimerScriptArgs(el.options[el.selectedIndex], values);
    }
    else {
        elHideId('timerActionPlay');
        elHideId('timerActionScript');
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
    const list = document.getElementById('timerActionScriptArguments');
    elClear(list);
    for (let i = 0, j = args.arguments.length; i < j; i++) {
        const input = elCreateEmpty('input', {"class": ["form-control"], "type": "text", "name": "timerActionScriptArguments" + i,
            "value": (values[args.arguments[i]] ? values[args.arguments[i]] : '')});
        setData(input, 'name', args.arguments[i]);
        const fg = elCreateNodes('div', {"class": ["form-group", "row", "mb-3"]}, [
            elCreateText('label', {"class": ["col-sm-4", "col-form-label"], "for": "timerActionScriptArguments" + i}, args.arguments[i]),
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
    document.getElementById('listTimer').classList.add('active');
    document.getElementById('editTimer').classList.remove('active');
    elShowId('listTimerFooter');
    elHideId('editTimerFooter');
    sendAPI("MYMPD_API_TIMER_LIST", {}, parseListTimer, true);
}

/**
 * Parses the MYMPD_API_TIMER_LIST jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parseListTimer(obj) {
    const tbody = document.getElementById('listTimerList');
    if (checkResult(obj, tbody) === false) {
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
