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
        case 'hidden':
        case 'text':
        case 'password':
        case 'checkbox':
            return createScriptDialogEl({"type": value, "name": name, "value": defaultValue});
        // No default
    }
    const match = value.split(';');
    const values = match.slice(1);
    switch(match[0]) {
        case 'select':
        case 'radio':
        case 'list':
            return createScriptDialogEl({"type": match[0], "name": name, "value": values, "defaultValue": defaultValue});
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
        const value = values !== undefined
            ? values[label] !== undefined
                ? values[label]
                : ''
            : '';
        if (match.length === 1) {
            match.push('text');
        }
        const input = parseArgument(match[0], match[1], value);
        if (match[1] === 'hidden') {
            container.appendChild(input);
        }
        else {
            container.appendChild(
                elCreateNodes('div', {"class": ["form-group", "row", "mb-3"]}, [
                    elCreateText('label', {"class": ["col-sm-4", "col-form-label"]}, label),
                    elCreateNode('div', {"class": ["col-sm-8"]}, input)
                ])
            );
        }
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
        elGetById('modalScriptExecTitle').textContent = tn('Execute script');
        uiElements.modalScriptExec.show();
    }
}

/**
 * Parses the script dialog definition and creates the form elements
 * @param {object} data Definition of form element to create
 * @returns {Element} Created element
 */
function createScriptDialogEl(data) {
    if (data.defaultValue === undefined) {
        data.defaultValue = data.value;
    }
    switch(data.type) {
        case 'text':
        case 'password':
        case 'hidden':
            return elCreateEmpty('input', {"name": data.name, "value": data.value, "type": data.type, "class": ["form-control"]});
        case 'checkbox': {
            const btn = elCreateText('button', {"name": data.name, "type": "button", "data-value": "true",
                "class": ["btn", "btn-sm", "btn-secondary", "mi", "chkBtn"]}, "radio_button_unchecked");
            if (data.value === 'true') {
                btn.classList.add('active');
                btn.textContent = 'check';
            }
            btn.addEventListener('click', function(event) {
                    toggleBtnChk(event.target, undefined);
                }, false);
            return btn;
        }
        case 'select': {
            const sel = elCreateEmpty('select', {"name": data.name, "class": ["form-select"]});
            for (let i = 0; i < data.value.length; i++) {
                const title = data.displayValue && data.displayValue[i]
                    ? data.displayValue[i]
                    : data.value[i];
                sel.appendChild(
                    elCreateText('option', {"value": data.value[i]}, title)
                );
            }
            if (data.defaultValue !== '') {
                sel.value = data.defaultValue;
            }
            return sel;
        }
        case 'radio': {
            const radios = [];
            for (let i = 0; i < data.value.length; i++) {
                const title = data.displayValue && data.displayValue[i]
                    ? data.displayValue[i]
                    : data.value[i];
                radios.push(
                    elCreateNodes('div', {"class": ["form-check"]}, [
                        elCreateEmpty('input', {"name": data.name, "value": data.value[i], "type": "radio", "class": ["form-check-input", "ms-0", "me-3"]}),
                        elCreateText('label', {"class": ["form-check-label"]}, title)
                    ])
                );
                if (data.defaultValue === data.value[i]) {
                    radios[radios.length - 1].querySelector('input').setAttribute('checked', 'checked');
                }
            }
            return elCreateNodes('div', {"data-name": data.name, "data-type": "radios"}, radios);
        }
        case 'list': {
            const rows = [];
            for (let i = 0; i < data.value.length; i++) {
                const title = data.displayValue && data.displayValue[i].title
                    ? data.displayValue[i].title
                    : data.value[i];
                const lines = [];
                let hasDesc = false;
                if (data.displayValue && data.displayValue[i].text) {
                    lines.push(elCreateText('p', {"class": ["mb-1"]}, data.displayValue[i].text));
                    hasDesc = true;
                }
                if (data.displayValue && data.displayValue[i].small) {
                    lines.push(elCreateText('small', {}, data.displayValue[i].small));
                    hasDesc = true;
                }
                rows.push(
                    elCreateNodes('li', {"data-value": data.value[i], "class": ["list-group-item", "clickable"]}, [
                        elCreateNodes('div', {"class": ["d-flex", "justify-content-between", "align-items-start"]}, [
                            elCreateText((hasDesc === true ? 'h5' : 'p'), {"class": ["mb-1"]}, title),
                            pEl.selectBtn.cloneNode(true),
                        ]),
                        ... lines
                    ])
                );
            }
            const lg = elCreateNodes('ul', {"data-name": data.name, "data-type": "list", "class": ["list-group"]}, rows);
            lg.addEventListener('click', function(event) {
                event.preventDefault();
                const target = event.target.nodeName === 'LI'
                    ? event.target
                    : event.target.closest('li');
                const active = target.classList.contains('active');
                if (active === false) {
                    target.classList.add('active');
                    target.querySelector('button').textContent = ligatures.checked;
                }
                else {
                    target.classList.remove('active');
                    target.querySelector('button').textContent = ligatures.unchecked;
                }
            }, false);
            return lg;
        }
        // No default
    }
    return elCreateEmpty('div', {});
}

/**
 * Creates the form for the script dialog
 * @param {Element} container Element to append the form elements
 * @param {object} data Definition of form elements
 * @returns {void}
 */
function scriptDialogToForm(container, data) {
    elClear(container);
    for (let i = 0, j = data.length; i < j; i++) {
        const input = createScriptDialogEl(data[i]);
        if (data[i].type === 'hidden') {
            container.appendChild(input);
        }
        else {
            container.appendChild(
                elCreateNodes('div', {"class": ["form-group", "row", "mb-3"]}, [
                    elCreateText('label', {"class": ["col-sm-4", "col-form-label"]}, data[i].name),
                    elCreateNode('div', {"class": ["col-sm-8"]}, input)
                ])
            );
        }
    }
}

/**
 * Shows the script dialog and asks for input
 * @param {object} params Jsonrpc params from script_dialog method
 * @returns {void}
 */
function showScriptDialog(params) {
    scriptDialogToForm(elGetById('modalScriptExecArgumentsList'), params.data);
    elGetById('modalScriptExecScriptnameInput').value = params.callback;
    elGetById('modalScriptExecTitle').textContent = params.message;
    uiElements.modalScriptExec.show();
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
    uiElements.modalScriptExec.hide();
    // The hide function requires some time, wait before executing the script.
    // Workaround for scripts that respond with a dialog.
    setTimeout(function() {
        sendAPI("MYMPD_API_SCRIPT_EXECUTE", {
            "script": script,
            "event": "user",
            "arguments": args
        }, null, true);
    }, 250);
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
    const btns = container.querySelectorAll('.chkBtn');
    for (let i = 0, j = btns.length; i < j; i++) {
        args[btns[i].name] = getBtnChkValue(btns[i]) === true ? 'true' : 'false';
    }
    const radios = container.querySelectorAll('[data-type=radios]');
    for (let i = 0, j = radios.length; i < j; i++) {
        args[radios[i].getAttribute('data-name')] = getRadioBoxValue(radios[i]);
    }
    const lists = container.querySelectorAll('[data-type=list]');
    for (let i = 0, j = lists.length; i < j; i++) {
        const items = [];
        for (const el of lists[i].querySelectorAll('.active')) {
            items.push(el.getAttribute('data-value'));
        }
        args[lists[i].getAttribute('data-name')] = items.join(';;');
    }
    return args;
}
