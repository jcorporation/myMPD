"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
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
 * Executes a script and asks for argument values
 * @param {object} cmd script and arguments object to execute
 * @returns {void}
 */
function execScript(cmd) {
    if (cmd.arguments.length === 0) {
        sendAPI("MYMPD_API_SCRIPT_EXECUTE", {
            "script": cmd.script,
            "arguments": {}
        }, null, false);
    }
    else {
        const arglist = elGetById('modalScriptExecArgumentsList');
        elClear(arglist);
        for (let i = 0, j = cmd.arguments.length; i < j; i++) {
            arglist.appendChild(
                elCreateNodes('div', {"class": ["form-group", "row", "mb-3"]}, [
                    elCreateText('label', {"class": ["col-sm-4", "col-form-label"]}, cmd.arguments[i]),
                    elCreateNode('div', {"class": ["col-sm-8"]},
                        elCreateEmpty('input', {"name": cmd.arguments[i], "type": "text", "class": ["form-control"]})
                    )
                ])
            );
        }
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
    const args = {};
    const inputs = document.querySelectorAll('#modalScriptExecArgumentsList input');
    for (let i = 0, j = inputs.length; i < j; i++) {
        args[inputs[i].name] = inputs[i].value;
    }
    sendAPI("MYMPD_API_SCRIPT_EXECUTE", {
        "script": script,
        "arguments": args
    }, modalClose, true);
}
