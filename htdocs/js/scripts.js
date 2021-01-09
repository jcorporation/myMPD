"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
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
        }
    }, false);

    document.getElementById('listScriptsList').addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (event.target.nodeName === 'TD') {
            if (settings.featScripteditor === false || event.target.parentNode.getAttribute('data-script') === '') {
                return false;
            }
            showEditScript(decodeURI(event.target.parentNode.getAttribute('data-script')));
        }
        else if (event.target.nodeName === 'A') {
            let action = event.target.getAttribute('data-action');
            let script = decodeURI(event.target.parentNode.parentNode.getAttribute('data-script'));
            if (action === 'delete') {
                deleteScript(script);
            }
            else if (action === 'execute') {
                execScript(event.target.getAttribute('data-href'));
            }
            else if (action === 'add2home') {
                addScriptToHome(script, event.target.getAttribute('data-href'))
            }
        }
    }, false);

    document.getElementById('modalScripts').addEventListener('shown.bs.modal', function () {
        showListScripts();
    });
}

//eslint-disable-next-line no-unused-vars
function saveScript() {
    let formOK = true;
    
    let nameEl = document.getElementById('inputScriptName');
    if (!validatePlnameEl(nameEl)) {
        formOK = false;
    }
    
    let orderEl = document.getElementById('inputScriptOrder');
    if (!validateInt(orderEl)) {
        formOK = false;
    }
    
    if (formOK === true) {
        let args = [];
        let argSel = document.getElementById('selectScriptArguments');
        for (let i = 0; i < argSel.options.length; i++) {
            args.push(argSel.options[i].text);
        }
        sendAPI("MYMPD_API_SCRIPT_SAVE", {
            "oldscript": document.getElementById('inputOldScriptName').value,
            "script": nameEl.value,
            "order": parseInt(orderEl.value),
            "content": document.getElementById('textareaScriptContent').value,
            "arguments": args
            }, showListScripts, false);
    }
}

function addScriptArgument() {
    let el = document.getElementById('inputScriptArgument');
    if (validatePlnameEl(el)) {
        let o = document.createElement('option');
        o.text = el.value;
        document.getElementById('selectScriptArguments').appendChild(o);
        el.value = '';
    }
}

function removeScriptArgument(ev) {
    let el = document.getElementById('inputScriptArgument');
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
        document.getElementById('selectScriptArguments').innerText = '';
        document.getElementById('textareaScriptContent').value = '';
    }
}

function parseEditScript(obj) {
    document.getElementById('inputOldScriptName').value = obj.result.script;
    document.getElementById('inputScriptName').value = obj.result.script;
    document.getElementById('inputScriptOrder').value = obj.result.metadata.order;
    document.getElementById('inputScriptArgument').value = '';
    let selSA = document.getElementById('selectScriptArguments');
    selSA.innerText = '';
    for (let i = 0; i < obj.result.metadata.arguments.length; i++) {
        let o = document.createElement('option');
        o.innerText = obj.result.metadata.arguments[i];
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

function deleteScript(script) {
    sendAPI("MYMPD_API_SCRIPT_DELETE", {"script": script}, function() {
        getScriptList(true);
    }, false);
}

function getScriptList(all) {
    sendAPI("MYMPD_API_SCRIPT_LIST", {"all": all}, parseScriptList, false);
}

function parseScriptList(obj) {
    let timerActions = document.createElement('optgroup');
    timerActions.setAttribute('data-value', 'script');
    timerActions.setAttribute('label', t('Script'));
    let scriptMaxListLen = 4;
    let scriptListMain = ''; //list in main menu
    let scriptList = ''; //list in scripts dialog
    let scriptListLen = obj.result.data.length;
    let showScriptListLen = 0;
    if (scriptListLen > 0) {
        obj.result.data.sort(function(a, b) {
            return a.metadata.order - b.metadata.order;
        });
        for (let i = 0; i < scriptListLen; i++) {
            let arglist = '';
            if (obj.result.data[i].metadata.arguments.length > 0) {
                for (let j = 0; j < obj.result.data[i].metadata.arguments.length; j++) {
                    obj.result.data[i].metadata.arguments[j] = e(obj.result.data[i].metadata.arguments[j]);
                }
                arglist = '"' + obj.result.data[i].metadata.arguments.join('","') + '"';
            }
            if (obj.result.data[i].metadata.order > 0) {
                showScriptListLen++;
                scriptListMain += '<a class="dropdown-item text-light alwaysEnabled" href="#" data-href=\'{"script": "' + 
                    e(obj.result.data[i].name) + '", "arguments": [' + arglist + ']}\'>' + e(obj.result.data[i].name) + '</a>';
                
            }
            scriptList += '<tr data-script="' + encodeURI(obj.result.data[i].name) + '"><td>' + e(obj.result.data[i].name) + '</td>' +
                '<td data-col="Action">' +
                    (settings.featScripteditor === true ? 
                        '<a href="#" title="' + t('Delete') + '" data-action="delete" class="material-icons color-darkgrey">delete</a>' : '') +
                        '<a href="#" title="' + t('Execute') + '" data-action="execute" class="material-icons color-darkgrey" ' +
                            ' data-href=\'{"script": "' + e(obj.result.data[i].name) + '", "arguments": [' + arglist + ']}\'>play_arrow</a>' +
                        '<a href="#" title="' + t('Add to homescreen') + '" data-action="add2home" class="material-icons color-darkgrey" ' +
                            ' data-href=\'{"script": "' + e(obj.result.data[i].name) + '", "arguments": [' + arglist + ']}\'>add_to_home_screen</a>' +
                '</td></tr>';
            timerActions.innerHTML += '<option data-arguments=\'{"arguments":[' + arglist + ']}\' value="' + 
                e(obj.result.data[i].name) + '">' + e(obj.result.data[i].name) + '</option>';
        }
        document.getElementById('listScriptsList').innerHTML = scriptList;
    }
    else {
        document.getElementById('listScriptsList').innerHTML = '<tr class="not-clickable"><td><span class="material-icons">error_outline</span></td>' +
            '<td colspan="2">' + t('Empty list') + '</td></tr>';
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
    
    let old = document.getElementById('selectTimerAction').querySelector('optgroup[data-value="script"]');
    if (old) {
        old.replaceWith(timerActions);
    }
    else {
        document.getElementById('selectTimerAction').appendChild(timerActions);
    }
}

//eslint-disable-next-line no-unused-vars
function execScriptFromOptions(cmd, options) {
    let args = options !== undefined && options !== '' ? options.split(',') : [];
    let script = {"script": cmd, "arguments": args};
    execScript(JSON.stringify(script));
}

function execScript(href) {
    let cmd = JSON.parse(href);
    if (cmd.arguments.length === 0) {
        sendAPI("MYMPD_API_SCRIPT_EXECUTE", {"script": cmd.script, "arguments": {}});
    }
    else {
        let arglist ='';
        for (let i = 0; i < cmd.arguments.length; i++) {
            arglist += '<div class="form-group row">' +
                  '<label class="col-sm-4 col-form-label" for="inputScriptArg' + i + '">' + e(cmd.arguments[i]) +'</label>' +
                  '<div class="col-sm-8">' +
                     '<input name="' + e(cmd.arguments[i]) + '" id="inputScriptArg' + i + '" type="text" class="form-control border-secondary" value="">' +
                  '</div>' +
                '</div>';

        }
        document.getElementById('execScriptArguments').innerHTML = arglist;
        document.getElementById('modalExecScriptScriptname').value = cmd.script;
        modalExecScript.show();
    }
}

//eslint-disable-next-line no-unused-vars
function execScriptArgs() {
    let script = document.getElementById('modalExecScriptScriptname').value;
    let args = {};
    let inputs = document.getElementById('execScriptArguments').getElementsByTagName('input');
    for (let i = 0; i < inputs.length; i++) {
        args[inputs[i].name] = inputs[i].value;
    }
    sendAPI("MYMPD_API_SCRIPT_EXECUTE", {"script": script, "arguments": args});
    modalExecScript.hide();
}
