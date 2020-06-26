"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

//eslint-disable-next-line no-unused-vars

function getScriptList(all) {
    sendAPI("MYMPD_API_SCRIPT_LIST", {"all": all}, parseScriptList, false);
}

function parseScriptList(obj) {
    let timerActions = document.createElement('optgroup');
    timerActions.setAttribute('data-value', 'script');
    timerActions.setAttribute('label', t('Script'));
    let scriptMaxListLen = 4;
    let scriptList = '';
    let scriptListLen = obj.result.data.length;
    if (scriptListLen > 0) {
        obj.result.data.sort(function(a, b) {
            return a.metadata.order - b.metadata.order;
        });
        let mi = 0;
        for (let i = 0; i < scriptListLen; i++) {
            if (obj.result.data[i].metadata.order > 0) {
                if (mi === 0) {
                    scriptList = scriptListLen > scriptMaxListLen ? '' : '<div class="dropdown-divider"></div>';
                }
                mi++;
                let arglist = '';
                if (obj.result.data[i].metadata.arguments.length > 0) {
                    arglist = '"' + obj.result.data[i].metadata.arguments.join('","') + '"';
                }
                scriptList += '<a class="dropdown-item text-light alwaysEnabled" href="#" data-href=\'{"script": "' + 
                            e(obj.result.data[i].name) + '", "arguments": [' + arglist + ']}\'>' + e(obj.result.data[i].name) + '</a>';
            }
            timerActions.innerHTML = '<option value="' + e(obj.result.data[i].name) + '">' + e(obj.result.data[i].name) + '</option>';
        }
    }
    document.getElementById('scripts').innerHTML = scriptList;
        
    if (scriptListLen > scriptMaxListLen) {
        document.getElementById('navScripting').classList.remove('hide');
        document.getElementById('scripts').classList.add('collapse', 'menu-indent');
    }
    else {
        document.getElementById('navScripting').classList.add('hide');
        document.getElementById('scripts').classList.remove('collapse', 'menu-indent');
    }
    
    let old = document.getElementById('selectTimerAction').querySelector('optgroup[data-value="script"]');
    if (old) {
        old.replaceWith(timerActions);
    }
    else {
        document.getElementById('selectTimerAction').appendChild(timerActions);
    }
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
