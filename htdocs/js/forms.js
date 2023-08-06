"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module forms_js */

/**
 * Populates the settings json from input elements
 * @param {string} prefix for element ids
 * @param {object} settingsParams settings object to populate
 * @param {object} defaults settingsFields object
 * @returns {boolean} true on success, else false
 */
function formToJson(prefix, settingsParams, defaults) {
    for (const key in defaults) {
        if (defaults[key].inputType === 'none') {
            continue;
        }
        const id = prefix + ucFirst(key) + 'Input';
        const el = document.getElementById(id);
        if (el) {
            const value = defaults[key].inputType === 'select'
                ? getSelectValue(el)
                : defaults[key].inputType === 'mympd-select-search'
                    ? getData(el, 'value')
                    : defaults[key].inputType === 'checkbox'
                        ? getBtnChkValue(el)
                        : el.value;
            if (defaults[key].validate !== undefined) {
                const func = getFunctionByName(defaults[key].validate.cmd);
                if (func(el, ... defaults[key].validate.options) === false) {
                    return false;
                }
            }
            settingsParams[key] = defaults[key].contentType === 'number'
                ? Number(value)
                : value;
        }
        else {
            logError('Element not found: ' + id);
            return false;
        }
    }
    return true;
}

/**
 * Creates form fields
 * @param {object} fields object with the values for the elements to create
 * @param {object} defaults object with elements to create and the default values
 * @param {string} prefix prefix for element ids
 * @param {object} forms cache for the form field containers
 * @returns {void}
 */
function createFrm(fields, defaults, prefix, forms) {
    // iterate through sorted keys
    const settingsKeys = Object.keys(defaults);
    settingsKeys.sort();
    for (let i = 0, j = settingsKeys.length; i < j; i++) {
        const key = settingsKeys[i];
        // check if we should add a field
        if (defaults[key] === undefined ||
            defaults[key].form === undefined ||
            defaults[key].inputType === 'none')
        {
            continue;
        }
        // calculate a camelCase id
        const id = prefix + ucFirst(key) + 'Input';
        // get the container
        const form = defaults[key].form;
        if (forms[form] === undefined) {
            forms[form] = document.getElementById(form);
            // clear the container if it was not cached
            elClear(forms[form]);
        }
        // create the form field
        const col = elCreateEmpty('div', {"class": ["col-sm-8", "position-relative"]});
        if (defaults[key].inputType === 'select') {
            const select = elCreateEmpty('select', {"class": ["form-select"], "id": id});
            for (const value in defaults[key].validValues) {
                select.appendChild(
                    elCreateTextTn('option', {"value": value}, defaults[key].validValues[value])
                );
                if ((defaults[key].contentType === 'integer' && fields[key] === Number(value)) ||
                    fields[key] === value)
                {
                    select.lastChild.setAttribute('selected', 'selected');
                }
            }
            col.appendChild(select);
        }
        else if (defaults[key].inputType === 'mympd-select-search') {
            const input = elCreateEmpty('input', {"class": ["form-select"], "id": id});
            setData(input, 'cb-filter', defaults[key].cbCallback);
            setData(input, 'cb-filter-options', [id + 'Input']);
            input.setAttribute('data-is', 'mympd-select-search');
            col.classList.add('position-relative');
            const btnGrp = elCreateNode('div', {"class": ["btn-group", "d-flex"]}, input);
            col.appendChild(btnGrp);
        }
        else if (defaults[key].inputType === 'checkbox') {
            const btn = elCreateEmpty('button', {"type": "button", "id": id, "class": ["btn", "btn-sm", "btn-secondary", "mi", "chkBtn"]});
            if (fields[key] === true) {
                btn.classList.add('active');
                btn.textContent = 'check';
            }
            else {
                btn.textContent = 'radio_button_unchecked';
            }
            if (defaults[key].onClick !== undefined) {
                btn.addEventListener('click', function(event) {
                    // @ts-ignore
                    window[defaults[key].onClick](event);
                }, false);
            }
            else {
                btn.addEventListener('click', function(event) {
                    toggleBtnChk(event.target, undefined);
                }, false);
            }
            col.appendChild(btn);
        }
        else if (defaults[key].inputType === 'password') {
            const input = elCreateEmpty('input', {"is": "mympd-input-password", "id": id,
                "value": fields[key], "class": ["form-control"], "type": "password"});
            col.appendChild(input);
        }
        else {
            const it = defaults[key].inputType === 'color'
                ? 'color'
                : 'text';
            const placeholder = defaults[key].placeholder !== undefined
                ? defaults[key].placeholder
                : defaults[key].defaultValue;
            const input = elCreateEmpty('input', {"is": "mympd-input-reset", "id": id, "data-default": defaults[key].defaultValue,
                "placeholder": placeholder, "value": fields[key], "class": ["form-control"], "type": it});
            col.appendChild(input);
        }
        // invalid feedback element for local validation
        if (defaults[key].invalid !== undefined) {
            col.appendChild(
                elCreateTextTn('div', {"class": ["invalid-feedback"]}, defaults[key].invalid)
            );
        }
        // warning element
        if (defaults[key].warn !== undefined) {
            col.appendChild(
                elCreateTextTn('div', {"id": id + 'Warn', "class": ["mt-2", "mb-1", "alert", "alert-warning", "d-none"]}, defaults[key].warn)
            );
        }
        // help text
        if (defaults[key].help !== undefined) {
            col.appendChild(
                elCreateTextTn('small', {"class": ["help"]}, defaults[key].help)
            );
        }
        // create the label
        const label = defaults[key].hint === undefined
            ? elCreateTextTn('label', {"class": ["col-sm-4", "col-form-label"], "for": id}, defaults[key].title)
            : elCreateNodes('label', {"class": ["col-sm-4", "col-form-label"], "for": id}, [
                    elCreateTextTn('span', {}, defaults[key].title),
                    elCreateText('small', {"class": ["mi", "mi-sm", "ms-1"], "title": tn("Browser specific setting"), "data-title-phrase": "Browser specific setting"}, defaults[key].hint)
              ]);
        // create the row and append it to the form field container
        let rowClasses = ["mb-3", "row"];
        if (defaults[key].cssClass !== undefined) {
            rowClasses = rowClasses.concat(defaults[key].cssClass);
        }
        forms[form].appendChild(
            elCreateNodes('div', {"class": rowClasses}, [
                label,
                col
            ])
        );
    }

    // add event handler
    for (const key in defaults) {
        if (defaults[key].onChange !== undefined) {
            const id = prefix + ucFirst(key) + 'Input';
            document.getElementById(id).addEventListener('change', function(event) {
                // @ts-ignore
                window[defaults[key].onChange](event);
            }, false);
        }
    }
}
