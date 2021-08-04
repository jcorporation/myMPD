"use strict";
// SPDX-License-Identifier: GPL-2.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

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
        }
    }, false);

    document.getElementById('listScriptsList').addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (event.target.nodeName === 'TD') {
            if (getCustomDomProperty(event.target.parentNode, 'data-script') === '') {
                return false;
            }
            showEditScript(getCustomDomProperty(event.target.parentNode, 'data-script'));
        }
        else if (event.target.nodeName === 'A') {
            const action = getCustomDomProperty(event.target, 'data-action');
            const script = getCustomDomProperty(event.target.parentNode.parentNode, 'data-script');
            if (action === 'delete') {
                deleteScript(event.target, script);
            }
            else if (action === 'execute') {
                execScript(getCustomDomProperty(event.target.parentNode.parentNode, 'data-href'));
            }
            else if (action === 'add2home') {
                addScriptToHome(script, getCustomDomProperty(event.target.parentNode.parentNode, 'data-href'));
            }
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
    
    let methodList = '<option value="">' + t('Select method') + '</option>';
    for (const m in APImethods) {
        methodList += '<option value="' + m + '">' + m + '</option>';
    }
    const selectAPIcallEl = document.getElementById('selectAPIcall');
    selectAPIcallEl.innerHTML = methodList;
    
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
        const method = getSelectValue('selectAPIcall');
        if (method === '') {
            return;
        }
        const el = document.getElementById('textareaScriptContent');
        const [start, end] = [el.selectionStart, el.selectionEnd];
        const newText = 'rc, raw_result = mympd_api_raw("' + method + '", json.encode(' +
            apiParamsToArgs(APImethods[method].params) +
            '))\n' + 
            'if rc == 0 then\n' +
            '    result = json.decode(raw_result)\n' +
            'end\n';
        el.setRangeText(newText, start, end, 'preserve');
        document.getElementById('btnDropdownAddAPIcall').Dropdown.hide();
        el.focus();
    }, false);
    
    let functionList = '<option value="">' + t('Select function') + '</option>';
    for (const m in LUAfunctions) {
        functionList += '<option value="' + m + '">' + m + '</option>';
    }
    const selectFunctionEl = document.getElementById('selectFunction');
    selectFunctionEl.innerHTML = functionList;
    
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
        const value = getSelectValue('selectFunction');
        if (value === '') {
            return;
        }
        const el = document.getElementById('textareaScriptContent');
        const [start, end] = [el.selectionStart, el.selectionEnd];
        el.setRangeText(LUAfunctions[value].func, start, end, 'end');
        document.getElementById('btnDropdownAddFunction').Dropdown.hide();
        el.focus();
    }, false);
}

function apiParamsToArgs(p) {
    let args = '{';
    let i = 0;
    for (const param in p) {
        if (i > 0) {
            args += ', ';
        }
        i++;
        args += param + ' =  ';
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

//eslint-disable-next-line no-unused-vars
function saveScript() {
    let formOK = true;
    
    const nameEl = document.getElementById('inputScriptName');
    if (!validatePlnameEl(nameEl)) {
        formOK = false;
    }
    
    const orderEl = document.getElementById('inputScriptOrder');
    if (!validateInt(orderEl)) {
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
            }, showListScripts, false);
    }
}

function addScriptArgument() {
    const el = document.getElementById('inputScriptArgument');
    if (validatePlnameEl(el)) {
        const o = document.createElement('option');
        o.text = el.value;
        document.getElementById('selectScriptArguments').appendChild(o);
        el.value = '';
    }
}

function removeScriptArgument(ev) {
    const el = document.getElementById('inputScriptArgument');
    el.value = ev.target.text;
    ev.target.remove();
    el.focus();  
}

//eslint-disable-next-line no-unused-vars
function showEditScript(script) {
    document.getElementById('listScripts').classList.remove('active');
    document.getElementById('editScript').classList.add('active');
    document.getElementById('listScriptsFooter').classList.add('hide');
    document.getElementById('editScriptFooter').classList.remove('hide');
    
    removeIsInvalid(document.getElementById('modalScripts'));
      
    if (script !== '') {
        sendAPI("MYMPD_API_SCRIPT_GET", {"script": script}, parseEditScript, false);
    }
    else {
        document.getElementById('inputOldScriptName').value = '';
        document.getElementById('inputScriptName').value = '';
        document.getElementById('inputScriptOrder').value = '1';
        document.getElementById('inputScriptArgument').value = '';
        document.getElementById('selectScriptArguments').textContent = '';
        document.getElementById('textareaScriptContent').value = '';
    }
    document.getElementById('inputScriptName').focus();
}

function parseEditScript(obj) {
    document.getElementById('inputOldScriptName').value = obj.result.script;
    document.getElementById('inputScriptName').value = obj.result.script;
    document.getElementById('inputScriptOrder').value = obj.result.metadata.order;
    document.getElementById('inputScriptArgument').value = '';
    const selSA = document.getElementById('selectScriptArguments');
    selSA.textContent = '';
    for (let i = 0, j = obj.result.metadata.arguments.length; i < j; i++) {
        const o = document.createElement('option');
        o.textContent = obj.result.metadata.arguments[i];
        selSA.appendChild(o);
    }
    document.getElementById('textareaScriptContent').value = obj.result.content;
}

function showListScripts() {
    document.getElementById('listScripts').classList.add('active');
    document.getElementById('editScript').classList.remove('active');
    document.getElementById('listScriptsFooter').classList.remove('hide');
    document.getElementById('editScriptFooter').classList.add('hide');
    sendAPI("MYMPD_API_SCRIPT_LIST", {"all": true}, parseScriptList);
}

function deleteScript(el, script) {
    showConfirmInline(el.parentNode.previousSibling, tn('Do you really want to delete the script?', {"script": script}), tn('Yes, delete it'), function() {
        sendAPI("MYMPD_API_SCRIPT_RM", {"script": script}, function() {
            getScriptList(true);
        }, false);
    });
}

function getScriptList(all) {
    sendAPI("MYMPD_API_SCRIPT_LIST", {"all": all}, parseScriptList, false);
}

function parseScriptList(obj) {
    const timerActions = document.createElement('optgroup');
    setCustomDomProperty(timerActions, 'data-value', 'script');
    timerActions.setAttribute('label', t('Script'));
    const scriptMaxListLen = 4;
    //list in main menu
    let scriptListMain = '';
    //list in scripts dialog
    let scriptList = '';
    const scriptListLen = obj.result.data.length;
    let showScriptListLen = 0;
    if (scriptListLen > 0) {
        obj.result.data.sort(function(a, b) {
            return a.metadata.order - b.metadata.order;
        });
        for (let i = 0; i < scriptListLen; i++) {
            let arglist = '';
            if (obj.result.data[i].metadata.arguments.length > 0) {
                for (let j = 0, k = obj.result.data[i].metadata.arguments.length; j < k; j++) {
                    obj.result.data[i].metadata.arguments[j] = e(obj.result.data[i].metadata.arguments[j]);
                }
                arglist = '"' + obj.result.data[i].metadata.arguments.join('","') + '"';
            }
            if (obj.result.data[i].metadata.order > 0) {
                showScriptListLen++;
                scriptListMain += '<a class="dropdown-item text-light alwaysEnabled" href="#" data-href=\'{"script": "' + 
                    e(obj.result.data[i].name) + '", "arguments": [' + arglist + ']}\'>' + e(obj.result.data[i].name) + '</a>';
                
            }
            scriptList += '<tr data-script="' + encodeURI(obj.result.data[i].name) + '" ' +
                'data-href=\'{"script": "' + e(obj.result.data[i].name) + '", "arguments": [' + arglist + ']}\'>' +
                '<td>' + e(obj.result.data[i].name) + '</td>' +
                '<td data-col="Action">' +
                    '<a href="#" title="' + t('Delete') + '" data-action="delete" class="mi color-darkgrey">delete</a>' +
                    '<a href="#" title="' + t('Execute') + '" data-action="execute" class="mi color-darkgrey">play_arrow</a>' +
                    '<a href="#" title="' + t('Add to homescreen') + '" data-action="add2home" class="mi color-darkgrey">add_to_home_screen</a>' +
                '</td></tr>';
            timerActions.innerHTML += '<option data-arguments=\'{"arguments":[' + arglist + ']}\' value="' + 
                e(obj.result.data[i].name) + '">' + e(obj.result.data[i].name) + '</option>';
        }
        document.getElementById('listScriptsList').innerHTML = scriptList;
    }
    else {
        document.getElementById('listScriptsList').innerHTML = '<tr class="not-clickable">' +
            '<td colspan="3"><span class="mi">info</span>&nbsp;&nbsp;' + t('Empty list') + '</td></tr>';
    }
    document.getElementById('scripts').innerHTML = (showScriptListLen > scriptMaxListLen || showScriptListLen === 0 ? '' : '<div class="dropdown-divider"></div>') + scriptListMain;
        
    if (showScriptListLen > scriptMaxListLen) {
        document.getElementById('navScripting').classList.remove('hide');
        document.getElementById('scripts').classList.add('collapse', 'menu-indent');
    }
    else {
        document.getElementById('navScripting').classList.add('hide');
        document.getElementById('scripts').classList.remove('collapse', 'menu-indent');
    }

    document.getElementById('selectTriggerScript').innerHTML = timerActions.innerHTML;
    
    const old = document.getElementById('selectTimerAction').querySelector('optgroup[data-value="script"]');
    if (old) {
        old.replaceWith(timerActions);
    }
    else {
        document.getElementById('selectTimerAction').appendChild(timerActions);
    }
    //reinit mainmenu -> change of script list
    uiElements.dropdownMainMenu.dispose();
    uiElements.dropdownMainMenu = new BSN.Dropdown(document.getElementById('mainMenu'));
}

//eslint-disable-next-line no-unused-vars
function execScriptFromOptions(cmd, options) {
    const args = options !== undefined && options !== '' ? options.split(',') : [];
    execScript(JSON.stringify({"script": cmd, "arguments": args}));
}

function execScript(href) {
    const cmd = JSON.parse(href);
    if (cmd.arguments.length === 0) {
        sendAPI("MYMPD_API_SCRIPT_EXECUTE", {"script": cmd.script, "arguments": {}});
    }
    else {
        let arglist ='';
        for (let i = 0, j = cmd.arguments.length; i < j; i++) {
            arglist += '<div class="form-group row">' +
                  '<label class="col-sm-4 col-form-label" for="inputScriptArg' + i + '">' + e(cmd.arguments[i]) +'</label>' +
                  '<div class="col-sm-8">' +
                     '<input name="' + e(cmd.arguments[i]) + '" id="inputScriptArg' + i + '" type="text" class="form-control border-secondary" value="">' +
                  '</div>' +
                '</div>';

        }
        document.getElementById('execScriptArguments').innerHTML = arglist;
        document.getElementById('modalExecScriptScriptname').value = cmd.script;
        uiElements.modalExecScript.show();
    }
}

//eslint-disable-next-line no-unused-vars
function execScriptArgs() {
    const script = document.getElementById('modalExecScriptScriptname').value;
    const args = {};
    const inputs = document.getElementById('execScriptArguments').getElementsByTagName('input');
    for (let i = 0, j = inputs.length; i < j; i++) {
        args[inputs[i].name] = inputs[i].value;
    }
    sendAPI("MYMPD_API_SCRIPT_EXECUTE", {"script": script, "arguments": args});
    uiElements.modalExecScript.hide();
}
