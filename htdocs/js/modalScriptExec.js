"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalScriptExec_js */

/**
 * Executes a script and uses a comma separated list of options as arguments
 * @param {string} cmd script to execute
 * @param {string} options options parsed as comma separated list of arguments
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function execScriptFromOptions(cmd, options) {
    const args = options[0] !== ''
        ? options[0].split(',')
        : [];
    execScript({"script": cmd, "arguments": args});
}

/**
 * Parses the script arguments definition and creates the form elements
 * @param {string} name Name of the element
 * @param {string} value Element value definition
 * @param {string} defaultValue Default value for the element
 * @returns {Element} Created element
 */
function parseArgument(name, value, defaultValue) {
    if (defaultValue === undefined) {
        defaultValue = '';
    }
    switch(value) {
        case 'text':
            return elCreateEmpty('input', {"name": name, "value": defaultValue, "type": "text", "class": ["form-control"]});
        case 'password':
            return elCreateEmpty('input', {"name": name, "value": defaultValue, "type": "password", "class": ["form-control"]});
        case 'checkbox': {
            const btn = elCreateText('button', {"name": name, "type": "button", "data-value": "true",
                "class": ["btn", "btn-sm", "btn-secondary", "mi", "chkBtn"]}, "radio_button_unchecked");
            if (defaultValue === 'true') {
                btn.classList.add('active');
                btn.textContent = 'check';
            }
            btn.addEventListener('click', function(event) {
                    toggleBtnChk(event.target, undefined);
                }, false);
            return btn;
        }
        // No default
    }
    const match = value.split(';');
    switch(match[0]) {
        case 'select': {
            const sel = elCreateEmpty('select', {"name": name, "class": ["form-select"]});
            for (let i = 1; i < match.length; i++) {
                sel.appendChild(
                    elCreateText('option', {"value": match[i]}, match[i])
                );
            }
            if (defaultValue !== '') {
                sel.value = defaultValue;
            }
            return sel;
        }
        case 'radio': {
            const radios = [];
            for (let i = 1; i < match.length; i++) {
                radios.push(
                    elCreateNodes('div', {"class": ["form-check"]}, [
                        elCreateEmpty('input', {"name": name, "value": match[i], "type": "radio", "class": ["form-check-input", "ms-0", "me-3"]}),
                        elCreateText('label', {"class": ["form-check-label"]}, match[i])
                    ])
                );
                if (defaultValue === match[i]) {
                    radios[radios.length - 1].querySelector('input').setAttribute('checked', 'checked');
                }
            }
            return elCreateNodes('div', {"data-name": name, "data-type": "radios"}, radios);
        }
        // No default
    }
    return elCreateEmpty('div', {});
}

/**
 * Creates the form to ask for script arguments
 * @param {Element} container Element to append the form elements
 * @param {Array} scriptArguments Array of script arguments
 * @param {object} values The default values for the form elements
 * @returns {void}
 */
function scriptArgsToForm(container, scriptArguments, values) {
    elClear(container);
    for (let i = 0, j = scriptArguments.length; i < j; i++) {
        const match = scriptArguments[i].split('|');
        const label = match.length === 1
            ? scriptArguments[i]
            : match[0];
        const value = values[label];
        const input = match.length === 1
            ? parseArgument(scriptArguments[i], 'text', value)
            : parseArgument(match[0], match[1], value);
        container.appendChild(
            elCreateNodes('div', {"class": ["form-group", "row", "mb-3"]}, [
                elCreateText('label', {"class": ["col-sm-4", "col-form-label"]}, label),
                elCreateNode('div', {"class": ["col-sm-8"]}, input)
            ])
        );
    }
}

/**
 * Executes a script and asks for argument values
 * @param {object} cmd script and arguments object to execute
 * @returns {void}
 */
function execScript(cmd) {
    if (cmd.arguments.length === 0) {
        sendAPI("MYMPD_API_SCRIPT_EXECUTE", {
            "script": cmd.script,
            "event": "user",
            "arguments": {}
        }, null, false);
    }
    else {
        scriptArgsToForm(elGetById('modalScriptExecArgumentsList'), cmd.arguments);
        elGetById('modalScriptExecScriptnameInput').value = cmd.script;
        uiElements.modalScriptExec.show();
    }
}

/**
 * Executes a script after asking for argument values
 * @param {Element} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function execScriptArgs(target) {
    const script = elGetById('modalScriptExecScriptnameInput').value;
    const args = formToScriptArgs(elGetById('modalScriptExecArgumentsList'));
    sendAPI("MYMPD_API_SCRIPT_EXECUTE", {
        "script": script,
        "event": "user",
        "arguments": args
    }, modalClose, true);
}

/**
 * Returns the form elements names and values for script arguments
 * @param {Element} container Element with the form elements
 * @returns {object} Object with form element names and values
 */
function formToScriptArgs(container) {
    const args = {};
    const inputs = container.querySelectorAll('input');
    for (let i = 0, j = inputs.length; i < j; i++) {
        if (inputs[i].type === 'radio') {
            continue;
        }
        args[inputs[i].name] = inputs[i].value;
    }
    const selects = container.querySelectorAll('select');
    for (let i = 0, j = selects.length; i < j; i++) {
        args[selects[i].name] = getSelectValue(selects[i]);
    }
    const btns = container.querySelectorAll('button');
    for (let i = 0, j = btns.length; i < j; i++) {
        args[btns[i].name] = getBtnChkValue(btns[i]) === true ? 'true' : 'false';
    }
    const radios = container.querySelectorAll('[data-type=radios]');
    for (let i = 0, j = radios.length; i < j; i++) {
        args[radios[i].getAttribute('data-name')] = getRadioBoxValue(radios[i]);
    }
    return args;
}
