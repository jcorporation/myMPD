"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalScripts_js */

/**
 * Initialization functions for the script elements
 * @returns {void}
 */
function initModalScripts() {
    elGetById('modalScriptsAddArgumentInput').addEventListener('keyup', function(event) {
        if (event.key === 'Enter') {
            event.preventDefault();
            event.stopPropagation();
            addScriptArgument();
        }
    }, false);

    elGetById('modalScriptsArgumentsInput').addEventListener('click', function(event) {
        if (event.target.nodeName === 'OPTION') {
            removeScriptArgument(event);
            event.stopPropagation();
        }
    }, false);

    elGetById('modalScriptsList').addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        const target = event.target.closest('TR');
        if (event.target.nodeName === 'A') {
            const action = getData(event.target, 'action');
            const script = getData(target, 'script');
            switch(action) {
                case 'delete':
                    deleteScript(event.target, script);
                    break;
                case 'execute':
                    execScript(getData(target, 'href'));
                    break;
                case 'add2home':
                    addScriptToHome(script, getData(target, 'href'));
                    break;
                default:
                    logError('Invalid action: ' + action);
            }
            return;
        }
        if (checkTargetClick(target) === true) {
            showEditScript(getData(target, 'script'));
        }
    }, false);

    elGetById('modalScriptsAddAPIcallBtn').parentNode.addEventListener('show.bs.dropdown', function() {
        const dw = elGetById('modalScriptsContentInput').offsetWidth - elGetById('modalScriptsAddAPIcallBtn').parentNode.offsetLeft;
        elGetById('modalScriptsAddAPIcallDropdown').style.width = dw + 'px';
    }, false);

    elGetById('modalScriptsAddFunctionBtn').parentNode.addEventListener('show.bs.dropdown', function() {
        const dw = elGetById('modalScriptsContentInput').offsetWidth - elGetById('modalScriptsAddFunctionBtn').parentNode.offsetLeft;
        elGetById('modalScriptsAddFunctionDropdown').style.width = dw + 'px';
    }, false);

    const modalScriptsAPIcallSelectEl = elGetById('modalScriptsAPIcallSelect');
    elClear(modalScriptsAPIcallSelectEl);
    modalScriptsAPIcallSelectEl.appendChild(
        elCreateTextTn('option', {"value": ""}, 'Select method')
    );
    for (const m of Object.keys(APImethods).sort()) {
        modalScriptsAPIcallSelectEl.appendChild(
            elCreateText('option', {"value": m}, m)
        );
    }

    modalScriptsAPIcallSelectEl.addEventListener('change', function(event) {
        const value = getSelectValue(event.target);
        elGetById('modalScriptsAPIdesc').textContent = value !== '' ? APImethods[value].desc : '';
    }, false);
  
    const modalScriptsFunctionSelectEl = elGetById('modalScriptsFunctionSelect');
    modalScriptsFunctionSelectEl.addEventListener('change', function(event) {
        const value = getSelectValue(event.target);
        elGetById('modalScriptsFunctionDesc').textContent = value !== '' ? LUAfunctions[value].desc : '';
    }, false);

    elGetById('modalScriptsImportList').addEventListener('click', function(event) {
        const target = event.target.nodeName === 'li'
            ? event.target
            : event.target.closest('li');
        if (event.target.nodeName === 'A') {
            return;
        }
        if (target !== null) {
            importScript(target);
        }
    }, false);

    initSearchSimpleInput(elGetById('modalScriptsImportSearchStr'), function(value) {
        const valueLwr = value.toLowerCase();
        const items = document.querySelectorAll('#modalScriptsImportList > li');
        for (const item of items) {
            if (item.textContent.toLowerCase().indexOf(valueLwr) > -1) {
                item.classList.remove('d-none');
            }
            else {
                item.classList.add('d-none');
            }
        }
    });

    initModalScriptAPItab();
}

/**
 * Initialization functions for the script api tab elements
 * @returns {void}
 */
function initModalScriptAPItab() {
    const modalScriptAPIcmdsInput = elGetById('modalScriptAPIcmdsInput');
    modalScriptAPIcmdsInput.appendChild(elCreateEmpty('option',{}));
    const methods = Object.keys(APImethods).sort();
    for (const method of methods) {
        modalScriptAPIcmdsInput.appendChild(elCreateText('option',{'value': method}, method));
    }
    modalScriptAPIcmdsInput.addEventListener('change', function() {
        document.getElementById('modalScriptAPIparams').textContent = '';
        const method = this.options[this.selectedIndex].value;
        if (method !== '' && APImethods[method].params !== undefined) {
            elGetById('modalScriptAPIparams').appendChild(exploreAPIparamsToForm(APImethods[method].params, ''));
            document.getElementById('modalScriptAPIdesc').textContent = APImethods[method].desc;
        }
        else {
            document.getElementById('modalScriptAPIdesc').textContent = '';
        }
        document.getElementById('modalScriptAPIresultState').textContent = tn('Result');
        document.getElementById('modalScriptAPIresultText').textContent = '';
        elHideId('modalScriptAPIresult');
    }, false);
    document.getElementById('modalScriptAPISubmitBtn').addEventListener('click', function(event) {
        event.preventDefault();
        exploreAPIsendRequest();
    }, false);
}

/**
 * Creates a HTML form from an jsonrpc param object
 * @param {object} p Jsonrpc params object
 * @param {string} k Parent key
 * @returns {HTMLElement} HTML form
 */
function exploreAPIparamsToForm(p, k) {
    const form = elCreateEmpty('div', {});
    for (const param in p) {
        if (p[param].params !== undefined) {
            form.appendChild(
                elCreateNodes('div', {'class': ['form-group', 'row']}, [
                    elCreateText('label', {'class': ['col-sm-4', 'col-form-label']}, param),
                    elCreateNode('col', {'class': ['col-sm-8']}, exploreAPIparamsToForm(p[param].params, param))
                ])
            );
        }
        else {
            form.appendChild(
                elCreateNodes('div', {'class': ['form-group', 'row']}, [
                    elCreateNodes('label', {'class': ['col-sm-4', 'col-form-label']}, [
                        document.createTextNode(param),
                        elCreateText('small', {}, ' (' + p[param].type + ')')
                    ]),
                    elCreateNodes('col', {'class': ['col-sm-8']}, [
                        elCreateEmpty('input', {'class': ['form-control'], 'id': 'exploreAPIparam' + k + param, 'value': p[param].example}),
                        elCreateText('small', {}, p[param].desc)
                    ])
                ])
            );
        }
    }
    return form;
}

/**
 * Creates an json object with values from a HTML form created by exploreAPIparamsToForm()
 * @param {object} p Jsonrpc params object
 * @param {string} k Parent key
 * @returns {object} JSON object
 */
function exploreAPIFormToParams(p, k) {
    const request = {};
    for (const param in p) {
        if (p[param].params !== undefined) {
            request[param] = exploreAPIFormToParams(p[param].params, param);
        }
        else {
            let value = document.getElementById('exploreAPIparam' + k + param).value;
            if (value.charAt(0) === '{' ||
                value.charAt(0) === '[')
            {
                request[param] = JSON.parse(value);
            }
            else {
                if (value === '') {
                    //do nothing
                }
                else if (value === 'true') {
                    value = true;
                }
                else if (value === 'false') {
                    value = false;
                }
                else if (!isNaN(value)) {
                    value = Number(value);
                }
                request[param] = value;
            }
        }
    }
    return request;
}

/**
 * Sends the API request and populates the result fields
 * @returns {void}
 */
function exploreAPIsendRequest() {
    document.getElementById('modalScriptAPIresultState').textContent = tn('Sending');
    document.getElementById('modalScriptAPIresultText').textContent = '';
    const method = getSelectValueId('modalScriptAPIcmdsInput');
    const params = APImethods[method].params !== undefined
        ? exploreAPIFormToParams(APImethods[method].params, '')
        : {};
    elShowId('modalScriptAPIresult');
    sendAPI(method, params, function(obj) {
        elGetById('modalScriptAPIresultState').textContent = obj.error
            ? tn('Error')
            : tn('OK');
        elGetById('modalScriptAPIresultText').textContent = JSON.stringify(obj);
    }, true);
}

/**
 * Adds a function to the script content element
 * @param {Event} event triggering event
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addScriptFunction(event) {
    event.preventDefault();
    event.stopPropagation();
    const value = getSelectValueId('modalScriptsFunctionSelect');
    if (value === '') {
        return;
    }
    const el = elGetById('modalScriptsContentInput');
    const [start, end] = [el.selectionStart, el.selectionEnd];
    el.setRangeText(LUAfunctions[value].func, start, end, 'end');
    BSN.Dropdown.getInstance(elGetById('modalScriptsAddFunctionBtn')).hide();
    setFocus(el);
}

/**
 * Adds an API call to the script content element
 * @param {Event} event triggering event
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addScriptAPIcall(event) {
    event.preventDefault();
    event.stopPropagation();
    const method = getSelectValueId('modalScriptsAPIcallSelect');
    if (method === '') {
        return;
    }
    const el = elGetById('modalScriptsContentInput');
    const [start, end] = [el.selectionStart, el.selectionEnd];
    const newText =
        'local options = {}\n' +
        apiParamsToArgs(APImethods[method].params) +
        'local rc, result = mympd.api("' + method + '", options)\n' +
        'if rc == 0 then\n' +
        '\n' +
        'end\n';
    el.setRangeText(newText, start, end, 'preserve');
    BSN.Dropdown.getInstance(elGetById('modalScriptsAddAPIcallBtn')).hide();
    setFocus(el);
}

/**
 * Adds the documented api params to the options lua table for the add api call function
 * @param {object} p parameters object
 * @returns {string} lua code
 */
function apiParamsToArgs(p) {
    let args = '';
    for (const param in p) {
        args += 'options["' + param + '"] = ';
        switch(p[param].type) {
            case APItypes.string:
                args += '"' + p[param].example + '"';
                break;
            case APItypes.array:
                args += '{' + p[param].example.slice(1, -1) + '}';
                break;
            case APItypes.object: {
                args += '{}';
                break;
            }
            default:
                args += p[param].example;
        }
        args += '\n';
    }
    return args;
}

/**
 * Saves a script
 * @param {Element} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function saveScript(target) {
    cleanupModalId('modalScripts');
    btnWaiting(target, true);
    const args = [];
    const argSel = elGetById('modalScriptsArgumentsInput');
    for (let i = 0, j = argSel.options.length; i < j; i++) {
        args.push(argSel.options[i].text);
    }
    sendAPI("MYMPD_API_SCRIPT_SAVE", {
        "oldscript": getDataId('modalScriptsEditTab', 'id'),
        "script": elGetById('modalScriptsScriptInput').value,
        "file": getDataId('modalScriptsEditTab', 'file'),
        "version": getDataId('modalScriptsEditTab', 'version'),
        "order": Number(elGetById('modalScriptsOrderInput').value),
        "content": elGetById('modalScriptsContentInput').value,
        "arguments": args
    }, saveScriptCheckError, true);
}

/**
 * Handler for the MYMPD_API_SCRIPT_SAVE jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function saveScriptCheckError(obj) {
    if (modalApply(obj) === true) {
        showListScripts();
    }
}

/**
 * Validates a script
 * @param {Element} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function validateScript(target) {
    cleanupModalId('modalScripts');
    btnWaiting(target, true);
    sendAPI("MYMPD_API_SCRIPT_VALIDATE", {
        "script": elGetById('modalScriptsScriptInput').value,
        "content": elGetById('modalScriptsContentInput').value,
    }, validateScriptCheckError, true);
}

/**
 * Handler for the MYMPD_API_SCRIPT_VALIDATE jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function validateScriptCheckError(obj) {
    if (modalApply(obj) === true) {
        showModalInfo('Script syntax is valid');
    }
}

/**
 * Appends an argument to the list of script arguments
 * @returns {void}
 */
function addScriptArgument() {
    const el = elGetById('modalScriptsAddArgumentInput');
    elGetById('modalScriptsArgumentsInput').appendChild(
        elCreateText('option', {}, el.value)
    );
    el.value = '';
}

/**
 * Removes an argument from the list of script arguments
 * @param {Event} ev triggering element
 * @returns {void}
 */
function removeScriptArgument(ev) {
    const el = elGetById('modalScriptsAddArgumentInput');
    // @ts-ignore
    el.value = ev.target.text;
    ev.target.remove();
    setFocus(el);
}

/**
 * Opens the scripts modal and shows the edit tab
 * @param {string} script name to edit
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showEditScriptModal(script) {
    // Open modal only if script can be opened
    sendAPI("MYMPD_API_SCRIPT_GET", {"script": script}, function(obj) {
        if (obj.result) {
            showEditScript(script, obj);
            uiElements.modalScripts.show();
        }
    }, true);
}

/**
 * Opens the scripts modal and shows the list tab
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showListScriptModal() {
    uiElements.modalScripts.show();
    showListScripts();
}

/**
 * Shows the edit script tab
 * @param {string} script script name
 * @param {object} [obj] script object
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showEditScript(script, obj) {
    elGetById('modalScriptsScriptInput').value = '';
    elGetById('modalScriptsOrderInput').value = '1';
    elGetById('modalScriptsAddArgumentInput').value = '';
    elClearId('modalScriptsArgumentsInput');
    elGetById('modalScriptsContentInput').value = '';
    cleanupModalId('modalScripts');
    elGetById('modalScripts').firstElementChild.classList.remove('modal-dialog-scrollable');
    elGetById('modalScriptsContentInput').removeAttribute('disabled');
    elGetById('modalScriptsEditTab').classList.add('active');
    elGetById('modalScriptAPItab').classList.remove('active');
    elGetById('modalScriptsListTab').classList.remove('active');
    elGetById('modalScriptsImportTab').classList.remove('active');
    elShowId('modalScriptsEditFooter');
    elHideId('modalScriptAPIfooter');
    elHideId('modalScriptsListFooter');
    elHideId('modalScriptsImportFooter');
    if (obj !== undefined) {
        parseEditScript(obj);
    }
    else if (script !== '') {
        sendAPI("MYMPD_API_SCRIPT_GET", {"script": script}, parseEditScript, false);
    }
    else {
        setDataId('modalScriptsEditTab', 'id', '');
        setDataId('modalScriptsEditTab', 'file', '');
        setDataId('modalScriptsEditTab', 'version', 0);
        elDisableId('modalScriptsUpdateBtn');
        elHideId('modalScriptsEditDescRow');
    }
    setFocusId('modalScriptsScriptInput');
}

/**
 * Parses the MYMPD_API_SCRIPT_GET jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parseEditScript(obj) {
    setDataId('modalScriptsEditTab', 'id', obj.result.script);
    setDataId('modalScriptsEditTab', 'file', obj.result.metadata.file);
    setDataId('modalScriptsEditTab', 'version', obj.result.metadata.version);
    elGetById('modalScriptsScriptInput').value = obj.result.script;
    elGetById('modalScriptsOrderInput').value = obj.result.metadata.order;
    elGetById('modalScriptsAddArgumentInput').value = '';
    if (obj.result.metadata.file !== '' && obj.result.metadata.version > 0) {
        elEnableId('modalScriptsUpdateBtn');
        elShowId('modalScriptsEditDescRow');
        elGetById('modalScriptsEditLink').setAttribute('href', scriptsUri + dirname(obj.result.metadata.file));
    }
    else {
        elDisableId('modalScriptsUpdateBtn');
        elHideId('modalScriptsEditDescRow');
    }
    const selSA = elGetById('modalScriptsArgumentsInput');
    selSA.options.length = 0;
    for (let i = 0, j = obj.result.metadata.arguments.length; i < j; i++) {
        selSA.appendChild(
            elCreateText('option', {}, obj.result.metadata.arguments[i])
        );
    }
    elGetById('modalScriptsContentInput').value = obj.result.content;
}

/**
 * Shows the list scripts tab
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showModalScriptAPItab() {
    cleanupModalId('modalScripts');
    elGetById('modalScripts').firstElementChild.classList.remove('modal-dialog-scrollable');
    elGetById('modalScriptAPItab').classList.add('active');
    elGetById('modalScriptsListTab').classList.remove('active');
    elGetById('modalScriptsEditTab').classList.remove('active');
    elGetById('modalScriptsImportTab').classList.remove('active');
    elShowId('modalScriptAPIfooter');
    elHideId('modalScriptsListFooter');
    elHideId('modalScriptsEditFooter');
    elHideId('modalScriptsImportFooter');
    getScriptList(true);
}

/**
 * Shows the list scripts tab
 * @returns {void}
 */
function showListScripts() {
    cleanupModalId('modalScripts');
    elGetById('modalScripts').firstElementChild.classList.remove('modal-dialog-scrollable');
    elGetById('modalScriptsListTab').classList.add('active');
    elGetById('modalScriptAPItab').classList.remove('active');
    elGetById('modalScriptsEditTab').classList.remove('active');
    elGetById('modalScriptsImportTab').classList.remove('active');
    elShowId('modalScriptsListFooter');
    elHideId('modalScriptAPIfooter');
    elHideId('modalScriptsEditFooter');
    elHideId('modalScriptsImportFooter');
    getScriptList(true);
}

/**
 * Deletes a script after confirmation
 * @param {EventTarget} el triggering element
 * @param {string} script script to delete
 * @returns {void}
 */
function deleteScript(el, script) {
    cleanupModalId('modalScripts');
    showConfirmInline(el.parentNode.previousSibling, tn('Do you really want to delete the script?', {"script": script}), tn('Yes, delete it'), function() {
        sendAPI("MYMPD_API_SCRIPT_RM", {
            "script": script
        }, deleteScriptCheckError, true);
    });
}

/**
 * Handler for the MYMPD_API_SCRIPT_RM jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function deleteScriptCheckError(obj) {
    if (modalListApply(obj) === true) {
        getScriptList(true);
    }
}

/**
 * Gets the list of scripts
 * @param {boolean} all true = get all scripts, false = get all scripts with pos > 0
 * @returns {void}
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
    const table = elGetById('modalScriptsList');
    const tbodyScripts = table.querySelector('tbody');
    elClear(tbodyScripts);
    const mainmenuScripts = elGetById('scripts');
    elClear(mainmenuScripts);
    const triggerScripts = elGetById('modalTriggerScriptInput');
    elClear(triggerScripts);
    const widgetScripts = elGetById('modalHomeWidgetScriptInput');
    elClear(widgetScripts);

    if (checkResult(obj, table, 'table') === false) {
        return;
    }

    const timerActions = elCreateEmpty('optgroup', {"id": "timerActionsScriptsOptGroup", "label": tn('Script')});
    setData(timerActions, 'value', 'script');
    const scriptListLen = obj.result.data.length;
    if (scriptListLen > 0) {
        obj.result.data.sort(function(a, b) {
            return a.metadata.order - b.metadata.order;
        });
        for (let i = 0; i < scriptListLen; i++) {
            //script list in main menu
            if (obj.result.data[i].metadata.order > 0) {
                const a = elCreateNodes('a', {"class": ["dropdown-item", "alwaysEnabled", "py-2"], "href": "#"}, [
                    elCreateText('span', {"class": ["mi", "me-2"]}, "code"),
                    elCreateText('span', {}, obj.result.data[i].name)
                ]);
                setData(a, 'href', {"script": obj.result.data[i].name, "arguments": obj.result.data[i].metadata.arguments});
                mainmenuScripts.appendChild(a);
            }
            //script list in scripts modal
            const tr = elCreateNodes('tr', {"title": tn('Edit')}, [
                elCreateText('td', {}, obj.result.data[i].name),
                elCreateText('td', {}, obj.result.data[i].metadata.order),
                elCreateNodes('td', {"data-col": "Action"}, [
                    elCreateText('a', {"href": "#", "data-title-phrase": "Delete", "data-action": "delete", "class": ["me-2", "mi", "color-darkgrey"]}, 'delete'),
                    elCreateText('a', {"href": "#", "data-title-phrase": "Execute", "data-action": "execute", "class": ["me-2", "mi", "color-darkgrey"]}, 'play_arrow'),
                    elCreateText('a', {"href": "#", "data-title-phrase": "Add to homescreen", "data-action": "add2home", "class": ["me-2", "mi", "color-darkgrey"]}, 'add_to_home_screen')
                ])
            ]);
            setData(tr, 'script', obj.result.data[i].name);
            tr.setAttribute('data-file', obj.result.data[i].metadata.file);
            setData(tr, 'href', {"script": obj.result.data[i].name, "arguments": obj.result.data[i].metadata.arguments});
            tbodyScripts.appendChild(tr);

            //script list select for timers, triggers and home widgets
            const option = elCreateText('option', {"value": obj.result.data[i].name}, obj.result.data[i].name);
            addOptionToScriptSelect(timerActions, option, obj.result.data[i].metadata.arguments);
            addOptionToScriptSelect(triggerScripts, option, obj.result.data[i].metadata.arguments);
            addOptionToScriptSelect(widgetScripts, option, obj.result.data[i].metadata.arguments);
        }
    }

    if (scriptListLen === 0) {
        elHide(mainmenuScripts.previousElementSibling);
    }
    else {
        elShow(mainmenuScripts.previousElementSibling);
    }
    //update timer actions select
    const old = elGetById('timerActionsScriptsOptGroup');
    if (old) {
        old.replaceWith(timerActions);
    }
    else {
        elGetById('modalTimerActionInput').appendChild(timerActions);
    }
}

/**
 * Add's an option to the script select
 * @param {HTMLElement} sel Select element to populate
 * @param {HTMLElement} opt Option element to add
 * @param {object} args Script arguments object
 * @returns {void}
 */
function addOptionToScriptSelect(sel, opt, args) {
    const optEl = opt.cloneNode(true);
    setData(optEl, 'arguments', {"arguments": args});
    sel.appendChild(optEl);
}

/**
 * Shows the import scripts tab
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showImportScript() {
    cleanupModalId('modalScripts');
    elGetById('modalScripts').firstElementChild.classList.add('modal-dialog-scrollable');
    elGetById('modalScriptsImportTab').classList.add('active');
    elGetById('modalScriptAPItab').classList.remove('active');
    elGetById('modalScriptsListTab').classList.remove('active');
    elGetById('modalScriptsEditTab').classList.remove('active');
    elShowId('modalScriptsImportFooter');
    elHideId('modalScriptAPIfooter');
    elHideId('modalScriptsListFooter');
    elHideId('modalScriptsEditFooter');
    if (userAgentData.isMobile === false) {
        setFocusId('modalScriptsImportSearchStr');
    }
    const list = elGetById('modalScriptsImportList');
    elClear(list);
    list.appendChild(
        elCreateTextTn('li', {"class": ["list-group-item", "not-clickable"]}, 'Loading...')
    );
    httpGet(subdir + '/proxy?uri=' + myEncodeURI(scriptsImportUri + 'index.json'), function(obj) {
        elClear(list);
        if (obj === null) {
            list.appendChild(
                elCreateNode('li', {"class": ["list-group-item", "not-clickable"]},
                    elCreateTextTn('div', {"class": ["alert", "alert-danger"]}, 'Failure loading script list.')
                )
            );
            return;
        }
        for (const key in obj) {
            const script = obj[key];
            const clickable = elGetById('modalScriptsList').querySelector('[data-file="' + key + '"') === null
                ? 'clickable'
                : 'disabled';
            list.appendChild(
                elCreateNodes('li', {"data-script": key, "class": ["list-group-item", "list-group-item-action", clickable],
                    "title": tn("Import"), "data-title-phrase": "Import"}, [
                    elCreateNodes('div', {"class": ["d-flex", "w-100", "justify-content-between"]}, [
                        elCreateText('h5', {}, script.name),
                        elCreateText('a', {"href": scriptsUri + dirname(key), "target": "_blank", "class": ["mi", "text-success"],
                            "data-title": tn("Open"), "data-title-phrase": "Open"}, 'open_in_browser')
                    ]),
                    elCreateNodes('div', {"class": ["d-flex", "w-100", "justify-content-between"]}, [
                        elCreateText('p', {"class": ["mb-1"]}, script.desc),
                        elCreateText('small', {}, 'v' + script.version)
                    ])
                ])
            );
        }
    }, true);
}

/**
 * Shows the edit script tab and imports a script
 * @param {EventTarget} target Event target
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function importScript(target) {
    const script = target.getAttribute('data-script');
    showEditScript('');
    elDisableId('modalScriptsContentInput');
    httpGet(subdir + '/proxy?uri=' + myEncodeURI(scriptsImportUri + script), function(text) {
        doImportScript(text);
    }, false);
}

/**
 * Imports a script from the mympd-scripts repository
 * @param {string} text Script to import
 * @returns {boolean} true on success, else false
 */
function doImportScript(text) {
    const lines = text.split('\n');
    const firstLine = lines.shift();
    let obj;
    let rc = true;
    try {
        obj = JSON.parse(firstLine.substring(firstLine.indexOf('{')));
        const scriptArgEl = elGetById('modalScriptsArgumentsInput');
        scriptArgEl.options.length = 0;
        for (let i = 0, j = obj.arguments.length; i < j; i++) {
            scriptArgEl.appendChild(
                elCreateText('option', {}, obj.arguments[i])
            );
        }
        elGetById('modalScriptsScriptInput').value = obj.name;
        elGetById('modalScriptsOrderInput').value = obj.order;
        setDataId('modalScriptsEditTab', 'file', obj.file);
        setDataId('modalScriptsEditTab', 'version', obj.version);
        elGetById('modalScriptsContentInput').value = lines.join('\n');
    }
    catch(error) {
        showModalAlert({
            "error": {
                "message": "Can not parse script metadata."
            }
        });
        logError('Can not parse script metadata:' + firstLine);
        logError(error);
        rc = false;
    }
    elEnableId('modalScriptsContentInput');
    setFocusId('modalScriptsContentInput');
    return rc;
}

/**
 * Updates a script from the mympd-scripts repository
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function updateScript() {
    cleanupModalId('modalScripts');
    btnWaitingId('modalScriptsUpdateBtn', true);
    const importFile = getDataId('modalScriptsEditTab', 'file',);
    const currentVersion = getDataId('modalScriptsEditTab', 'version');
    if (importFile === '' || currentVersion === '') {
        return;
    }
    httpGet(subdir + '/proxy?uri=' + myEncodeURI(scriptsImportUri + 'index.json'), function(obj) {
        if (obj[importFile] === undefined) {
            showModalAlert({
                "error": {
                    "message": "Can not find script in repository."
                }
            });
            btnWaitingId('modalScriptsUpdateBtn', false);
            return;
        }
        if (obj[importFile].version === currentVersion) {
            showModalInfo("Script is up-to-date.");
            btnWaitingId('modalScriptsUpdateBtn', false);
            return;
        }
        elDisableId('modalScriptsContentInput');
        httpGet(subdir + '/proxy?uri=' + myEncodeURI(scriptsImportUri + importFile), function(text) {
            if (doImportScript(text) === true) {
                showModalInfo("Script successfully updated.");
            }
            btnWaitingId('modalScriptsUpdateBtn', false);
        }, false);
    }, true);
}
