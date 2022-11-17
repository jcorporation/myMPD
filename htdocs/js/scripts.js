"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module scripts_js */

/**
 * Initialization functions for the script elements
 */
function initScripts() {
    document.getElementById('inputScriptArgument').addEventListener('keyup', function(event) {
        if (event.key === 'Enter') {
            event.preventDefault();
            event.stopPropagation();
            addScriptArgument();
        }
    }, false);

    document.getElementById('selectScriptArguments').addEventListener('click', function(event) {
        if (event.target.nodeName === 'OPTION') {
            removeScriptArgument(event);
            event.stopPropagation();
        }
    }, false);

    document.getElementById('listScriptsList').addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (event.target.nodeName === 'A') {
            const action = getData(event.target, 'action');
            const script = getData(event.target.parentNode.parentNode, 'script');
            switch(action) {
                case 'delete':
                    deleteScript(event.target, script);
                    break;
                case 'execute':
                    execScript(getData(event.target.parentNode.parentNode, 'href'));
                    break;
                case 'add2home':
                    addScriptToHome(script, getData(event.target.parentNode.parentNode, 'href'));
                    break;
            }
            return;
        }

        const target = getParent(event.target, 'TR');
        if (checkTargetClick(target) === true) {
            showEditScript(getData(target, 'script'));
        }
    }, false);

    document.getElementById('modalScripts').addEventListener('shown.bs.modal', function () {
        showListScripts();
    }, false);

    document.getElementById('btnDropdownAddAPIcall').parentNode.addEventListener('show.bs.dropdown', function() {
        const dw = document.getElementById('textareaScriptContent').offsetWidth - document.getElementById('btnDropdownAddAPIcall').parentNode.offsetLeft;
        document.getElementById('dropdownAddAPIcall').style.width = dw + 'px';
    }, false);

    document.getElementById('btnDropdownAddFunction').parentNode.addEventListener('show.bs.dropdown', function() {
        const dw = document.getElementById('textareaScriptContent').offsetWidth - document.getElementById('btnDropdownAddFunction').parentNode.offsetLeft;
        document.getElementById('dropdownAddFunction').style.width = dw + 'px';
    }, false);

    document.getElementById('btnDropdownImportScript').parentNode.addEventListener('show.bs.dropdown', function() {
        const dw = document.getElementById('textareaScriptContent').offsetWidth - document.getElementById('btnDropdownImportScript').parentNode.offsetLeft;
        document.getElementById('dropdownImportScript').style.width = dw + 'px';
        getImportScriptList();
    }, false);

    document.getElementById('btnImportScript').addEventListener('click', function(event) {
        event.preventDefault();
        event.stopPropagation();
        const script = getSelectValueId('selectImportScript');
        if (script === '') {
            return;
        }
        getImportScript(script);
        BSN.Dropdown.getInstance(document.getElementById('btnDropdownImportScript')).hide();
        setFocusId('textareaScriptContent');
    }, false);

    const selectAPIcallEl = document.getElementById('selectAPIcall');
    elClear(selectAPIcallEl);
    selectAPIcallEl.appendChild(
        elCreateTextTn('option', {"value": ""}, 'Select method')
    );
    for (const m in APImethods) {
        selectAPIcallEl.appendChild(
            elCreateText('option', {"value": m}, m)
        );
    }

    selectAPIcallEl.addEventListener('click', function(event) {
        event.stopPropagation();
    }, false);

    selectAPIcallEl.addEventListener('change', function(event) {
        const value = getSelectValue(event.target);
        document.getElementById('APIdesc').textContent = value !== '' ? APImethods[value].desc : '';
    }, false);

    document.getElementById('btnAddAPIcall').addEventListener('click', function(event) {
        event.preventDefault();
        event.stopPropagation();
        const method = getSelectValueId('selectAPIcall');
        if (method === '') {
            return;
        }
        const el = document.getElementById('textareaScriptContent');
        const [start, end] = [el.selectionStart, el.selectionEnd];
        const newText = 'rc, result = mympd.api("' + method + '", ' +
            apiParamsToArgs(APImethods[method].params) +
            ')\n' +
            'if rc == 0 then\n' +
            '\n' +
            'end\n';
        el.setRangeText(newText, start, end, 'preserve');
        BSN.Dropdown.getInstance(document.getElementById('btnDropdownAddAPIcall')).hide();
        setFocus(el);
    }, false);

    const selectFunctionEl = document.getElementById('selectFunction');
    elClear(selectFunctionEl);
    selectFunctionEl.appendChild(
        elCreateTextTn('option', {"value": ""}, 'Select function')
    );
    for (const m in LUAfunctions) {
        selectFunctionEl.appendChild(
            elCreateText('option', {"value": m}, m)
        );
    }

    selectFunctionEl.addEventListener('click', function(event) {
        event.stopPropagation();
    }, false);

    selectFunctionEl.addEventListener('change', function(event) {
        const value = getSelectValue(event.target);
        document.getElementById('functionDesc').textContent = value !== '' ? LUAfunctions[value].desc : '';
    }, false);

    document.getElementById('btnAddFunction').addEventListener('click', function(event) {
        event.preventDefault();
        event.stopPropagation();
        const value = getSelectValueId('selectFunction');
        if (value === '') {
            return;
        }
        const el = document.getElementById('textareaScriptContent');
        const [start, end] = [el.selectionStart, el.selectionEnd];
        el.setRangeText(LUAfunctions[value].func, start, end, 'end');
        BSN.Dropdown.getInstance(document.getElementById('btnDropdownAddFunction')).hide();
        setFocus(el);
    }, false);
}

/**
 * Fetches the list of available scripts to import
 */
function getImportScriptList() {
    const sel = document.getElementById('selectImportScript');
    sel.setAttribute('disabled', 'disabled');
    httpGet(subdir + '/proxy?uri=' + myEncodeURI('https://jcorporation.github.io/myMPD/scripting/scripts/index.json'), function(obj) {
        sel.options.length = 0;
        for (const script of obj.scripts) {
            sel.appendChild(
                elCreateText('option', {"value": script}, script)
            );
        }
        sel.removeAttribute('disabled');
    }, true);
}

/**
 * Imports a script
 * @param {string} script script to import
 */
function getImportScript(script) {
    document.getElementById('textareaScriptContent').setAttribute('disabled', 'disabled');
    httpGet(subdir + '/proxy?uri=' + myEncodeURI('https://jcorporation.github.io/myMPD/scripting/scripts/' + script), function(text) {
        const lines = text.split('\n');
        const firstLine = lines.shift();
        let obj;
        try {
            obj = JSON.parse(firstLine.substring(firstLine.indexOf('{')));
        }
        catch(error) {
            showNotification(tn('Can not parse script arguments'), '', 'general', 'error');
            logError('Can not parse script arguments:' + firstLine);
            return;
        }
        const scriptArgEl = document.getElementById('selectScriptArguments');
        scriptArgEl.options.length = 0;
        for (let i = 0, j = obj.arguments.length; i < j; i++) {
            scriptArgEl.appendChild(
                elCreateText('option', {}, obj.arguments[i])
            );
        }
        document.getElementById('textareaScriptContent').value = lines.join('\n');
        document.getElementById('textareaScriptContent').removeAttribute('disabled');
    }, false);
}

/**
 * Adds the documented api params to a lua string for the add api call function
 * @param {object} p parameters object
 * @returns {string} lua string
 */
function apiParamsToArgs(p) {
    let args = '{';
    let i = 0;
    for (const param in p) {
        if (i > 0) {
            args += ', ';
        }
        i++;
        args += param + ' = ';
        if (p[param].params !== undefined) {
            args += apiParamsToArgs(p[param].params);
        }
        else {
            if (p[param].type === 'text') {
                args += '"' + p[param].example + '"';
            }
            else {
                args += p[param].example;
            }
        }
    }
    args += '}';
    return args;
}

/**
 * Saves a script
 */
//eslint-disable-next-line no-unused-vars
function saveScript() {
    cleanupModalId('modalScripts');
    let formOK = true;

    const nameEl = document.getElementById('inputScriptName');
    if (!validatePlistEl(nameEl)) {
        formOK = false;
    }

    const orderEl = document.getElementById('inputScriptOrder');
    if (!validateIntEl(orderEl)) {
        formOK = false;
    }

    if (formOK === true) {
        const args = [];
        const argSel = document.getElementById('selectScriptArguments');
        for (let i = 0, j = argSel.options.length; i < j; i++) {
            args.push(argSel.options[i].text);
        }
        sendAPI("MYMPD_API_SCRIPT_SAVE", {
            "oldscript": document.getElementById('inputOldScriptName').value,
            "script": nameEl.value,
            "order": Number(orderEl.value),
            "content": document.getElementById('textareaScriptContent').value,
            "arguments": args
        }, saveScriptCheckError, true);
    }
}

/**
 * Handler for the MYMPD_API_SCRIPT_SAVE jsonrpc response
 * @param {object} obj jsonrpc response
 */
function saveScriptCheckError(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        showListScripts();
    }
}

/**
 * Appends an argument to the list of script arguments
 */
function addScriptArgument() {
    const el = document.getElementById('inputScriptArgument');
    if (validatePrintableEl(el)) {
        document.getElementById('selectScriptArguments').appendChild(
            elCreateText('option', {}, el.value)
        );
        el.value = '';
    }
}

/**
 * Removes an argument from the list of script arguments
 * @param {Event} ev triggering element
 */
function removeScriptArgument(ev) {
    const el = document.getElementById('inputScriptArgument');
    // @ts-ignore
    el.value = ev.target.text;
    ev.target.remove();
    setFocus(el);
}

/**
 * Shows the edit script tab
 * @param {string} script script name
 */
//eslint-disable-next-line no-unused-vars
function showEditScript(script) {
    cleanupModalId('modalScripts');
    document.getElementById('textareaScriptContent').removeAttribute('disabled');
    document.getElementById('listScripts').classList.remove('active');
    document.getElementById('editScript').classList.add('active');
    elHideId('listScriptsFooter');
    elShowId('editScriptFooter');

    if (script !== '') {
        sendAPI("MYMPD_API_SCRIPT_GET", {"script": script}, parseEditScript, false);
    }
    else {
        document.getElementById('inputOldScriptName').value = '';
        document.getElementById('inputScriptName').value = '';
        document.getElementById('inputScriptOrder').value = '1';
        document.getElementById('inputScriptArgument').value = '';
        elClearId('selectScriptArguments');
        document.getElementById('textareaScriptContent').value = '';
    }
    setFocusId('inputScriptName');
}

/**
 * Parses the MYMPD_API_SCRIPT_GET jsonrpc response
 * @param {object} obj jsonrpc response
 */
function parseEditScript(obj) {
    document.getElementById('inputOldScriptName').value = obj.result.script;
    document.getElementById('inputScriptName').value = obj.result.script;
    document.getElementById('inputScriptOrder').value = obj.result.metadata.order;
    document.getElementById('inputScriptArgument').value = '';
    const selSA = document.getElementById('selectScriptArguments');
    selSA.options.length = 0;
    for (let i = 0, j = obj.result.metadata.arguments.length; i < j; i++) {
        selSA.appendChild(
            elCreateText('option', {}, obj.result.metadata.arguments[i])
        );
    }
    document.getElementById('textareaScriptContent').value = obj.result.content;
}

/**
 * Shows the list scripts tab
 */
function showListScripts() {
    cleanupModalId('modalScripts');
    document.getElementById('listScripts').classList.add('active');
    document.getElementById('editScript').classList.remove('active');
    elShowId('listScriptsFooter');
    elHideId('editScriptFooter');
    getScriptList(true);
}

/**
 * Deletes a script after confirmation
 * @param {EventTarget} el triggering element
 * @param {string} script script to delete
 */
function deleteScript(el, script) {
    showConfirmInline(el.parentNode.previousSibling, tn('Do you really want to delete the script?', {"script": script}), tn('Yes, delete it'), function() {
        sendAPI("MYMPD_API_SCRIPT_RM", {
            "script": script
        }, deleteScriptCheckError, true);
    });
}

/**
 * Handler for the MYMPD_API_SCRIPT_RM jsonrpc response
 * @param {object} obj jsonrpc response
 */
function deleteScriptCheckError(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        getScriptList(true);
    }
}

/**
 * Gets the list of scripts
 * @param {boolean} all true = get all scripts, false = get all scripts with pos > 0
 */
function getScriptList(all) {
    sendAPI("MYMPD_API_SCRIPT_LIST", {
        "all": all
    }, parseScriptList, true);
}

/**
 * Parses the MYMPD_API_SCRIPT_LIST jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parseScriptList(obj) {
    const tbodyScripts = document.getElementById('listScriptsList');
    elClear(tbodyScripts);
    const mainmenuScripts = document.getElementById('scripts');
    elClear(mainmenuScripts);
    const triggerScripts = document.getElementById('selectTriggerScript');
    elClear(triggerScripts);

    if (checkResult(obj, tbodyScripts) === false) {
        return;
    }

    const timerActions = elCreateEmpty('optgroup', {"id": "timerActionsScriptsOptGroup", "label": tn('Script')});
    setData(timerActions, 'value', 'script');
    const scriptMaxListLen = 4;
    const scriptListLen = obj.result.data.length;
    let showScriptListLen = 0;
    if (scriptListLen > 0) {
        obj.result.data.sort(function(a, b) {
            return a.metadata.order - b.metadata.order;
        });
        for (let i = 0; i < scriptListLen; i++) {
            //scriptlist in main menu
            if (obj.result.data[i].metadata.order > 0) {
                showScriptListLen++;
                const a = elCreateText('a', {"class": ["dropdown-item", "alwaysEnabled"], "href": "#"}, obj.result.data[i].name);
                setData(a, 'href', {"script": obj.result.data[i].name, "arguments": obj.result.data[i].metadata.arguments});
                mainmenuScripts.appendChild(a);
            }
            //scriptlist in scripts modal
            const tr = elCreateNodes('tr', {}, [
                elCreateText('td', {}, obj.result.data[i].name),
                elCreateNodes('td', {"data-col": "Action"}, [
                    elCreateText('a', {"href": "#", "data-title-phrase": "Delete", "data-action": "delete", "class": ["me-2", "mi", "color-darkgrey"]}, 'delete'),
                    elCreateText('a', {"href": "#", "data-title-phrase": "Execute", "data-action": "execute", "class": ["me-2", "mi", "color-darkgrey"]}, 'play_arrow'),
                    elCreateText('a', {"href": "#", "data-title-phrase": "Add to homescreen", "data-action": "add2home", "class": ["me-2", "mi", "color-darkgrey"]}, 'add_to_home_screen')
                ])
            ]);
            setData(tr, 'script', obj.result.data[i].name);
            setData(tr, 'href', {"script": obj.result.data[i].name, "arguments": obj.result.data[i].metadata.arguments});
            tbodyScripts.appendChild(tr);

            //scriptlist select for timers
            const option = elCreateText('option', {"value": obj.result.data[i].name}, obj.result.data[i].name);
            setData(option, 'arguments', {"arguments": obj.result.data[i].metadata.arguments});
            timerActions.appendChild(option);
            //scriptlist select for trigger
            const option2 = option.cloneNode(true);
            setData(option2, 'arguments', {"arguments": obj.result.data[i].metadata.arguments});
            triggerScripts.appendChild(option2);
        }
    }

    const navScripting = document.getElementById('navScripting');
    if (showScriptListLen > scriptMaxListLen) {
        elShow(navScripting);
        elHide(navScripting.previousElementSibling);
        document.getElementById('scripts').classList.add('collapse', 'menu-indent');
    }
    else {
        elHide(navScripting);
        if (showScriptListLen === 0) {
            elHide(navScripting.previousElementSibling);
        }
        else {
            elShow(navScripting.previousElementSibling);
        }
        document.getElementById('scripts').classList.remove('collapse', 'menu-indent');
    }
    //update timer actions select
    const old = document.getElementById('timerActionsScriptsOptGroup');
    if (old) {
        old.replaceWith(timerActions);
    }
    else {
        document.getElementById('selectTimerAction').appendChild(timerActions);
    }
}

/**
 * Executes a script and uses a comma separated list of options as arguments
 * @param {string} cmd script to execute
 * @param {string} options options parsed as comma separated list of arguments
 */
//eslint-disable-next-line no-unused-vars
function execScriptFromOptions(cmd, options) {
    const args = options !== undefined && options !== '' ? options.split(',') : [];
    execScript({"script": cmd, "arguments": args});
}

/**
 * Executes a script and asks for argument values
 * @param {object} cmd script and arguments object to execute
 */
function execScript(cmd) {
    if (cmd.arguments.length === 0) {
        sendAPI("MYMPD_API_SCRIPT_EXECUTE", {
            "script": cmd.script,
            "arguments": {}
        }, null, false);
    }
    else {
        const arglist = document.getElementById('execScriptArguments');
        elClear(arglist);
        for (let i = 0, j = cmd.arguments.length; i < j; i++) {
            arglist.appendChild(
                elCreateNodes('div', {"class": ["form-group", "row", "mb-3"]}, [
                    elCreateText('label', {"class": ["col-sm-4", "col-form-label"]}, cmd.arguments[i]),
                    elCreateNode('div', {"class": ["col-sm-8"]},
                        elCreateEmpty('input', {"name": cmd.arguments[i], "id": "inputScriptArg" + i, "type": "text", "class": ["form-control"]})
                    )
                ])
            );
        }
        document.getElementById('modalExecScriptScriptname').value = cmd.script;
        uiElements.modalExecScript.show();
    }
}

/**
 * Executes a script after asking for argument values
 */
//eslint-disable-next-line no-unused-vars
function execScriptArgs() {
    const script = document.getElementById('modalExecScriptScriptname').value;
    const args = {};
    const inputs = document.querySelectorAll('#execScriptArguments input');
    for (let i = 0, j = inputs.length; i < j; i++) {
        args[inputs[i].name] = inputs[i].value;
    }
    sendAPI("MYMPD_API_SCRIPT_EXECUTE", {
        "script": script,
        "arguments": args
    }, null, false);
    uiElements.modalExecScript.hide();
}
