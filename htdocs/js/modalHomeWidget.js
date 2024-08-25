"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalHomeWidget_js */

/**
 * Initializes the modalHomeWidget
 * @returns {void}
 */
function initModalHomeWidget() {
    elGetById('modalHomeWidgetScriptInput').addEventListener('change', function() {
        selectWidgetScriptChange();
    }, false);
}

/**
 * Opens the add to homescreen modal for widgets
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addHomeWidget() {
    const modal = elGetById('modalHomeWidget');
    elGetById('modalHomeWidgetTitle').textContent = tn('Add to homescreen');
    setData(modal, 'replace', false);
    setData(modal, 'oldpos', 0);
    elGetById('modalHomeWidgetNameInput').value = '';
    elGetById('modalHomeWidgetSizeInput').value = '2x2';
    elGetById('modalHomeWidgetScriptInput').value = '';
    selectWidgetScriptChange();
    //show modal
    cleanupModalId('modalHomeWidget');
    uiElements.modalHomeWidget.show();
}

/**
 * Shows the edit home widget dialog
 * @param {number} pos home icon position
 * @param {boolean} replace true = replace existing home widget, false = duplicate home widget
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function editHomeWidget(pos, replace) {
    elGetById('modalHomeWidgetTitle').textContent = replace === true ? 'Edit widget' : 'Duplicate widget';
    sendAPI("MYMPD_API_HOME_ICON_GET", {"pos": pos}, function(obj) {
        const modal = elGetById('modalHomeWidget');
        setData(modal, 'replace', replace);
        setData(modal, 'oldpos', pos);
        elGetById('modalHomeWidgetNameInput').value = obj.result.data.name;
        elGetById('modalHomeWidgetSizeInput').value = obj.result.data.size;
        elGetById('modalHomeWidgetScriptInput').value = obj.result.data.script;
        selectWidgetScriptChange(obj.result.data.arguments);
        //show modal
        cleanupModalId('modalHomeWidget');
        uiElements.modalHomeWidget.show();
    }, false);
}

/**
 * Calls showWidgetScriptArgs for the selected script
 * @param {object} [values] array of values for the script arguments
 * @returns {void}
 */
function selectWidgetScriptChange(values) {
    const el = elGetById('modalHomeWidgetScriptInput');
    if (el.selectedIndex > -1) {
        showWidgetScriptArgs(el.options[el.selectedIndex], values);
    }
}

/**
 * Shows the list of arguments and values for the selected script
 * @param {HTMLElement} option selected option from script select
 * @param {object} values Key/value pairs for values for the script arguments
 * @returns {void}
 */
function showWidgetScriptArgs(option, values) {
    if (values === undefined) {
        values = {};
    }
    const args = getData(option, 'arguments');
    const list = elGetById('modalHomeWidgetArgumentsInput');
    scriptArgsToForm(list, args.arguments, values);
    if (args.arguments.length === 0) {
        elClear(list);
        list.textContent = tn('No arguments');
    }
}

/**
 * Saves the home widget
 * @param {Element} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function saveHomeWidget(target) {
    cleanupModalId('modalHomeIcon');
    btnWaiting(target, true);
    const args = {};
    const argEls = document.querySelectorAll('#modalHomeWidgetArgumentsInput input');
    for (const argEl of argEls) {
        args[argEl.getAttribute('name')] = argEl.value;
    }
    const modal = elGetById('modalHomeWidget');
    sendAPI("MYMPD_API_HOME_WIDGET_SAVE", {
        "replace": getData(modal, 'replace'),
        "oldpos": getData(modal, 'oldpos'),
        "name": elGetById('modalHomeWidgetNameInput').value,
        "size": getSelectValueId('modalHomeWidgetSizeInput'),
        "script": getSelectValueId('modalHomeWidgetScriptInput'),
        "arguments": args
    }, modalClose, true);
}
